"""Create a PRIVATE author packet; never commit its output.

    python tools/packet.py func_XXXXXX [--version rev02] [--out DIR] [--json]

Output defaults to scratch/packets/<version>/<unit>.md (or .json). --json also prints JSON to
stdout. Catalogue tables are decoded as tables, never as instructions. Indirect calls without
catalogued switch-table evidence remain unresolved. Prototype hints are observations, not an ABI.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import importlib.util
import json
import re
import sys
import subprocess
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECODERS = {}


class PacketError(RuntimeError):
    pass


def number(value: int | str) -> int:
    return int(value, 0) if isinstance(value, str) else value


def decoder(root: Path):
    """Load the repository's existing decoder, including when running an unintegrated proposal."""
    path = root.resolve() / "tools/m68k.py"
    if path not in DECODERS:
        spec = importlib.util.spec_from_file_location("_packet_m68k_" + hashlib.sha256(str(path).encode()).hexdigest()[:12], path)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        DECODERS[path] = module
    return DECODERS[path]


def unit_name(unit: dict) -> str:
    return f"{'func' if unit['kind'] == 'function' else 'frag'}_{unit['address']:06x}"


class Symbols:
    """A stable primary name plus aliases/provenance; offsets require an observed extent."""

    def __init__(self, catalogue: dict, names: dict | None = None):
        self.entries: dict[int, list[dict]] = {}
        self.by_name: dict[str, int] = {}
        for unit in catalogue["functions"]:
            self.add({**unit, "name": unit_name(unit), "source": "catalogue", "evidence": "catalogued extent"})
        if "header" in catalogue:
            self.add({**catalogue["header"], "name": "header_000000", "kind": "header", "source": "catalogue"})
        for span in catalogue.get("data", []):
            self.add({**span, "name": f"data_{span['address']:06x}", "kind": "data", "source": "catalogue"})
        for address in catalogue.get("labels", []):
            self.add({"address": address, "name": f"loc_{address:06x}", "kind": "label", "source": "catalogue"})
        for table in catalogue.get("jump_tables", []):
            self.add({"address": table["address"], "size": 2 * table["count"], "name": f"jtbl_{table['address']:06x}",
                      "kind": "jump_table", "source": "catalogue"})
        for record in (names or {}).get("symbols", []):
            self.add(record, primary=True)
            for alias in record.get("aliases", []):
                alias = {"name": alias} if isinstance(alias, str) else alias
                self.add({**record, **alias, "address": record["address"]})
        self.starts = sorted(self.entries)

    def add(self, record: dict, primary: bool = False) -> None:
        record = {**record, "address": number(record["address"])}
        if "size" in record:
            record["size"] = number(record["size"])
        name, address = record["name"], record["address"]
        if name in self.by_name and self.by_name[name] != address:
            raise PacketError(f"symbol {name!r} has conflicting addresses")
        self.by_name[name] = address
        entries = self.entries.setdefault(address, [])
        if primary:
            entries.insert(0, record)
        else:
            entries.append(record)

    def identify(self, address: int) -> tuple[str, int]:
        if address in self.entries:
            return self.entries[address][0]["name"], 0
        # Search all preceding spans: an exact zero-size label must not hide its enclosing function.
        for start in reversed(self.starts[:bisect.bisect_right(self.starts, address)]):
            records = self.entries[start]
            if any(start <= address < start + r.get("size", 0) for r in records):
                return records[0]["name"], address - start
        prefix = "ram" if 0xFF0000 <= address <= 0xFFFFFF else "addr"
        return f"{prefix}_{address:06x}", 0

    def name(self, address: int) -> str:
        name, offset = self.identify(address)
        return name if not offset else f"{name}+0x{offset:x}"

    def records(self, address: int) -> list[dict]:
        return self.entries.get(address, [])


