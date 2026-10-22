"""Compare a private original function with a symbol in a big-endian m68k ELF object.

    python tools/diff.py func_XXXXXX CANDIDATE.o [--symbol NAME] [--version rev02]

Non-relocated bytes must match exactly. Relocated operands must name a resolvable original
symbol/addend, and must encode exactly at the original address; unknown/unsupported relocations
fail closed. This is an author aid, not a full-ROM acceptance receipt.
"""
from __future__ import annotations

import argparse
import difflib
import re
import sys
from dataclasses import dataclass
from pathlib import Path

from elftools.common.exceptions import ELFError
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

sys.path.insert(0, str(Path(__file__).resolve().parent))
import packet  # noqa: E402
import revmap  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
# ELF for the Motorola 68000 family, scalar absolute and PC-relative relocations.
RELOCATIONS = {1: (4, False, "R_68K_32"), 2: (2, False, "R_68K_16"), 3: (1, False, "R_68K_8"),
               4: (4, True, "R_68K_PC32"), 5: (2, True, "R_68K_PC16"), 6: (1, True, "R_68K_PC8")}


class DiffError(RuntimeError):
    pass


@dataclass
class Candidate:
    name: str
    data: bytes
    offset: int
    section_index: int
    symbols: list[dict]
    relocations: list[dict]


def read_candidate(path: Path, wanted: str) -> Candidate:
    # Keep the stream alive only during extraction; callers get immutable section/symbol facts.
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        if elf["e_machine"] != "EM_68K" or elf.elfclass != 32 or elf.little_endian or elf["e_type"] != "ET_REL":
            raise DiffError("candidate must be a big-endian ELF32 m68k relocatable object")
        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            raise DiffError("candidate has no .symtab")
        symbols = [{"name": s.name, "value": s["st_value"], "size": s["st_size"],
                    "section": s["st_shndx"], "type": s["st_info"]["type"], "bind": s["st_info"]["bind"]}
                   for s in symtab.iter_symbols()]
        found = [s for s in symbols if s["name"] == wanted and isinstance(s["section"], int)]
        if len(found) != 1:
            raise DiffError(f"expected one defined symbol {wanted!r}; use --symbol NAME")
        symbol = found[0]
        section = elf.get_section(symbol["section"])
        if not section["sh_flags"] & 4 or section["sh_type"] == "SHT_NOBITS":
            raise DiffError(f"{wanted}: symbol is not in an executable, byte-backed section")
        start, size = symbol["value"], symbol["size"]
        data = section.data()
        if not size:
            # GNU assembly symbols often have no .size; internal labels must not truncate them.
            # MCC68K output (tools/mcc.py) marks functions only as global labels, not STT_FUNC.
            next_functions = [s["value"] for s in symbols if s["section"] == symbol["section"]
                              and (s["type"] == "STT_FUNC" or s.get("bind") == "STB_GLOBAL") and s["value"] > start]
            size = min(next_functions, default=len(data)) - start
        if start < 0 or size <= 0 or start + size > len(data):
            raise DiffError(f"{wanted}: invalid symbol extent")
        relocations = []
        for relsec in elf.iter_sections():
            if not isinstance(relsec, RelocationSection) or relsec["sh_info"] != symbol["section"]:
                continue
            linked_symbols = elf.get_section(relsec["sh_link"])
            for rel in relsec.iter_relocations():
                offset, kind = rel["r_offset"], rel["r_info_type"]
                if kind == 0:  # R_68K_NONE occupies no bytes.
                    continue
                width = RELOCATIONS.get(kind, (1, False, ""))[0]
                if offset < start + size and offset + width > start:
                    if kind not in RELOCATIONS:
                        raise DiffError(f"unsupported m68k relocation type {kind} at section offset 0x{offset:x}")
                    if offset < start or offset + width > start + size:
                        raise DiffError("relocation straddles the candidate symbol boundary")
                    target = linked_symbols.get_symbol(rel["r_info_sym"])
                    addend = rel["r_addend"] if relsec.is_RELA() else int.from_bytes(data[offset:offset + width], "big", signed=True)
                    relocations.append({"offset": offset - start, "kind": kind, "addend": addend,
                                        "symbol": {"name": target.name, "value": target["st_value"],
                                                   "size": target["st_size"], "section": target["st_shndx"],
                                                   "type": target["st_info"]["type"]}})
        return Candidate(wanted, data[start:start + size], start, symbol["section"], symbols, relocations)


