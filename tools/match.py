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

ROOT = Path(__file__).resolve().parents[1]
AS_FLAGS = ["-m68000", "--register-prefix-optional"]  # externals: absolute long, as ASM68K emits them


class MatchError(RuntimeError):
    pass


def symbol_table(version: str) -> dict[str, int]:
    """Every name with a known original address: build ELF symbols, catalogue functions, imported names."""
    table: dict[str, int] = {}
    elf_path = ROOT / "build" / version / "toejam.elf"
    if elf_path.is_file():
        with elf_path.open("rb") as stream:
            for sym in ELFFile(stream).get_section_by_name(".symtab").iter_symbols():
                if sym.name and not sym.name.startswith(".L") and sym["st_info"]["type"] != "STT_SECTION":
                    table[sym.name] = sym["st_value"]
    catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text())
    for f in catalogue["functions"]:
        table.setdefault(f"func_{f['address']:06x}", f["address"])
    for name in namestool.load(version).values():
        table[name.name] = name.address
    return table


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


ADDRESSED = re.compile(r"^(func|loc|dat|rom|ram)_([0-9a-f]{6})$")


def resolve(name: str, table: dict[str, int]) -> int:
    """A referenced symbol's original address: a known name, or an address-spelled name."""
    if name in table:
        return table[name]
    m = ADDRESSED.match(name)
    if m:
        address = int(m.group(2), 16)
        if (m.group(1) == "ram") != (address >= 0xFF0000):
            raise MatchError(f"{name}: ram_ names RAM (0xFF0000-0xFFFFFF), the others ROM")
        return address
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
    table = {**symbol_table(version), **(defines or {})}
    unit = mcc.dos_name(source.stem).lower()
    compiled = mcc.compile_to_gas(source, unit, [ROOT / "include"], options)
    with tempfile.TemporaryDirectory(prefix="match-") as tmp:
        work = keep or Path(tmp)
        work.mkdir(parents=True, exist_ok=True)
        (work / f"{unit}.SRC").write_text(compiled.listing, encoding="latin-1")
        (work / f"{unit}.s").write_text(compiled.gas)
        obj = work / f"{unit}.o"
        result = subprocess.run([str(toolchain.tool("m68k-elf", "as")), *AS_FLAGS, "-o", str(obj), str(work / f"{unit}.s")],
                                capture_output=True, text=True)
        if result.returncode:
            raise MatchError(f"assembler rejected the compiler output:\n{result.stderr}")
        functions = defined_functions(obj, unit)
        for name, _, _ in functions:
            if name not in table:
                raise MatchError(f"{name} has no original address (use func_XXXXXX or an imported name)")
        base = table[functions[0][0]]
        for name, offset, _ in functions:
            if table[name] != base + offset:
                raise MatchError(f"{name}: in the file it would be at 0x{base + offset:06x}, but the original is at "
                                 f"0x{table[name]:06x} (functions must be a contiguous run in ROM order)")
        script = ["SECTIONS {", f"  .text 0x{base:x} : {{ *(.text.{unit}) }}", "  .data : { *(.data) }", "}"]
        script += [f"{n} = 0x{resolve(n, table):x};" for n in undefined_symbols(obj)]
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
    except (MatchError, mcc.CompileError, romtool.RomError, toolchain.ToolchainError) as error:
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