def load_inputs(root: Path, version: str) -> tuple[bytes, dict, Symbols]:
    title = json.loads((root / "config/title.json").read_text())
    if version not in title["versions"]:
        raise PacketError(f"unknown version {version!r}")
    spec = title["versions"][version]
    path = root / spec["orig"]
    if not path.is_file():
        raise PacketError(f"{path}: private ROM missing; run tools/rom.py import")
    rom = path.read_bytes()
    if hashlib.sha256(rom).hexdigest() != spec["sha256"]:
        raise PacketError(f"{path}: ROM does not match its pinned SHA-256")
    catalogue = json.loads((root / "config" / version / "catalogue.json").read_text())
    if catalogue["version"] != version or catalogue["rom_sha256"] != hashlib.sha256(rom).hexdigest():
        raise PacketError("catalogue version/ROM identity does not match the selected ROM")
    names_path = root / "config" / version / "names.json"
    names = json.loads(names_path.read_text()) if names_path.is_file() else None
    if names is not None and (names.get("schema_version") != 1 or names.get("version") != version):
        raise PacketError(f"{names_path}: unsupported names schema/version")
    return rom, catalogue, Symbols(catalogue, names)


class Program:
    def __init__(self, rom: bytes, catalogue: dict, symbols: Symbols, m68k):
        self.rom, self.catalogue, self.symbols, self.m68k = rom, catalogue, symbols, m68k
        self.units = sorted(catalogue["functions"], key=lambda u: u["address"])
        self.starts = [u["address"] for u in self.units]
        self.tables = {}
        for record in catalogue.get("jump_tables", []):
            start, count = record["address"], record["count"]
            if start < 0 or start + count * 2 > len(rom):
                raise PacketError(f"jump table outside ROM at 0x{start:x}")
            targets = [start + int.from_bytes(rom[a:a + 2], "big", signed=True) for a in range(start, start + count * 2, 2)]
            self.tables[start] = {**record, "targets": targets}
        self.instructions = {}
        self.by_unit = {}
        for unit in self.units:
            pc, end = unit["address"], unit["address"] + unit["size"]
            if pc < 0 or end > len(rom):
                raise PacketError(f"unit outside ROM: {unit_name(unit)}")
            insns = []
            while pc < end:
                if pc in self.tables:
                    table_end = pc + self.tables[pc]["count"] * 2
                    if table_end > end:
                        raise PacketError("jump table crosses a unit boundary")
                    pc = table_end
                    continue
                insn = m68k.decode_one(rom, pc)
                if insn is None or pc + insn.size > end:
                    raise PacketError(f"undecodable instruction in {unit_name(unit)} at 0x{pc:06x}")
                self.instructions[pc] = insn
                insns.append(insn)
                pc += insn.size
            self.by_unit[unit["address"]] = insns
        self.edges = []
        self.unresolved = []
        table_by_jump = {t["jump"]: t for t in self.tables.values()}
        for unit in self.units:
            for insn in self.by_unit[unit["address"]]:
                targets = []
                if insn.flow == "call" and insn.branch_target is not None:
                    targets.append(("call", insn.branch_target))
                elif insn.address in table_by_jump:
                    targets.extend(("switch", t) for t in table_by_jump[insn.address]["targets"])
                elif insn.flow == "jump" and insn.branch_target is not None:
                    target_unit = self.owner(insn.branch_target)
                    if target_unit is not None and target_unit["address"] != unit["address"]:
                        targets.append(("tail_jump", insn.branch_target))
                elif insn.flow in ("computed_call", "computed_jump"):
                    self.unresolved.append({"from": unit["address"], "site": insn.address, "kind": insn.flow})
                for kind, target in targets:
                    owner = self.owner(target)
                    self.edges.append({"from": unit["address"], "site": insn.address, "target": target,
                                       "to": owner["address"] if owner else None, "kind": kind})

    def owner(self, address: int) -> dict | None:
        pos = bisect.bisect_right(self.starts, address) - 1
        if pos >= 0:
            unit = self.units[pos]
            if address < unit["address"] + unit["size"]:
                return unit
        return None

    def select(self, name: str) -> dict:
        match = re.fullmatch(r"(func|frag)_([0-9a-fA-F]{6})", name)
        if not match:
            raise PacketError("unit must be func_XXXXXX or frag_XXXXXX")
        address = int(match[2], 16)
        unit = self.owner(address)
        if unit is None or unit["address"] != address or unit_name(unit) != name.lower():
            raise PacketError(f"{name}: not a catalogued unit start")
        return unit

    def assembly(self, insn) -> str:
        text = self.m68k.render(insn, lambda address, required: self.symbols.name(address))
        if text is None:
            text = f"{insn.mnemonic} {insn.op_str}".strip()
        # The build renderer intentionally leaves RAM absolute operands numeric. Packets are
        # descriptive, so symbolize those too, including sign-extended absolute-short addresses.
        def absolute(match):
            value = int(match[1], 16)
            if match[2] == "w" and value & 0x8000:
                value |= 0xFF0000
            value &= 0xFFFFFF
            return f"({self.symbols.name(value)}).{match[2]}"
        text = re.sub(r"\((?:0x|\$)([0-9a-fA-F]+)\)\.([wl])", absolute, text)
        def immediate(match):
            value = int(match[1], 16)
            if value in self.symbols.entries or 0xFF0000 <= value <= 0xFFFFFF:
                return "#" + self.symbols.name(value)
            return match[0]
        return re.sub(r"#(?:0x|\$)([0-9a-fA-F]+)", immediate, text)