def original_symbol_address(name: str, symbols: packet.Symbols) -> int | None:
    translate = getattr(symbols, "translate", None)
    spelled = re.fullmatch(r"(?:func|frag|loc|data|dat|jtbl|rom|ram)_([0-9a-fA-F]{6})", name)
    if translate is not None and spelled and name not in symbols.canonical:
        return translate(int(spelled[1], 16))   # other revisions: address spellings are REV 02 addresses
    if name in symbols.by_name:
        return symbols.by_name[name]
    # Generated absolute labels have address-bearing names, but only accept exact metadata labels
    # or a location inside a known extent, not a misspelled arbitrary symbol that happens to parse.
    match = re.fullmatch(r"(?:func|frag|loc|data|dat|rom|jtbl|ram)_([0-9a-fA-F]{6})", name)
    if match:
        address = int(match[1], 16)
        if 0xFF0000 <= address <= 0xFFFFFF:
            return address
        owner, _ = symbols.identify(address)
        if not owner.startswith(("addr_", "ram_")):   # inside a catalogued unit or a named data unit
            return address
    return None


def target_expression(program: packet.Program, address: int) -> dict:
    """Canonical original anchor; local code labels are offsets within their function."""
    for start, table in program.tables.items():
        if start <= address < start + 2 * table["count"]:
            return {"symbol": program.symbols.name(start), "addend": address - start}
    owner = program.owner(address)
    if owner is not None:
        return {"symbol": program.symbols.name(owner["address"]), "addend": address - owner["address"]}
    name, offset = program.symbols.identify(address)
    return {"symbol": name, "addend": offset}


def definition_address(candidate: Candidate, symbol: dict, original_address: int) -> int:
    section = symbol["section"]
    if section == "SHN_ABS":
        return symbol["value"]
    if section == candidate.section_index:
        return original_address + symbol["value"] - candidate.offset
    raise DiffError(f"cannot place candidate-defined symbol {symbol['name']!r} from section {section!r} at an original address")


def validate_definition(candidate: Candidate, symbol: dict, symbols: packet.Symbols, original_address: int) -> int:
    actual = definition_address(candidate, symbol, original_address)
    expected = original_symbol_address(symbol["name"], symbols)
    if expected is not None and actual != expected:
        raise DiffError(f"candidate definition of {symbol['name']!r} contradicts original metadata: "
                        f"ELF maps to 0x{actual:x}, expected 0x{expected:x}")
    return actual


def relocation_expression(program: packet.Program, candidate: Candidate, rel: dict,
                          original_address: int, bias: int) -> tuple[dict, int]:
    target, addend = rel["symbol"], rel["addend"] + bias
    known = original_symbol_address(target["name"], program.symbols)
    if target["type"] == "STT_SECTION" and (target["section"] != candidate.section_index or target["value"] != 0):
        raise DiffError("relocation does not reference a genuine zero-offset current-section anchor")
    if target["section"] == "SHN_UNDEF":
        if known is None:
            raise DiffError(f"unresolved original address for relocation symbol {target['name']!r}; add a provenance-backed names.json entry")
        base = known
    else:
        base = validate_definition(candidate, target, program.symbols, original_address)
    address = base + addend
    if known is None and target["type"] == "STT_SECTION":
        # A section relocation has no named function/data identity. Its actual section offset is
        # normalized through the placement anchored by the selected function, not a guessed name.
        return target_expression(program, address), address
    # Do NOT re-identify base+addend: a different named function plus a compensating addend must
    # retain that different identity even when its final numerical value happens to be equal.
    expression = target_expression(program, base)
    expression["addend"] += addend
    return expression, address


