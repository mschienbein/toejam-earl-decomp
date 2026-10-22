"""Compile a C file with MCC68K and check its functions against the original ROM, byte for byte.

    python tools/match.py src/file.c [--version rev02] [--option /OPT ...] [--keep DIR]

Every function the file defines must be named `func_XXXXXX` or carry its imported name
(config/<version>/names.json), and the file's functions must be one contiguous run of catalogued
functions in ROM order (they are emitted and linked together, as a translation unit). The object is
linked at the first function's address with every other symbol resolved to its original address
(catalogue functions, labels and data from the current build's ELF, imported names), so call and
data relocations are checked as final bytes: the comparison is strict, not relocation-lenient.

Exit status 0 only when every function in the file matches; a side-by-side disassembly shows the
first differences otherwise.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from elftools.elf.elffile import ELFFile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import m68k  # noqa: E402
import mcc  # noqa: E402
import names as namestool  # noqa: E402
import rom as romtool  # noqa: E402
import toolchain  # noqa: E402
import revmap  # noqa: E402
import tus  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]


class MatchError(RuntimeError):
    pass


def symbol_table(version: str) -> dict[str, int]:
    """The authoritative names: config/<version>/names.json, with REV 02 identities in other revisions.

    Nothing else (no build output, no command-line definition) can give a known name an address;
    unnamed places are spelled by their REV 02 address and translated (`resolve`).
    """
    table: dict[str, int] = {}
    for name in namestool.load(version).values():
        for spelling in (name.name, *name.aliases):   # aliases resolve (the runtime's entries); authors write primaries
            table[spelling] = name.address
    return table


def aliases(version: str) -> dict[str, str]:
    """Alias -> primary name: C writes the primary name of every place."""
    return {alias: name.name for name in namestool.load(version).values() for alias in name.aliases}


def defined_functions(obj: Path, unit: str) -> list[tuple[str, int, int]]:
    """(name, offset, size-until-next) for each global function in the unit's code section."""
    with obj.open("rb") as stream:
        elf = ELFFile(stream)
        section = elf.get_section_by_name(f".text.{unit}")
        if section is None:
            raise MatchError("the file defines no code")
        index = elf.get_section_index(f".text.{unit}")
        size = section["sh_size"]
        starts = sorted((s["st_value"], s.name) for s in elf.get_section_by_name(".symtab").iter_symbols()
                        if s["st_shndx"] == index and s["st_info"]["bind"] == "STB_GLOBAL")
    return [(name, offset, (starts[k + 1][0] if k + 1 < len(starts) else size) - offset)
            for k, (offset, name) in enumerate(starts)]


def undefined_symbols(obj: Path) -> list[str]:
    with obj.open("rb") as stream:
        return sorted({s.name for s in ELFFile(stream).get_section_by_name(".symtab").iter_symbols()
                       if s.name and s["st_shndx"] == "SHN_UNDEF"})


ADDRESSED = re.compile(r"^(func|frag|loc|dat|data|jtbl|header|rom|ram)_([0-9a-f]{6})$")


def resolve(name: str, table: dict[str, int], translate=lambda address: address) -> int:
    """A referenced symbol's original address: a known name, or an address-spelled name.

    Address spellings are primary-revision (REV 02) addresses; `translate` (tools/revmap.py) moves
    them to the revision being matched.
    """
    if name in table:
        return table[name]
    m = ADDRESSED.match(name)
    if m:
        address = int(m.group(2), 16)
        if (m.group(1) == "ram") != (address >= 0xFF0000):
            raise MatchError(f"{name}: ram_ names RAM (0xFF0000-0xFFFFFF), the others ROM")
        moved = translate(address)
        if moved is None:
            raise MatchError(f"{name}: no corresponding place in this revision")
        named = next((n for n, a in table.items() if a == moved and not ADDRESSED.match(n)), None)
        if named:
            raise MatchError(f"{name} has a name: write {named}")
        return moved
    raise MatchError(f"{name} is referenced but has no known address (add it to names.json, or spell it ram_XXXXXX/dat_XXXXXX)")


def side_by_side(expected: bytes, actual: bytes, address: int, rows: int = 12) -> str:
    def listing(data: bytes) -> list[str]:
        image = bytes(address) + data + bytes(10)   # decode at the real address so targets read naturally
        out, pc = [], 0
        while pc < len(data):
            insn = m68k.decode_one(image, address + pc)
            if insn is None:
                out.append(f"{address + pc:06x}  .short 0x{int.from_bytes(data[pc:pc + 2], 'big'):04x}")
                pc += 2
                continue
            out.append(f"{address + pc:06x}  {insn.mnemonic} {insn.op_str}")
            pc += insn.size
        return out
    left, right = listing(expected), listing(actual)
    first = next((k for k, (a, b) in enumerate(zip(left, right)) if a != b), min(len(left), len(right)))
    lo = max(0, first - 2)
    lines = [f"{'original':<44} candidate"]
    for k in range(lo, min(max(len(left), len(right)), lo + rows)):
        a = left[k] if k < len(left) else ""
        b = right[k] if k < len(right) else ""
        lines.append(f"{'  ' if a == b else '> '}{a:<42} {b}")
    return "\n".join(lines)