STACK_REF = re.compile(r"(?P<disp>-?(?:\$[0-9a-fA-F]+|0x[0-9a-fA-F]+|\d+))\((?P<reg>sp|a7|a6)\)")


def signed_literal(text: str, bits: int = 16) -> int:
    text = text.strip().lstrip("#")
    value = int(text.replace("$", "0x"), 0) if "x" in text or "$" in text else int(text)
    if value >= 1 << (bits - 1):
        value -= 1 << bits
    return value


def register_count(text: str) -> int:
    count = 0
    for group in text.split("/"):
        if "-" in group:
            first, last = group.split("-", 1)
            count += int(last[-1]) - int(first[-1]) + 1
        elif re.fullmatch(r"[ad][0-7]|sp", group):
            count += 1
    return count


def stack_transfer(insn, state: tuple[int | None, int | None]) -> tuple[int | None, int | None]:
    sp, fp = state
    ops = insn.op_str.replace(" ", "").replace("a7", "sp")
    size = {"b": 2, "w": 2, "l": 4}.get(insn.mnemonic.rsplit(".", 1)[-1], 4)
    def adjust(value, amount):
        return None if value is None else value + amount
    if insn.base == "link" and ops.startswith("a6,"):
        fp = adjust(sp, -4)
        sp = adjust(fp, signed_literal(ops.split(",", 1)[1]))
    elif insn.base == "unlk" and ops == "a6":
        sp, fp = adjust(fp, 4), None
    elif insn.base == "movem":
        regs = ops.split(",")[0] if ops.endswith("-(sp)") else ops.rsplit(",", 1)[-1]
        amount = register_count(regs) * size
        if "-(sp)" in ops:
            sp = adjust(sp, -amount)
        elif "(sp)+" in ops:
            sp = adjust(sp, amount)
    elif insn.base == "pea":
        sp = adjust(sp, -4)
    else:
        sp = adjust(sp, size * (ops.count("(sp)+") - ops.count("-(sp)")))
        if insn.base in ("add", "adda", "addq", "sub", "suba", "subq") and ops.endswith(",sp"):
            try:
                delta = signed_literal(ops.split(",")[0], 32 if insn.mnemonic.endswith(".l") else 16)
                sp = adjust(sp, delta if insn.base.startswith("add") else -delta)
            except ValueError:
                sp = None
        elif insn.base == "lea" and ops.endswith(",sp"):
            match = STACK_REF.fullmatch(ops.rsplit(",", 1)[0])
            sp = adjust(sp if match and match["reg"] in ("sp", "a7") else fp, signed_literal(match["disp"])) if match else None
        elif insn.base in ("move", "movea"):
            if ops == "sp,a6":
                fp = sp
            elif ops.endswith(",sp"):
                sp = fp if ops == "a6,sp" else None
            elif ops.endswith(",a6"):
                fp = None
    if insn.base == "exg":
        registers = ops.split(",")
        if "sp" in registers:
            sp = None
        if "a6" in registers:
            fp = None
    elif insn.base not in ("link", "unlk", "move", "movea", "cmp", "cmpa", "tst", "btst") and ops.endswith(",a6"):
        fp = None
    if insn.base == "movem" and "(sp)+" in ops and re.search(r"(?:^|[/,-])a6(?:$|[/,-])", ops.rsplit(",", 1)[-1]):
        fp = None
    return sp, fp


