"""Compile C with Microtec MCC68K 3.3K (under emu2) and turn its assembly into GNU as input.

    python tools/mcc.py src/file.c [--out DIR] [--option OPT ...]   # writes FILE.SRC and file.s

The compiler runs in a private DOS drive (C:\\MCC68K\\MCC68K.EXE, C:\\WORK\\<8.3 name>.C, header
directories copied in). Its output is Microtec assembler syntax; `adapt()` rewrites only what GNU as
spells differently and never changes an instruction:

- sections: `SECTION 9` (code) -> `.section .text.<unit>`; any other section -> `.data`;
  `XDEF` -> `.globl`; `XREF`, `OPT`, `IDNT`, `END` and comments dropped;
- Microtec prefixes C identifiers with a dot (`.name`); the dot is removed so C names are the
  project's symbol names (`func_0011ca`, imported names, RAM globals);
- `$hex` -> `0xhex`;
- sizing, reproducing ASM68K 6.6A as measured (CompilerHunter receipts asm66-branch-rules.json,
  asm66-call-option-results.json) and as the ROM shows (compiled code: 386 backward PC-relative
  calls without NOP, 72 forward PC-relative calls followed by NOP, none forward without NOP):
  - an unsized Bcc/BRA/BSR to a label not yet defined (forward) gets a 16-bit displacement (`.w`);
    to a label already defined (backward) the shortest form (`jb<cc>`: byte when it fits, -128 too);
  - a bare `JSR`/`JMP` to a label in the same code section becomes PC-relative: 4 bytes backward;
    forward the assembler keeps the 6 bytes it reserved, as PC-relative plus `NOP`; any other target
    (another C file, RAM, the runtime) stays absolute long.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import toolchain  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
AS_FLAGS = ["-m68000"]   # registers are spelled %d0 by adapt(); bare words are always symbols
# Entry points of the MCC68K 3.3K runtime that the compiler itself calls (docs/COMPILER.md); C never
# spells them, so their alias names (two entries share a body) are not an author's choice.
RUNTIME_ENTRIES = frozenset({"abs", "lmodt", "ldivt", "lmult", "ulmult", "ulmodt", "uldivt"})
CODE_SECTION = 9         # MCC68K's code section number
PC16_REACH = 32767
REGISTER = re.compile(r"^(?:[DA][0-7]|SP|PC|SR|CCR|USP)$", re.I)
BRANCH = re.compile(r"^B(?:RA|SR|HI|LS|CC|HS|CS|LO|NE|EQ|VC|VS|PL|MI|GE|LT|GT|LE)$", re.I)
DBCC = re.compile(r"^DB(?:RA|T|F|HI|LS|CC|HS|CS|LO|NE|EQ|VC|VS|PL|MI|GE|LT|GT|LE)$", re.I)
CALL = re.compile(r"^(?:JSR|JMP)$", re.I)
TOKEN = re.compile(r"'(?:[^']|'')*'|\$[0-9A-Fa-f]+|\.?[A-Za-z_][A-Za-z0-9_]*|\s+|.", re.S)
LABEL = re.compile(r"^(\.?[A-Za-z_]\w*):(.*)$")


class AdaptError(ValueError):
    pass


@dataclass
class Statement:
    label: str | None     # label defined on this line (dot removed)
    mnemonic: str         # as written, e.g. MOVE.W; "" for a label-only line
    operands: str         # operand field as written (Microtec spelling), comment removed


def split_fields(text: str) -> tuple[str, str]:
    """(mnemonic, operand field) of a statement; Microtec ends the operand field at the first unquoted blank."""
    text = text.strip()
    if not text:
        return "", ""
    parts = text.split(None, 1)
    mnemonic, rest = parts[0], parts[1] if len(parts) > 1 else ""
    if not rest or rest.startswith("*") or rest.startswith(";"):
        return mnemonic, ""
    field, quoted = [], False
    for ch in rest:
        if ch == "'":
            quoted = not quoted   # a doubled '' toggles twice: still inside the string
        elif ch.isspace() and not quoted:
            break
        field.append(ch)
    if quoted:
        raise AdaptError(f"unterminated string in: {text}")
    return mnemonic, "".join(field)


def parse(listing: str) -> list[tuple[int, Statement]]:
    """Statements with the section number they are in (comments and blank lines dropped)."""
    statements, section = [], None
    for raw in listing.splitlines():
        if not raw.strip() or raw.lstrip().startswith("*"):
            continue
        label = None
        body = raw
        if not raw[:1].isspace():
            match = LABEL.match(raw.strip())
            if match:
                label, body = match.group(1).lstrip("."), match.group(2)
            else:   # a column-1 word without a colon: Microtec label syntax, or a directive line like "UNIT IDNT"
                head, _, rest = raw.partition(" ") if " " in raw else raw.partition("\t")
                if head.upper() in ("SECTION", "XDEF", "XREF", "OPT", "END"):
                    body = raw
                else:
                    label, body = head.lstrip("."), rest
        mnemonic, operands = split_fields(body)
        if mnemonic.upper() == "SECTION":
            number = re.match(r"(\d+)", operands)
            if not number:
                raise AdaptError(f"unrecognised SECTION: {raw}")
            section = int(number.group(1))
        elif mnemonic.upper() == "IDNT":
            label = None   # "UNIT IDNT": a module name, not a label
        statements.append((section, Statement(label, mnemonic, operands)))
    return statements


def gas_operands(operands: str) -> str:
    """Microtec operand spelling -> GNU as: %registers, 0xhex, C symbols without their dot."""
    out = []
    for token in TOKEN.findall(operands):
        if token.startswith("'"):
            raise AdaptError(f"a string operand outside DC.B: {operands}")
        if token.startswith("$"):
            out.append("0x" + token[1:])
        elif token.startswith("."):
            previous = out[-1] if out else ""
            if previous and (previous[-1:].isalnum() or previous[-1:] in "_)"):
                out.append(token)            # a size suffix such as D1.L or (x).L
            else:
                out.append(token[1:])        # a C symbol
        elif REGISTER.match(token):
            out.append("%" + token.lower())
        else:
            out.append(token)
    return "".join(out)


def dc_bytes(operands: str) -> str:
    """DC.B operands with Microtec strings -> a .byte list holding exactly the same bytes."""
    values = []
    for item in re.findall(r"'(?:[^']|'')*'|[^,]+", operands):
        if item.startswith("'"):
            values += [str(b) for b in item[1:-1].replace("''", "'").encode("latin-1")]
        elif item.strip():
            values.append(gas_operands(item.strip()))
    return "\t.byte\t" + ",".join(values)


def code_labels(listing: str) -> set[str]:
    """Labels defined in the code section, with the Microtec dot removed."""
    return {st.label for section, st in parse(listing) if st.label and section == CODE_SECTION}


def call_target(statement: Statement) -> str | None:
    """The bare symbol a JSR/JMP names (None for any other addressing mode)."""
    if CALL.match(statement.mnemonic) and re.fullmatch(r"\.?[A-Za-z_]\w*", statement.operands) \
            and not REGISTER.match(statement.operands):
        return statement.operands.lstrip(".")
    return None


def external_calls(listing: str) -> list[str]:
    """JSR/JMP targets that the code section does not define (other files, or the rest of this file's original unit)."""
    local, seen = code_labels(listing), []
    for _, statement in parse(listing):
        target = call_target(statement)
        if target and target not in local and target not in seen:
            seen.append(target)
    return seen


def first_function(listing: str) -> str | None:
    """The first non-local label of the code section (the unit's lowest address)."""
    for section, statement in parse(listing):
        if section == CODE_SECTION and statement.label and not statement.label.startswith("_L"):
            return statement.label
    return None


def adapt(listing: str, unit: str, near: dict[str, str] | None = None) -> str:
    """Microtec assembly -> GNU as assembly (directives, spellings and ASM68K sizing only).

    `near` names external call targets that sit in the same original translation unit as this C
    file ("backward" before it, "forward" after it, both within PC-relative reach): a C file holding
    only part of an original unit calls the rest the way the original assembler did.
    """
    near = near or {}
    lines: list[str] = []
    defined: set[str] = set()      # code-section labels seen so far
    local = code_labels(listing)
    for section, statement in parse(listing):
        word = statement.mnemonic.upper()
        if statement.label and word != "IDNT":
            if section == CODE_SECTION:
                defined.add(statement.label)
            lines.append(f"{statement.label}:")
        if not word or word in ("OPT", "END", "XREF", "IDNT"):
            continue
        if word == "SECTION":
            lines.append(f'\t.section .text.{unit},"ax"' if section == CODE_SECTION else "\t.data")
            continue
        if word == "XDEF":
            lines.append("\t.globl " + ", ".join(n.strip().lstrip(".") for n in statement.operands.split(",")))
            continue
        if word == "DC.B" and "'" in statement.operands:
            lines.append(dc_bytes(statement.operands))
            continue
        operands = statement.operands
        if BRANCH.match(word):
            target = operands.lstrip(".")
            if section != CODE_SECTION or target not in local:
                raise AdaptError(f"{statement.mnemonic} {operands}: branch outside this file's code")
            if target in defined:   # backward: shortest form (gas jbra/jbsr/jb<cc> relax to byte when it fits)
                lines.append(f"\tjb{word[1:].lower()}\t{target}")
            else:                   # forward: the one-pass assembler reserved a word displacement
                lines.append(f"\t{statement.mnemonic}.w\t{target}")
            continue
        if DBCC.match(word) and operands.split(",")[-1].lstrip(".") not in local:
            raise AdaptError(f"{statement.mnemonic} {operands}: DBcc outside this file's code")
        target = call_target(statement)
        if target:
            if target in local and target in defined:
                lines.append(f"\t{statement.mnemonic}\t({target},%pc)")
            elif target in local or near.get(target) == "forward":
                lines.append(f"\t{statement.mnemonic}\t({target},%pc)")
                lines.append("\tnop")   # forward: PC-relative inside the 6 bytes reserved for absolute long
            elif near.get(target) == "backward":
                lines.append(f"\t{statement.mnemonic}\t({target},%pc)")
            else:
                lines.append(f"\t{statement.mnemonic}\t({target}).l")
            continue
        lines.append(f"\t{statement.mnemonic}\t{gas_operands(operands)}" if operands else f"\t{statement.mnemonic}")
    return "\n".join(lines) + "\n"


class CompileError(RuntimeError):
    pass


@dataclass
class Compiled:
    source: Path          # the C file compiled
    listing: str          # the compiler's assembly output (Microtec syntax)
    gas: str              # adapted GNU as text


def dos_name(stem: str) -> str:
    """An 8.3-safe upper-case DOS file stem."""
    clean = re.sub(r"[^A-Za-z0-9]", "", stem).upper()[:8]
    return clean or "UNIT"


def revision_header(version: str) -> str:
    """revision.h for one build: `#if REVISION >= 2` marks code that only REV 02 has."""
    number = re.fullmatch(r"rev(\d+)", version)
    if not number:
        raise CompileError(f"{version}: not a revision name")
    return f"/* Generated per build by tools/mcc.py: which revision this compile is for. */\n#define REVISION {int(number.group(1))}\n"


def compile_c(source: Path, include_dirs: list[Path] = (), options: list[str] = (), timeout: int = 120,
              version: str = "rev02") -> str:
    """Run MCC68K on `source`; returns the compiler's assembly text (it writes <NAME>.SRC)."""
    toolchain.verified_compiler()   # the exact programs about to run
    emu2 = toolchain.emu2_path()
    compiler = toolchain.mcc68k_dir() / "MCC68K" / "MCC68K.EXE"
    with tempfile.TemporaryDirectory(prefix="mcc68k-") as tmp:
        drive = Path(tmp)
        (drive / "MCC68K").mkdir()
        os.symlink(compiler, drive / "MCC68K" / "MCC68K.EXE")
        work = drive / "WORK"
        work.mkdir()
        for folder in include_dirs:   # MCC68K finds "name.h" in the current directory; keep relative paths
            folder = Path(folder)
            for header in sorted(folder.rglob("*.h")) if folder.is_dir() else []:
                target = work / header.relative_to(folder).as_posix().upper()
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(header, target)
        (work / "REVISION.H").write_text(revision_header(version))
        name = dos_name(source.stem)
        shutil.copy2(source, work / f"{name}.C")
        env = {**os.environ, "EMU2_DRIVE_C": str(drive), "EMU2_CWD": "C:\\WORK",
               "EMU2_DEFAULT_DRIVE": "C", "EMU2_PROGNAME": "C:\\MCC68K\\MCC68K.EXE"}
        command = [str(emu2), str(compiler), " ".join([name, *options]) + ";"]
        result = subprocess.run(command, cwd=work, env=env, capture_output=True, text=True, timeout=timeout)
        outputs = [p for p in work.iterdir() if p.name.upper() == f"{name}.SRC"]
        errors = re.search(r"(\d+) Errors? detected", result.stdout)
        if result.returncode != 0 or not outputs or (errors and errors.group(1) != "0"):
            raise CompileError(f"MCC68K failed on {source}:\n{result.stdout}{result.stderr}")
        return outputs[0].read_text(encoding="latin-1")


def near_targets(listing: str, address_of, spans: list[tuple[int, int]]) -> dict[str, str]:
    """External call targets inside the original translation unit that contains this file's first function."""
    first = first_function(listing)
    base = address_of(first) if first else None
    if base is None:
        return {}
    span = next(((lo, hi) for lo, hi in spans if lo <= base < hi), None)
    near = {}
    for target in external_calls(listing):
        address = address_of(target)
        if span and address is not None and span[0] <= address < span[1] and span[1] - span[0] <= PC16_REACH:
            near[target] = "backward" if address < base else "forward"
    return near


def assemble(gas: str, source: Path, obj: Path, unit: str) -> None:
    """Assemble adapted text; refuses code too large for the PC-relative forms adapt() chose."""
    asm = obj.with_suffix(".s")
    asm.write_text(gas)
    result = subprocess.run([str(toolchain.tool("m68k-elf", "as")), *AS_FLAGS, "-o", str(obj), str(asm)],
                            capture_output=True, text=True)
    if result.returncode:
        raise CompileError(f"{source}: the assembler rejected the compiler output:\n{result.stderr}")
    from elftools.elf.elffile import ELFFile
    with obj.open("rb") as stream:
        section = ELFFile(stream).get_section_by_name(f".text.{unit}")
        if section is not None and section["sh_size"] > PC16_REACH:
            raise CompileError(f"{source}: {section['sh_size']} bytes of code; files over 32 KB are not supported "
                               "(PC-relative calls inside them would not reach)")


def compile_to_gas(source: Path, unit: str, include_dirs: list[Path] = (), options: list[str] = (),
                   address_of=None, spans: list[tuple[int, int]] = (), version: str = "rev02") -> Compiled:
    listing = compile_c(source, include_dirs, options, version=version)
    near = near_targets(listing, address_of, list(spans)) if address_of else {}
    return Compiled(source, listing, adapt(listing, unit, near))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path)
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "mcc")
    parser.add_argument("--include", type=Path, action="append", default=[ROOT / "include"])
    parser.add_argument("--option", action="append", default=[], help="MCC68K option, e.g. /STRINTEXT")
    args = parser.parse_args()
    compiled = compile_to_gas(args.source, dos_name(args.source.stem).lower(), args.include, args.option)
    args.out.mkdir(parents=True, exist_ok=True)
    stem = args.source.stem
    (args.out / f"{stem}.SRC").write_text(compiled.listing, encoding="latin-1")
    (args.out / f"{stem}.s").write_text(compiled.gas)
    print(args.out / f"{stem}.s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