def containing_instruction(program: packet.Program, unit: dict, offset: int):
    address = unit["address"] + offset
    for insn in program.by_unit[unit["address"]]:
        if insn.address <= address < insn.address + insn.size:
            return insn
    return None


def compare(program: packet.Program, unit: dict, candidate: Candidate) -> dict:
    address, size = unit["address"], unit["size"]
    original = program.rom[address:address + size]
    actual = bytearray(candidate.data)
    masked = set()
    relocations = []
    errors = []
    for symbol in candidate.symbols:
        if symbol["section"] != "SHN_UNDEF" and original_symbol_address(symbol["name"], program.symbols) is not None:
            try:
                validate_definition(candidate, symbol, program.symbols, address)
            except DiffError as error:
                errors.append(str(error))
    for rel in sorted(candidate.relocations, key=lambda r: r["offset"]):
        offset = rel["offset"]
        width, pc_relative, kind_name = RELOCATIONS[rel["kind"]]
        field = set(range(offset, offset + width))
        if field & masked:
            raise DiffError("overlapping relocations in candidate symbol")
        masked |= field
        if offset + width > len(original):
            errors.append(f"{kind_name} at +0x{offset:x} is outside the original function")
            continue
        insn = containing_instruction(program, unit, offset)
        # CPU PC-relative operands use instruction+2; ELF uses the relocation field address.
        # In particular R_68K_PC8 branch addends include -1, which must not become an off-by-one
        # symbol difference. Table fields use their own ELF place unless an instruction owns them.
        bias = (insn.address + 2 - (address + offset)) if pc_relative and insn is not None else 0
        value = int.from_bytes(original[offset:offset + width], "big", signed=pc_relative)
        expected_target = value + address + offset + bias if pc_relative else value
        if not pc_relative and width == 2 and insn is not None:
            if any(0xFF0000 <= target <= 0xFFFFFF for target in insn.targets) and value & 0x8000:
                expected_target = value | 0xFF0000
        expected_expression = target_expression(program, expected_target)
        record = {"offset": offset, "width": width, "type": kind_name,
                  "symbol": rel["symbol"]["name"], "elf_addend": rel["addend"],
                  "expected": expected_expression, "actual": None, "match": False}
        try:
            expression, target = relocation_expression(program, candidate, rel, address, bias)
            record["actual"] = expression
            record["match"] = expression == expected_expression
            encoded = target - bias - (address + offset) if pc_relative else target
            if pc_relative and not -(1 << (width * 8 - 1)) <= encoded < 1 << (width * 8 - 1):
                record["match"] = False
                errors.append(f"{kind_name} at +0x{offset:x} overflows its signed field")
            actual[offset:offset + width] = (encoded & ((1 << (width * 8)) - 1)).to_bytes(width, "big")
            record["match"] = record["match"] and actual[offset:offset + width] == original[offset:offset + width]
        except DiffError as error:
            if str(error) not in errors:
                errors.append(str(error))
        relocations.append(record)
    byte_differences = [offset for offset in range(min(len(original), len(actual)))
                        if offset not in masked and original[offset] != actual[offset]]
    exact = len(original) == len(actual) and not byte_differences and not errors and all(r["match"] for r in relocations)
    return {"match": exact, "original": original, "candidate": bytes(actual), "candidate_unlinked": candidate.data,
            "relocations": relocations, "byte_differences": byte_differences, "errors": errors,
            "address": address, "original_size": size, "candidate_size": len(actual)}


