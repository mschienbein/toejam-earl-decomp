"""Discover code by recursive descent and write the function catalogue (addresses and sizes only).

    python tools/analyze.py [--version rev02]          # writes config/<version>/catalogue.json
    python tools/analyze.py [--version rev02] --check  # fails if the committed catalogue is stale

Seeds are the 68000 exception vectors plus config/<version>/seeds.json (hand-confirmed entry points,
e.g. functions only reached through pointer tables). From every seed the analyzer follows
fall-through, branches (bcc/dbcc/bra), direct jumps and calls (jmp/jsr absolute or PC-relative,
bsr). Call targets and seeds become functions; everything never reached as code stays data, which the
build reproduces with .incbin. A path stops at returns, computed jumps, invalid encodings, the end of
the ROM, or an instruction overlapping one already decoded (recorded as a conflict).

The catalogue holds no ROM bytes: addresses, sizes, counts, the ROM sha256 and this tool's sha256.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import m68k  # noqa: E402
import rom as romtool  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
HEADER_END = 0x200
VECTOR_NAMES = {
    1: "reset", 2: "bus_error", 3: "address_error", 4: "illegal_instruction", 5: "zero_divide",
    6: "chk", 7: "trapv", 8: "privilege_violation", 9: "trace", 10: "line_a", 11: "line_f",
    24: "spurious_irq", 25: "irq1", 26: "irq2_ext", 27: "irq3", 28: "irq4_hblank", 29: "irq5",
    30: "irq6_vblank", 31: "irq7",
    **{32 + n: f"trap{n}" for n in range(16)},
}


def catalogue_path(version: str) -> Path:
    return ROOT / "config" / version / "catalogue.json"


def seeds_path(version: str) -> Path:
    return ROOT / "config" / version / "seeds.json"


def vectors(rom: bytes) -> dict[int, list[str]]:
    """Code addresses named by the vector table (entry 0 is the initial stack pointer)."""
    found: dict[int, list[str]] = {}
    for index in range(1, 64):
        target = int.from_bytes(rom[index * 4:index * 4 + 4], "big")
        if HEADER_END <= target < len(rom) and target % 2 == 0:
            found.setdefault(target, []).append(VECTOR_NAMES.get(index, f"vector{index}"))
    return found


class Analysis:
    def __init__(self, rom: bytes):
        self.rom = rom
        self.insns: dict[int, m68k.Insn] = {}
        self.byte_owner: dict[int, int] = {}      # byte address -> instruction/table start
        self.functions: dict[int, str] = {}       # entry -> kind (vector | call | seed)
        self.labels: set[int] = set()
        self.tables: dict[int, dict] = {}         # switch jump tables (word offsets), by address
        self.conflicts: list[dict] = []
        self.invalid: list[int] = []
        self.computed: list[dict] = []
        self.out_of_rom: list[dict] = []

    def run(self, seeds: dict[int, str]) -> None:
        work = []
        for address, kind in sorted(seeds.items()):
            self.functions[address] = kind
            work.append(address)
        while work:
            self.sweep(work.pop(), work)

    def claim(self, start: int, size: int) -> int | None:
        """Owner of the first already-claimed byte in [start, start+size), else None."""
        for a in range(start, start + size):
            if a in self.byte_owner:
                return self.byte_owner[a]
        return None

    def switch_table(self, history: list[m68k.Insn], jump: m68k.Insn) -> dict | None:
        """Recognise the compiler's switch idiom and return its word-offset table.

            cmpi.w #N,Dn ; bgt/bhi default ; add.w Dn,Dn ; move.w T-2k(pc,Dn.w),Dn ; jmp T(pc,Dn.w)

        The table at T holds N-k+1 signed word offsets from T (k = lowest case index).
        """
        if len(history) < 4 or not jump.targets:
            return None
        cmp, bound, add, load = history[-4:]
        reg = jump.op_str.rsplit(",", 1)[-1].strip(" )").split(".")[0]
        if not (cmp.mnemonic == "cmpi.w" and cmp.op_str.endswith(f", {reg}") and bound.base in ("bgt", "bhi", "bge", "bcc")
                and add.mnemonic == "add.w" and add.op_str == f"{reg}, {reg}"
                and load.mnemonic == "move.w" and load.op_str.endswith(f"(pc, {reg}.w), {reg}") and load.targets):
            return None
        base, index_base = jump.targets[0], load.targets[0]
        if (base - index_base) % 2 or base < index_base:
            return None
        first = (base - index_base) // 2
        last = int(cmp.op_str.split(",")[0].lstrip("#$"), 16)
        count = last - first + 1
        if not 0 < count <= 512 or base + 2 * count > len(self.rom):
            return None
        targets = []
        for k in range(count):
            offset = int.from_bytes(self.rom[base + 2 * k:base + 2 * k + 2], "big", signed=True)
            target = base + offset
            if target % 2 or not HEADER_END <= target < len(self.rom):
                return None
            targets.append(target)
        return {"address": base, "count": count, "jump": jump.address, "index_base": index_base, "targets": targets}

    def sweep(self, start: int, work: list[int]) -> None:
        pc = start
        history: list[m68k.Insn] = []
        while True:
            if pc in self.insns:
                return  # joined already-decoded code
            if pc >= len(self.rom) or pc < HEADER_END or pc % 2:
                self.out_of_rom.append({"from": start, "address": pc})
                return
            insn = m68k.decode_one(self.rom, pc)
            if insn is None or pc + insn.size > len(self.rom):
                self.invalid.append(pc)
                return
            owner = self.claim(pc, insn.size)
            if owner is not None:
                self.conflicts.append({"address": pc, "overlaps": owner})
                return
            self.insns[pc] = insn
            for a in range(pc, pc + insn.size):
                self.byte_owner[a] = pc
            history.append(insn)
            target = insn.branch_target
            if insn.flow in ("branch", "jump", "call") and target is not None:
                if target < len(self.rom):
                    if insn.flow == "call":
                        self.functions.setdefault(target, "call")
                    else:
                        self.labels.add(target)
                    work.append(target)
                else:
                    self.out_of_rom.append({"from": pc, "address": target})
            if insn.flow in ("return", "jump", "stop"):
                return
            if insn.flow == "computed_jump":
                table = self.switch_table(history[:-1], insn)
                if table and self.claim(table["address"], 2 * table["count"]) is None:
                    self.tables[table["address"]] = table
                    for a in range(table["address"], table["address"] + 2 * table["count"]):
                        self.byte_owner[a] = table["address"]
                    for case in table["targets"]:
                        self.labels.add(case)
                        work.append(case)
                    # MCC68K follows a switch's table with the statement's exit jump: a BRA to the end
                    # label, which the table also uses. Nothing jumps to it, but it is the function's code.
                    after = table["address"] + 2 * table["count"]
                    exit_jump = m68k.decode_one(self.rom, after) if after < len(self.rom) else None
                    if exit_jump is not None and exit_jump.base == "bra" and exit_jump.branch_target in table["targets"]:
                        work.append(after)
                else:
                    self.computed.append({"address": pc})  # addresses only: no disassembly in the repository
                return
            pc += insn.size

    def code_ranges(self) -> list[tuple[int, int]]:
        """Maximal runs of instructions and the switch tables embedded between them."""
        spans = [(a, a + i.size) for a, i in self.insns.items()]
        spans += [(a, a + 2 * t["count"]) for a, t in self.tables.items()]
        ranges: list[list[int]] = []
        for start, end in sorted(spans):
            if ranges and ranges[-1][1] == start:
                ranges[-1][1] = end
            else:
                ranges.append([start, end])
        return [(a, b) for a, b in ranges]


def build_catalogue(rom: bytes, version: str, extra_seeds: dict[int, str]) -> dict:
    vec = vectors(rom)
    seeds = {address: "vector" for address in vec}
    for address, kind in extra_seeds.items():
        seeds.setdefault(address, kind)
    analysis = Analysis(rom)
    analysis.run(seeds)
    ranges = analysis.code_ranges()
    entries = sorted(a for a in analysis.functions if a in analysis.insns)
    functions = []
    for start, end in ranges:
        inside = [a for a in entries if start <= a < end]
        if not inside or inside[0] != start:
            inside.insert(0, start)  # code reached only by jumps from elsewhere: still a unit
        for k, address in enumerate(inside):
            stop = inside[k + 1] if k + 1 < len(inside) else end
            kind = analysis.functions.get(address, "fragment")
            entry = {"address": address, "size": stop - address, "kind": "function" if kind != "fragment" else "fragment",
                     "reached_by": kind}
            if address in vec:
                entry["vectors"] = vec[address]
            functions.append(entry)
    code_bytes = sum(b - a for a, b in ranges)
    data = []
    cursor = HEADER_END
    for start, end in ranges:
        if start > cursor:
            data.append({"address": cursor, "size": start - cursor})
        cursor = end
    if cursor < len(rom):
        data.append({"address": cursor, "size": len(rom) - cursor})
    tool = Path(__file__).resolve()
    return {
        "schema_version": 1,
        "version": version,
        "rom_sha256": hashlib.sha256(rom).hexdigest(),
        "analyzer_sha256": hashlib.sha256(tool.read_bytes() + (tool.parent / "m68k.py").read_bytes()).hexdigest(),
        "header": {"address": 0, "size": HEADER_END},
        "functions": functions,
        "code_ranges": [{"address": a, "size": b - a} for a, b in ranges],
        "data": data,
        "jump_tables": [{k: t[k] for k in ("address", "count", "jump", "index_base")} for _, t in sorted(analysis.tables.items())],
        "labels": sorted(t for t in analysis.labels if t in analysis.insns),
        "stats": {
            "rom_size": len(rom),
            "code_bytes": code_bytes,
            "code_percent": round(100.0 * code_bytes / len(rom), 3),
            "instructions": len(analysis.insns),
            "functions": sum(1 for f in functions if f["kind"] == "function"),
            "fragments": sum(1 for f in functions if f["kind"] == "fragment"),
            "computed_jumps": len(analysis.computed),
            "jump_tables": len(analysis.tables),
            "conflicts": len(analysis.conflicts),
            "invalid_stops": len(analysis.invalid),
        },
        "diagnostics": {
            "computed_jumps": analysis.computed,
            "conflicts": analysis.conflicts,
            "invalid_stops": sorted(set(analysis.invalid)),
            "out_of_rom": analysis.out_of_rom,
        },
    }


def load_seeds(version: str) -> dict[int, str]:
    path = seeds_path(version)
    if not path.is_file():
        return {}
    return {int(entry["address"], 16): entry.get("kind", "seed") for entry in json.loads(path.read_text())["seeds"]}


def render_json(catalogue: dict) -> str:
    """Stable, diff-friendly JSON: one function/range per line."""
    lines = ["{"]
    keys = list(catalogue)
    for n, key in enumerate(keys):
        value = catalogue[key]
        comma = "," if n + 1 < len(keys) else ""
        if isinstance(value, list) and value and isinstance(value[0], dict):
            lines.append(f' "{key}": [')
            lines.extend(f"  {json.dumps(item, sort_keys=True)}{',' if k + 1 < len(value) else ''}" for k, item in enumerate(value))
            lines.append(f" ]{comma}")
        else:
            lines.append(f' "{key}": {json.dumps(value, sort_keys=True)}{comma}')
    lines.append("}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default=romtool.title()["default_version"])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    rom = romtool.load(args.version)
    catalogue = build_catalogue(rom, args.version, load_seeds(args.version))
    text = render_json(catalogue)
    path = catalogue_path(args.version)
    if args.check:
        if not path.is_file() or path.read_text() != text:
            print(f"{path.relative_to(ROOT)} is stale; run tools/analyze.py --version {args.version}", file=sys.stderr)
            return 1
        print(f"{path.relative_to(ROOT)} OK")
        return 0
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    s = catalogue["stats"]
    print(f"{args.version}: {s['functions']} functions + {s['fragments']} fragments, {s['instructions']} instructions, "
          f"{s['code_bytes']} code bytes ({s['code_percent']}%), computed jumps {s['computed_jumps']}, "
          f"conflicts {s['conflicts']}, invalid stops {s['invalid_stops']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