def stack_usage(program: Program, unit: dict) -> dict:
    insns = {i.address: i for i in program.by_unit[unit["address"]]}
    incoming = {unit["address"]: (0, None)}
    work = deque([unit["address"]])
    table_by_jump = {t["jump"]: t for t in program.tables.values()}
    while work:
        address = work.popleft()
        insn = insns.get(address)
        if insn is None:
            continue
        state = stack_transfer(insn, incoming[address])
        successors = []
        if insn.flow not in ("return", "stop", "jump", "computed_jump"):
            successors.append(address + insn.size)
        if insn.flow in ("branch", "jump") and insn.branch_target is not None:
            successors.append(insn.branch_target)
        if address in table_by_jump:
            successors += table_by_jump[address]["targets"]
        for target in successors:
            if target not in insns:
                continue
            old = incoming.get(target)
            merged = state if old is None else tuple(a if a == b else None for a, b in zip(old, state))
            if old != merged:
                incoming[target] = merged
                work.append(target)
    accesses = []
    for address, insn in sorted(insns.items()):
        state = incoming.get(address, (None, None))
        for match in STACK_REF.finditer(insn.op_str):
            displacement = signed_literal(match["disp"])
            base = state[0] if match["reg"] in ("sp", "a7") else state[1]
            entry_offset = None if base is None else base + displacement
            accesses.append({"site": address, "base": match["reg"], "displacement": displacement,
                             "entry_offset": entry_offset,
                             "width": {"b": 1, "w": 2, "l": 4}.get(insn.mnemonic.rsplit(".", 1)[-1]),
                             "assembly": program.assembly(insn)})
    arguments = {}
    for access in accesses:
        offset = access["entry_offset"]
        if offset is not None and offset >= 4:
            arguments[offset] = max(arguments.get(offset, 0), access["width"] or 0)
    return {"accesses": accesses, "argument_hints": [{"entry_offset": offset, "width": width or None}
                                                    for offset, width in sorted(arguments.items())],
            "qualification": "[INFERENCE] Stack slots, not proven parameters/types. CFG joins with inconsistent SP/A6 become unknown; register parameters, aliases, and dynamic stack allocation are not inferred."}