def match_file(source: Path, version: str, options: list[str], keep: Path | None = None,
               defines: dict[str, int] | None = None) -> list[dict]:
    rom = romtool.load(version)
    catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text())
    extents = {f["address"]: f["size"] for f in catalogue["functions"]}
    table = symbol_table(version)
    alias_of = aliases(version)
    translate = revmap.translator(version)
    for name, address in (defines or {}).items():   # experiments may add unknown names, never move known ones
        if name in table or ADDRESSED.match(name):
            raise MatchError(f"--define {name}: the name already has an authoritative address")
        table[name] = address
    unit = mcc.dos_name(source.stem).lower()
    def address_of(name: str) -> int | None:
        try:
            return resolve(name, table, translate)
        except MatchError:
            return None

    compiled = mcc.compile_to_gas(source, unit, [ROOT / "include"], options, address_of, tus.load(version))
    with tempfile.TemporaryDirectory(prefix="match-") as tmp:
        work = keep or Path(tmp)
        work.mkdir(parents=True, exist_ok=True)
        (work / f"{unit}.SRC").write_text(compiled.listing, encoding="latin-1")
        obj = work / f"{unit}.o"
        mcc.assemble(compiled.gas, source, obj, unit)
        functions = defined_functions(obj, unit)
        spelled = [n for n, _, _ in functions] + undefined_symbols(obj)
        for name in spelled:
            if name in alias_of and name not in mcc.RUNTIME_ENTRIES:
                raise MatchError(f"{name} is an alias: write {alias_of[name]}")
        homes = {}
        for name, _, _ in functions:
            if name in (defines or {}):
                raise MatchError(f"{name}: a defined function takes its address from names.json or its spelling, not --define")
            home = resolve(name, table, translate)
            if home not in extents:
                raise MatchError(f"{name} (0x{home:06x}) is not a catalogued function start")
            homes[name] = home
        base = homes[functions[0][0]]
        for name, offset, _ in functions:
            if homes[name] != base + offset:
                raise MatchError(f"{name}: in the file it would be at 0x{base + offset:06x}, but the original is at "
                                 f"0x{homes[name]:06x} (functions must be a contiguous run in ROM order)")
        script = ["SECTIONS {", f"  .text 0x{base:x} : {{ *(.text.{unit}) }}", "  .data : { *(.data) }", "}"]
        script += [f"{n} = 0x{resolve(n, table, translate):x};" for n in undefined_symbols(obj)]
        (work / "match.ld").write_text("\n".join(script) + "\n")
        elf = work / f"{unit}.elf"
        result = subprocess.run([str(toolchain.tool("m68k-elf", "ld")), "-T", str(work / "match.ld"), "-o", str(elf), str(obj)],
                                capture_output=True, text=True)
        if result.returncode:
            raise MatchError(f"link failed:\n{result.stderr}")
        with elf.open("rb") as stream:
            text = ELFFile(stream).get_section_by_name(".text").data()
    reports = []
    for name, offset, size in functions:
        address = base + offset
        expected_size = extents.get(address)
        actual = text[offset:offset + size]
        expected = rom[address:address + (expected_size or size)]
        ok = expected_size == size and actual == expected
        report = {"function": name, "address": f"0x{address:06x}", "size": size, "expected_size": expected_size,
                  "status": "MATCH" if ok else "DIFF"}
        if not ok:
            report["diff"] = side_by_side(expected, actual, address)
        reports.append(report)
    return reports


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path)
    parser.add_argument("--version", default="rev02")
    parser.add_argument("--option", action="append", default=[])
    parser.add_argument("--keep", type=Path, help="keep the compiler output, object and link script here")
    parser.add_argument("--define", action="append", default=[], metavar="NAME=ADDRESS",
                        help="extra symbol, e.g. lmodt=0x027154 (experiments; real names belong in names.json)")
    args = parser.parse_args()
    defines = {d.split("=", 1)[0]: int(d.split("=", 1)[1], 0) for d in args.define}
    try:
        reports = match_file(args.source, args.version, args.option, args.keep, defines)
    except (MatchError, mcc.CompileError, mcc.AdaptError, romtool.RomError, toolchain.ToolchainError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for r in reports:
        extra = "" if r["status"] == "MATCH" else f" (size {r['size']}, original {r['expected_size']})"
        print(f"{r['status']:5} {r['function']} @ {r['address']}{extra}")
        if "diff" in r:
            print(r["diff"])
    return 0 if all(r["status"] == "MATCH" for r in reports) else 1


if __name__ == "__main__":
    sys.exit(main())