def instruction_rows(program: packet.Program, unit: dict, data: bytes, relocations: list[dict], side: str) -> list[tuple[str, str]]:
    address = unit["address"]
    # decode_one indexes a full image; retain only the small prefix needed to give it ROM addresses.
    image = bytes(address) + data
    rows = []
    offset = 0
    table_sizes = {start - address: table["count"] * 2 for start, table in program.tables.items()
                   if address <= start < address + unit["size"]}
    while offset < len(data):
        if offset in table_sizes:
            count = min(table_sizes[offset], len(data) - offset)
            raw = data[offset:offset + count]
            text = ".short " + ", ".join(f"0x{int.from_bytes(raw[k:k + 2], 'big'):04x}" for k in range(0, len(raw), 2))
        else:
            insn = program.m68k.decode_one(image, address + offset)
            if insn is not None and offset + insn.size <= len(data):
                count, raw, text = insn.size, insn.raw, program.assembly(insn)
            else:
                count = min(2, len(data) - offset)
                raw = data[offset:offset + count]
                text = ".byte " + ",".join(f"0x{b:02x}" for b in raw)
        annotations = []
        for rel in relocations:
            if offset <= rel["offset"] < offset + count:
                expression = rel["expected"] if side == "original" else rel["actual"]
                expression_text = "UNRESOLVED" if expression is None else f"{expression['symbol']}{expression['addend']:+d}"
                annotations.append(f"{rel['type']} {expression_text}")
        if annotations:
            text += " ; " + ", ".join(annotations)
        display = f"{address + offset:06x}: {raw.hex(' '):<29} {text}"
        # Keep byte differences visible even when aliases/rendering happen to spell the same text.
        rows.append((raw.hex() + " " + text, display))
        offset += count
    return rows


def side_by_side(program: packet.Program, unit: dict, result: dict) -> str:
    left = instruction_rows(program, unit, result["original"], result["relocations"], "original")
    right = instruction_rows(program, unit, result["candidate"], result["relocations"], "candidate")
    lines = [f"{'ORIGINAL':<88} | CANDIDATE (relocations resolved at original address)"]
    matcher = difflib.SequenceMatcher(a=[r[0] for r in left], b=[r[0] for r in right], autojunk=False)
    for kind, a, b, c, d in matcher.get_opcodes():
        for index in range(max(b - a, d - c)):
            ltext = left[a + index][1] if a + index < b else ""
            rtext = right[c + index][1] if c + index < d else ""
            marker = "=" if kind == "equal" else "!" if ltext and rtext else "<" if ltext else ">"
            lines.append(f"{ltext:<88} {marker} {rtext}")
    lines += [f"relocation +0x{r['offset']:x}: {r['type']} {r['symbol']!r} addend {r['elf_addend']:+d}: "
              f"{'MATCH' if r['match'] else 'DIFF'} (expected {r['expected']}, actual {r['actual']})" for r in result["relocations"]]
    lines += ["error: " + error for error in result["errors"]]
    lines.append(f"{'MATCH' if result['match'] else 'DIFFER'}: original {result['original_size']} bytes, "
                 f"candidate {result['candidate_size']} bytes; {len(result['byte_differences'])} non-relocated byte differences; "
                 f"{sum(not r['match'] for r in result['relocations'])} relocation differences")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("unit")
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--symbol")
    parser.add_argument("--version", default="rev02")
    parser.add_argument("--repo", type=Path, default=ROOT)
    args = parser.parse_args(argv)
    try:
        rom, catalogue, symbols = packet.load_inputs(args.repo, args.version)
        if args.version != revmap.PRIMARY:
            symbols.translate = revmap.translator(args.version, args.repo)
        program = packet.Program(rom, catalogue, symbols, packet.decoder(args.repo))
        unit = program.select(args.unit)
        candidate = read_candidate(args.candidate, args.symbol or args.unit)
        result = compare(program, unit, candidate)
        print(side_by_side(program, unit, result), end="")
        return 0 if result["match"] else 1
    except (DiffError, packet.PacketError, OSError, ValueError, KeyError, ELFError) as error:
        print(f"diff: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