def make_packet(program: Program, unit: dict, version: str) -> dict:
    address, size = unit["address"], unit["size"]
    symbols = program.symbols
    assembly = []
    references = {"ram": {}, "rom_data": {}}
    data_spans = program.catalogue.get("data", []) + ([program.catalogue["header"]] if "header" in program.catalogue else [])
    for insn in program.by_unit[address]:
        assembly.append({"address": insn.address, "size": insn.size, "text": program.assembly(insn)})
        targets = [(target, "decoded address operand") for target in insn.targets]
        for match in re.finditer(r"#\$([0-9a-fA-F]+)", insn.op_str):
            target = int(match[1], 16)
            if target in symbols.entries or 0xFF0000 <= target <= 0xFFFFFF:
                targets.append((target, "immediate value; pointer interpretation is [INFERENCE]"))
        for target, evidence in targets:
            if insn.flow in ("call", "branch", "jump") and target == insn.branch_target:
                continue
            kind = "ram" if 0xFF0000 <= target <= 0xFFFFFF else None
            if kind is None and any(d["address"] <= target < d["address"] + d["size"] for d in data_spans):
                kind = "rom_data"
            if kind:
                ref = references[kind].setdefault(target, {"address": target, "name": symbols.name(target), "uses": []})
                ref["uses"].append({"site": insn.address, "evidence": evidence})
    tables = []
    for start, table in sorted(program.tables.items()):
        if address <= start < address + size:
            tables.append({**table, "name": symbols.name(start),
                           "target_names": [symbols.name(t) for t in table["targets"]]})
            assembly.append({"address": start, "size": table["count"] * 2,
                             "text": ".short " + ", ".join(f"{symbols.name(t)}-{symbols.name(start)}" for t in table["targets"])})
    def edge_record(edge):
        return {**edge, "from_name": symbols.name(edge["from"]), "target_name": symbols.name(edge["target"])}
    callers = [edge_record(e) for e in program.edges if e["to"] == address and (e["kind"] != "switch" or e["from"] != address)]
    callees = [edge_record(e) for e in program.edges if e["from"] == address and (e["kind"] != "switch" or e["to"] != address)]
    usage = stack_usage(program, unit)
    name = re.sub(r"\W", "_", symbols.name(address))
    if name[:1].isdigit():
        name = "fn_" + name
    params = [f"uint{hint['width'] * 8 if hint['width'] in (1, 2, 4) else 32}_t stack_at_{hint['entry_offset']:x}"
              for hint in usage["argument_hints"]]
    skeleton = ("#include <stdint.h>\n\n/* [INFERENCE] Signature sketch only: return type and ABI are unknown. */\n"
                f"void {name}({', '.join(params) or 'void'})\n{{\n    /* Reconstruct behavior from the private packet; this is not a matching candidate. */\n}}\n")
    return {"schema_version": 1, "private": True, "version": version, "unit": unit_name(unit),
            "address": address, "size": size, "kind": unit["kind"], "name": symbols.name(address),
            "symbols": symbols.records(address), "assembly": sorted(assembly, key=lambda row: row["address"]),
            "callers": callers, "callees": callees, "call_graph_edges": [edge_record(e) for e in program.edges if e["from"] == address],
            "unresolved_control_flow": [e for e in program.unresolved if e["from"] == address],
            "ram_references": list(references["ram"].values()), "rom_data_references": list(references["rom_data"].values()),
            "jump_tables": tables, "stack_usage": usage, "c_skeleton": skeleton}


def markdown(packet: dict) -> str:
    lines = [f"# PRIVATE: {packet['unit']} — {packet['name']}", "", "**ROM-derived: never commit this packet.**", "",
             f"Version: `{packet['version']}`; address: `0x{packet['address']:06x}`; size: `{packet['size']}`; kind: `{packet['kind']}`.", "",
             "## Names and provenance", ""]
    for record in packet["symbols"]:
        lines.append(f"- `{record['name']}`: {record.get('source', 'unspecified')}; evidence: {record.get('evidence', 'not supplied')}")
    lines += ["", "## Symbolized assembly", "", "```asm"]
    for row in packet["assembly"]:
        lines.append(f"{row['address']:06x}:  {row['text']}")
    lines += ["```", ""]
    for key, heading in (("callers", "Callers"), ("callees", "Callees")):
        lines += [f"## {heading}", ""]
        for edge in packet[key]:
            lines.append(f"- `{edge['from_name']}` at `0x{edge['site']:06x}` → `{edge['target_name']}` ({edge['kind']}).")
        if not packet[key]:
            lines.append("None observed in catalogued direct control flow.")
        lines.append("")
    if packet["unresolved_control_flow"]:
        lines += ["Unresolved computed control flow (not assumed absent):", ""]
        lines += [f"- `0x{edge['site']:06x}`: {edge['kind']}" for edge in packet["unresolved_control_flow"]]
        lines.append("")
    for key, heading in (("ram_references", "RAM references"), ("rom_data_references", "ROM data references")):
        lines += [f"## {heading}", ""]
        for ref in packet[key]:
            lines.append(f"- `{ref['name']}` (`0x{ref['address']:06x}`): " + "; ".join(f"0x{use['site']:06x} — {use['evidence']}" for use in ref["uses"]))
        if not packet[key]:
            lines.append("None observed.")
        lines.append("")
    lines += ["## Jump tables", ""]
    for table in packet["jump_tables"]:
        lines.append(f"- `{table['name']}`: {table['count']} signed word offsets; jump `0x{table['jump']:06x}`; index base `0x{table['index_base']:06x}`.")
        lines.append("  Targets: " + ", ".join(f"`{name}`" for name in table["target_names"]))
    if not packet["jump_tables"]:
        lines.append("None catalogued.")
    lines += ["", "## Argument-offset observations", "", packet["stack_usage"]["qualification"], ""]
    for access in packet["stack_usage"]["accesses"]:
        normalized = "unknown" if access["entry_offset"] is None else f"{access['entry_offset']:+d}"
        lines.append(f"- `0x{access['site']:06x}`: `{access['displacement']}({access['base']})`; entry-SP offset {normalized}; width {access['width'] or 'unknown'}.")
    if not packet["stack_usage"]["accesses"]:
        lines.append("No displacement-based SP/A6 accesses observed; this does not prove a no-argument prototype.")
    lines += ["", "## C signature/body sketch", "", "```c", packet["c_skeleton"].rstrip(), "```", ""]
    return "\n".join(lines)


