"""Build the ROM from the catalogue and prove it byte-identical to the original.

    python tools/build.py [--version rev02] [--jobs N]

Everything is generated under build/<version>/ (git-ignored; it embeds ROM-derived bytes):

1. units: the vector table + cartridge header (0x000-0x1FF), one unit per catalogued function or
   fragment, one per data gap between code;
2. code units are rendered to GNU as by tools/m68k.py; data units are `.incbin` slices of the
   private original;
3. every unit is assembled (m68k-elf-as -m68000 --bitwise-or --pcrel -L); each instruction's size
   and, where no relocation covers it, its bytes are checked in the object; mismatches are
   re-emitted as raw `.short` words and the unit is reassembled;
4. a generated linker script places each unit at its original address; ld + objcopy -O binary;
5. the image is compared with the original; any instruction still differing (a relocated operand)
   falls back to raw words and the link repeats;
6. receipt.json records PASS only for a byte-identical image, with the input, tool and toolchain
   hashes and how many instructions are readable assembly versus raw words.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

sys.path.insert(0, str(Path(__file__).resolve().parent))
import cunits  # noqa: E402
import m68k  # noqa: E402
import rom as romtool  # noqa: E402
import names as namestool  # noqa: E402
import toolchain  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
TARGET = "m68k-elf"
AS_FLAGS = ["-m68000", "--bitwise-or", "--pcrel", "-L"]
MAX_ROUNDS = 8


class BuildError(RuntimeError):
    pass


@dataclass
class Table:
    """A switch jump table inside a code unit: signed word offsets from its own address."""
    address: int
    size: int
    raw: bytes
    index_base: int
    mnemonic: str = ".jtbl"

    @property
    def targets(self) -> list[int]:
        return [self.address + int.from_bytes(self.raw[k:k + 2], "big", signed=True) for k in range(0, self.size, 2)]


@dataclass
class Unit:
    name: str
    address: int
    size: int
    kind: str                                    # header | function | fragment | data
    insns: list = field(default_factory=list)    # m68k.Insn and Table items in address order

    @property
    def end(self) -> int:
        return self.address + self.size


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_units(rom: bytes, catalogue: dict) -> list[Unit]:
    units = [Unit(f"header_{0:06x}", 0, catalogue["header"]["size"], "header")]
    for f in catalogue["functions"]:
        prefix = "func" if f["kind"] == "function" else "frag"
        units.append(Unit(f"{prefix}_{f['address']:06x}", f["address"], f["size"], f["kind"]))
    for d in catalogue["data"]:
        units.append(Unit(f"data_{d['address']:06x}", d["address"], d["size"], "data"))
    units.sort(key=lambda u: u.address)
    cursor = 0
    for unit in units:
        if unit.address != cursor:
            raise BuildError(f"catalogue leaves a gap or overlap at 0x{cursor:06x} ({unit.name})")
        cursor = unit.end
    if cursor != len(rom):
        raise BuildError(f"catalogue covers 0x{cursor:x} bytes, ROM has 0x{len(rom):x}")
    tables = {t["address"]: t for t in catalogue.get("jump_tables", [])}
    for unit in units:
        if unit.kind in ("function", "fragment"):
            pc = unit.address
            while pc < unit.end:
                if pc in tables:
                    size = 2 * tables[pc]["count"]
                    unit.insns.append(Table(pc, size, rom[pc:pc + size], tables[pc]["index_base"]))
                    pc += size
                    continue
                insn = m68k.decode_one(rom, pc)
                if insn is None or pc + insn.size > unit.end:
                    raise BuildError(f"{unit.name}: undecodable instruction at 0x{pc:06x}")
                unit.insns.append(insn)
                pc += insn.size
    return units


class Symbols:
    """Names for every referenced address: inline labels at instruction starts, absolute otherwise.

    Imported names (config/<version>/names.json) replace the generated func_/loc_/dat_ names; RAM
    names are used for absolute long operands and defined in the linker script.
    """

    def __init__(self, units: list[Unit], catalogue: dict, names: dict | None = None):
        self.names = names or {}
        self.starts = {i.address for u in units for i in u.insns if not isinstance(i, Table)}
        self.labels: dict[int, str] = {}
        self.alias: dict[int, str] = {}           # in-unit expressions for unlabelled addresses
        for u in units:
            if u.kind == "function":
                self.labels[u.address] = self.code_name(u.address, u.name)
            elif u.kind == "fragment":
                self.labels[u.address] = self.code_name(u.address, f"loc_{u.address:06x}")
            for item in u.insns:
                if isinstance(item, Table):
                    name = f"jtbl_{item.address:06x}"
                    self.labels[item.address] = name
                    if item.index_base != item.address:  # 1-based switch: the load indexes from T-2k
                        self.alias[item.index_base] = f"{name}-{item.address - item.index_base}"
        for address in catalogue.get("labels", []):
            self.labels.setdefault(address, self.code_name(address, f"loc_{address:06x}"))
        self.absolute: dict[int, str] = {}       # linker-script definitions (ROM data, RAM)

    def code_name(self, address: int, generated: str) -> str:
        known = self.names.get(address)
        return known.name if known is not None and known.kind in ("function", "label") else generated

    def __call__(self, address: int, required: bool) -> str | None:
        if address in self.labels:
            return self.labels[address]
        if address in self.alias:
            return self.alias[address]
        if address in self.starts:
            self.labels[address] = self.code_name(address, f"loc_{address:06x}")
            return self.labels[address]
        known = self.names.get(address)
        if known is not None and (required or known.kind in ("ram", "rom_data")):
            return self.absolute.setdefault(address, known.name)
        if not required:
            return None
        return self.absolute.setdefault(address, f"dat_{address:06x}")


def render_unit(unit: Unit, symbols: Symbols, raw: set[int], baserom: Path) -> tuple[str, dict[int, int]]:
    """Assembly text for one unit and the source line of each instruction (1-based)."""
    lines = [f'\t.section .text.{unit.name},"ax"' if unit.insns else f'\t.section .text.{unit.name},"a"']
    if not unit.insns:
        lines.append(f'\t.incbin "{baserom}", 0x{unit.address:x}, 0x{unit.size:x}')
        return "\n".join(lines) + "\n", {}
    local = {symbols.labels[i.address] for i in unit.insns if i.address in symbols.labels}
    body = []
    for insn in unit.insns:
        label = symbols.labels.get(insn.address)
        if isinstance(insn, Table):
            text = None if insn.address in raw else ".short " + ", ".join(f"{symbols(t, True)}-{label}" for t in insn.targets)
        else:
            text = None if insn.address in raw else m68k.render(insn, symbols)
        if (text is not None and isinstance(insn, m68k.Insn) and insn.size == 2
                and insn.flow in ("branch", "jump", "call") and not insn.base.startswith("db")):
            target = symbols(insn.branch_target, True)
            if target not in local:
                # gas range-checks an 8-bit PC-relative fixup to a symbol in another section against
                # its in-section offset and rejects it, so emit the R_68K_PC8 relocation explicitly.
                text = f".reloc .+1, R_68K_PC8, {target}-1; .byte 0x{insn.raw[0]:02x}, 0\t/* {text} */"
        if text is None:
            text = m68k.words(insn.raw)
        if label:
            body.append(f"{label}:")
        body.append(f".Li_{insn.address:06x}:\t{text}")
    lines.append("\t.globl " + ", ".join(sorted(local)) if local else "")
    lines.extend(body)
    line_of = {}
    for number, text in enumerate(lines, start=1):
        if text.startswith(".Li_"):
            line_of[number] = int(text[4:10], 16)
    return "\n".join(lines) + "\n", line_of


def assemble(source: Path, obj: Path) -> list[int]:
    """Assemble; return the source lines gas rejected (empty on success)."""
    result = subprocess.run([str(toolchain.tool(TARGET, "as")), *AS_FLAGS, "-o", str(obj), str(source)],
                            capture_output=True, text=True)
    if result.returncode == 0:
        return []
    bad = [int(m.group(1)) for m in re.finditer(rf"^{re.escape(str(source))}:(\d+): Error", result.stderr, re.M)]
    if not bad:
        raise BuildError(f"{source.name}: {result.stderr.strip()[:2000]}")
    return bad


def check_object(unit: Unit, obj: Path) -> tuple[set[int], set[int]]:
    """(mis-sized, mis-encoded) instruction addresses in an assembled code unit."""
    with obj.open("rb") as stream:
        elf = ELFFile(stream)
        section = elf.get_section_by_name(f".text.{unit.name}")
        data = section.data()
        index = elf.get_section_index(f".text.{unit.name}")
        markers = {}
        for sym in elf.get_section_by_name(".symtab").iter_symbols():
            if sym.name.startswith(".Li_") and sym["st_shndx"] == index:
                markers[int(sym.name[4:], 16)] = sym["st_value"]
        relocated = set()
        for sec in elf.iter_sections():
            if isinstance(sec, RelocationSection) and sec["sh_info"] == index:
                for rel in sec.iter_relocations():
                    relocated.add(rel["r_offset"])
    sized, encoded = set(), set()
    offsets = [markers[i.address] for i in unit.insns] + [len(data)]
    for k, insn in enumerate(unit.insns):
        start, stop = offsets[k], offsets[k + 1]
        if stop - start != insn.size:
            sized.add(insn.address)
        elif not any(start <= r < stop for r in relocated) and data[start:stop] != insn.raw:
            encoded.add(insn.address)
    return sized, encoded


def linker_script(units: list[Unit], symbols: Symbols, extra: dict[str, int] | None = None) -> str:
    lines = ['OUTPUT_FORMAT("elf32-m68k")', "OUTPUT_ARCH(m68k)", "SECTIONS", "{", "  .rom 0x0 : AT(0x0)", "  {"]
    lines += [f"    . = 0x{u.address:06x}; KEEP(obj/{u.name}.o(.text.{u.name}))" for u in units]
    lines += ["  }", "  /DISCARD/ : { *(.comment) *(.note*) }", "}"]
    definitions = {name: address for address, name in symbols.absolute.items()}
    definitions.update(extra or {})
    lines += [f"{name} = 0x{address:06x};" for name, address in sorted(definitions.items(), key=lambda kv: (kv[1], kv[0]))]
    return "\n".join(lines) + "\n"


def tool_hashes() -> dict:
    here = Path(__file__).resolve().parent
    return {name: sha256((here / name).read_bytes()) for name in ("build.py", "cunits.py", "m68k.py", "mcc.py", "names.py", "rom.py", "toolchain.py")}


def build(version: str, jobs: int) -> dict:
    """Build one catalogued version of the game; writes build/<version>/receipt.json."""
    rom = romtool.load(version)
    baserom = ROOT / romtool.title()["versions"][version]["orig"]
    catalogue_file = ROOT / "config" / version / "catalogue.json"
    catalogue = json.loads(catalogue_file.read_text())
    if catalogue["rom_sha256"] != sha256(rom):
        raise BuildError(f"{catalogue_file.relative_to(ROOT)} was made for a different ROM")
    out = ROOT / "build" / version
    result = build_image(rom, baserom, catalogue, out, jobs, namestool.load(version), cunits.load_sources(version))
    result["inputs"]["catalogue_sha256"] = sha256(catalogue_file.read_bytes())
    names_file = namestool.path(version)
    result["inputs"]["names_sha256"] = sha256(names_file.read_bytes()) if names_file.is_file() else None
    receipt = {"schema_version": 1, "result": result.pop("result"), "version": version, **result}
    (out / "receipt.json").write_text(json.dumps(receipt, indent=1) + "\n")
    return receipt


def place_c_units(c_units: list, units: list[Unit], symbols: Symbols, names: dict | None) -> tuple[list[Unit], dict[str, int]]:
    """Put each C unit in place of the catalogued units it reproduces; the definitions it needs."""
    label_address = {label: address for address, label in symbols.labels.items()}
    replaced: set[str] = set()
    placed: list[Unit] = []
    c_defined: set[str] = set()
    for cu in c_units:
        first = cu.functions[0][0]
        if first not in label_address:
            raise BuildError(f"{cu.source.name}: {first} is not the name of a catalogued address")
        base = label_address[first]
        for name, offset, _ in cu.functions:
            if label_address.get(name) != base + offset:
                raise BuildError(f"{cu.source.name}: {name} would land at 0x{base + offset:06x}; functions must use their "
                                 f"canonical names and be one contiguous run in ROM order")
        covered = [u for u in units if base <= u.address < base + cu.size]
        if not covered or covered[0].address != base or covered[-1].end != base + cu.size:
            raise BuildError(f"{cu.source.name}: its {cu.size} bytes do not end on a unit boundary at 0x{base + cu.size:06x}")
        if any(u.kind == "header" or u.name in replaced for u in covered):
            raise BuildError(f"{cu.source.name}: overlaps the header or another C unit")
        cu.address = base
        replaced.update(u.name for u in covered)
        c_defined |= cu.defined
        placed.append(Unit(cu.name, base, cu.size, "c"))
    remaining = sorted([u for u in units if u.name not in replaced] + placed, key=lambda u: u.address)
    covered_ranges = [(cu.address, cu.address + cu.size) for cu in c_units]
    extra: dict[str, int] = {}
    for address, label in symbols.labels.items():   # labels inside C code that other units still reference
        if label not in c_defined and any(lo <= address < hi for lo, hi in covered_ranges):
            extra[label] = address
    provided = set(symbols.labels.values()) | set(symbols.absolute.values()) | c_defined | set(extra)
    known = {}
    for name in (names or {}).values():
        for spelling in (name.name, *name.aliases):
            known[spelling] = name.address
    for cu in c_units:
        for name in sorted(cu.undefined - provided):
            if name in known:
                extra[name] = known[name]
            elif m := re.fullmatch(r"(func|loc|dat|rom|ram)_([0-9a-f]{6})", name):
                extra[name] = int(m.group(2), 16)
            else:
                raise BuildError(f"{cu.source.name}: references {name}, which has no known address")
    return remaining, extra


def build_image(rom: bytes, baserom: Path, catalogue: dict, out: Path, jobs: int, names: dict | None = None,
                sources: list[dict] | None = None) -> dict:
    """Generate, assemble and link every unit into out/; the result is PASS only when byte-identical."""
    started = time.time()
    for sub in ("asm", "obj"):  # units change whenever the catalogue does: never link stale objects
        shutil.rmtree(out / sub, ignore_errors=True)
        (out / sub).mkdir(parents=True)
    units = load_units(rom, catalogue)
    symbols = Symbols(units, catalogue, names)
    pool = ThreadPoolExecutor(max_workers=jobs)
    c_units = list(pool.map(lambda entry: cunits.compile_unit(entry, out), sources or []))
    units, extra = place_c_units(c_units, units, symbols, names)
    code_units = [u for u in units if u.insns]
    raw: set[int] = set()
    for u in code_units:  # first pass names every reference, so labels are fixed before any file is written
        for item in u.insns:
            if isinstance(item, Table):
                for target in item.targets:
                    symbols(target, True)
            else:
                m68k.render(item, symbols)
    starts = sorted(i.address for u in code_units for i in u.insns)
    insn_by_address = {i.address: i for u in code_units for i in u.insns}
    unit_of = {i.address: u for u in code_units for i in u.insns}

    def emit(unit: Unit) -> None:
        source = out / "asm" / f"{unit.name}.s"
        obj = out / "obj" / f"{unit.name}.o"
        for _ in range(MAX_ROUNDS):
            text, line_of = render_unit(unit, symbols, raw, baserom)
            source.write_text(text)
            rejected = assemble(source, obj)
            if not rejected:
                return
            addresses = {line_of[n] for n in rejected if n in line_of} - raw
            if not addresses:
                raise BuildError(f"{source.name}: gas rejected lines {rejected[:5]} that are not rendered instructions")
            raw.update(addresses)  # spelled as raw words from now on
        raise BuildError(f"{source.name}: still rejected after {MAX_ROUNDS} rounds")

    list(pool.map(emit, [u for u in units if u.kind != "c"]))
    rounds = 0
    pending = code_units
    # Object-level fixpoint: sizes first (a mis-sized instruction shifts in-unit displacements), then bytes.
    while pending and rounds < MAX_ROUNDS:
        rounds += 1
        results = list(pool.map(lambda u: (u, check_object(u, out / "obj" / f"{u.name}.o")), pending))
        changed = []
        for unit, (sized, encoded) in results:
            bad = sized or encoded
            if bad:
                raw.update(bad)
                changed.append(unit)
        list(pool.map(emit, changed))
        pending = changed
    image = b""
    link_rounds = 0
    while True:
        link_rounds += 1
        (out / "rom.ld").write_text(linker_script(units, symbols, extra))
        elf, binary = out / "toejam.elf", out / "toejam.bin"
        subprocess.run([str(toolchain.tool(TARGET, "ld")), "-T", "rom.ld", "-Map", "toejam.map", "-o", elf.name],
                       cwd=out, check=True, capture_output=True, text=True)
        subprocess.run([str(toolchain.tool(TARGET, "objcopy")), "-O", "binary", elf.name, binary.name],
                       cwd=out, check=True, capture_output=True, text=True)
        image = binary.read_bytes()
        if image == rom or link_rounds > MAX_ROUNDS:
            break
        diff = [a for a in range(min(len(image), len(rom))) if image[a] != rom[a]]
        bad = set()
        for a in diff:
            k = bisect.bisect_right(starts, a) - 1
            if k >= 0 and starts[k] <= a < starts[k] + insn_by_address[starts[k]].size:
                bad.add(starts[k])
        bad -= raw
        if not bad:
            break  # differences outside rendered instructions: nothing left to fall back
        raw.update(bad)
        list(pool.map(emit, {unit_of[a].name: unit_of[a] for a in bad}.values()))
    pool.shutdown()
    tables = sum(1 for u in code_units for i in u.insns if isinstance(i, Table))
    raw_tables = sum(1 for a in raw if isinstance(insn_by_address[a], Table))
    rendered = len(starts) - tables - (len(raw) - raw_tables)
    by_mnemonic = Counter(insn_by_address[a].mnemonic for a in raw)
    identity = toolchain.identity()["binutils"]["targets"][TARGET]
    receipt = {
        "result": "PASS" if image == rom else "FAIL",
        "expected": {"size": len(rom), "sha256": sha256(rom)},
        "actual": {"size": len(image), "sha256": sha256(image)},
        "inputs": {
            "tools": tool_hashes(),
            "toolchain": {name: identity[name]["sha256"] for name in ("as", "ld", "objcopy")},
            "toolchain_banner": identity["as"]["banner"],
            "as_flags": AS_FLAGS,
            "compiler": {"mcc68k": toolchain.identity().get("mcc68k"), "emu2": (toolchain.identity().get("emu2") or {}).get("sha256"),
                         "as_flags": cunits.AS_FLAGS} if c_units else None,
        },
        "units": {kind: sum(1 for u in units if u.kind == kind) for kind in ("header", "function", "fragment", "data", "c")},
        "c_units": [{"source": str(cu.source.relative_to(ROOT)) if cu.source.is_relative_to(ROOT) else str(cu.source),
                     "sha256": cu.source_sha256, "options": cu.options, "address": f"0x{cu.address:06x}", "size": cu.size,
                     "functions": [name for name, _, _ in cu.functions]} for cu in c_units],
        "instructions": {
            "total": len(starts) - tables,
            "rendered": rendered,
            "raw_words": len(raw) - raw_tables,
            "raw_by_mnemonic": dict(by_mnemonic.most_common()),
            "jump_tables": tables,
            "raw_jump_tables": raw_tables,
        },
        "rounds": {"object": rounds, "link": link_rounds},
        "seconds": round(time.time() - started, 2),
    }
    if image != rom:
        first = next((a for a in range(min(len(image), len(rom))) if image[a] != rom[a]), min(len(image), len(rom)))
        receipt["first_difference"] = f"0x{first:06x}"
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default=romtool.title()["default_version"])
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()
    try:
        receipt = build(args.version, args.jobs)
    except (BuildError, romtool.RomError, namestool.NamesError, toolchain.ToolchainError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", "") or ""
        print(f"error: {error} {detail}".strip(), file=sys.stderr)
        return 1
    ins = receipt["instructions"]
    print(f"{receipt['version']}: {receipt['result']} sha256 {receipt['actual']['sha256']} "
          f"({ins['rendered']}/{ins['total']} instructions as assembly, {ins['raw_words']} raw; "
          f"{receipt['seconds']} s) -> build/{receipt['version']}/receipt.json")
    return 0 if receipt["result"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