def private_output_path(repo: Path, path: Path) -> Path:
    """Both requested and physical destinations must be untracked and ignored in this worktree."""
    result = subprocess.run(["git", "-C", str(repo), "rev-parse", "--show-toplevel"], capture_output=True, text=True)
    if result.returncode:
        raise PacketError("packet output requires a Git worktree with an explicit ignore policy")
    root = Path(result.stdout.strip()).resolve()
    requested, resolved = path.absolute(), path.resolve()
    try:
        parts = requested.relative_to(root).parts
    except ValueError as error:
        raise PacketError("private packet output must stay inside the repository") from error
    lexical = root
    for part in parts:
        lexical /= part
        if lexical.is_symlink():
            # Git refuses pathspecs below a symlink. Check the lexical alias itself, then the
            # completely resolved destination; both must independently be ignored and untracked.
            break
    for destination in {lexical, resolved}:
        try:
            relative = destination.relative_to(root)
        except ValueError as error:
            raise PacketError("private packet output must resolve inside the repository's ignored paths") from error
        tracked = subprocess.run(["git", "-C", str(root), "ls-files", "-z", "--", relative.as_posix()], capture_output=True)
        if tracked.returncode or tracked.stdout:
            raise PacketError(f"refusing tracked packet destination {relative}")
        ignored = subprocess.run(["git", "-C", str(root), "check-ignore", "--no-index", "-q", "--", relative.as_posix()], capture_output=True)
        if ignored.returncode:
            raise PacketError(f"packet destination {relative} is not Git-ignored")
    return resolved


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("unit")
    parser.add_argument("--version", default="rev02")
    parser.add_argument("--repo", type=Path, default=ROOT, help="repository root (also supports running a proposal copy)")
    parser.add_argument("--out", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        rom, catalogue, symbols = load_inputs(args.repo, args.version)
        program = Program(rom, catalogue, symbols, decoder(args.repo))
        packet = make_packet(program, program.select(args.unit), args.version)
        out = args.out or args.repo / "scratch/packets" / args.version
        path = private_output_path(args.repo, out / f"{packet['unit']}{'.json' if args.json else '.md'}")
        path.parent.mkdir(parents=True, exist_ok=True)
        path = private_output_path(args.repo, path)
        text = json.dumps(packet, indent=2) + "\n" if args.json else markdown(packet)
        path.write_text(text)
        if args.json:
            print(text, end="")
            print(f"PRIVATE packet written to {path}", file=sys.stderr)
        else:
            print(f"PRIVATE packet written to {path}")
    except (PacketError, OSError, ValueError, KeyError, RuntimeError) as error:
        print(f"packet: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
