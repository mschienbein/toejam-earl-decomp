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
BRANCH = re.compile(r"^(B(?:RA|SR|HI|LS|CC|HS|CS|LO|NE|EQ|VC|VS|PL|MI|GE|LT|GT|LE))(\s+)(\S+)$", re.I)
HEX = re.compile(r"\$([0-9A-Fa-f]+)")
DOTTED = re.compile(r"(?<![\w.$])\.([A-Za-z_]\w*)")
LABEL = re.compile(r"^([A-Za-z_]\w*):\s*(.*)$")
FIELDS = re.compile(r"""^(\S+)(?:(\s+)((?:'[^']*'|"[^"]*"|[^\s'"])+))?(?:\s+.*)?$""")


def strip_comment(statement: str) -> str:
    """Drop an end-of-line comment: Microtec ends the operand field at the first unquoted blank."""
    fields = FIELDS.match(statement)
    if not fields:
        return statement
    mnemonic, gap, operands = fields.groups()
    if operands is None or operands.startswith("*"):
        return mnemonic
    return f"{mnemonic}{gap}{operands}"


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


def compile_c(source: Path, include_dirs: list[Path] = (), options: list[str] = (), timeout: int = 120) -> str:
    """Run MCC68K on `source`; returns the compiler's assembly text (it writes <NAME>.SRC)."""
    emu2 = toolchain.emu2_path()
    compiler = toolchain.mcc68k_dir() / "MCC68K" / "MCC68K.EXE"
    for path in (emu2, compiler):
        if not path.is_file():
            raise toolchain.ToolchainError(f"{path} is missing; run tools/setup.py")
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


CALL = re.compile(r"^(JSR|JMP)(\s+)([A-Za-z_]\w*)$", re.I)


def code_labels(listing: str) -> set[str]:
    """Labels defined in the code section (SECTION 9), with the Microtec dot removed."""
    labels, in_code = set(), False
    for raw in listing.splitlines():
        stripped = raw.strip()
        if re.match(r"SECTION\s+(\d+)", stripped, re.I):
            in_code = re.match(r"SECTION\s+(\d+)", stripped, re.I).group(1) == "9"
            continue
        label = LABEL.match(DOTTED.sub(r"\1", stripped))
        if in_code and label:
            labels.add(label.group(1))
    return labels


def external_calls(listing: str) -> list[str]:
    """JSR/JMP targets that the listing does not define (other files, or the rest of this file's original unit)."""
    local, seen = code_labels(listing), []
    for raw in listing.splitlines():
        call = CALL.match(DOTTED.sub(r"\1", raw.strip()))
        if call and call.group(3) not in local and call.group(3) not in seen:
            seen.append(call.group(3))
    return seen


def first_function(listing: str) -> str | None:
    """The first label defined in the code section (the unit's lowest address)."""
    in_code = False
    for raw in listing.splitlines():
        stripped = raw.strip()
        section = re.match(r"SECTION\s+(\d+)", stripped, re.I)
        if section:
            in_code = section.group(1) == "9"
            continue
        label = LABEL.match(DOTTED.sub(r"\1", stripped))
        if in_code and label and not label.group(1).startswith("_L"):
            return label.group(1)
    return None


def adapt(listing: str, unit: str, near: dict[str, str] | None = None) -> str:
    """Microtec assembly -> GNU as assembly (directives, spellings and ASM68K sizing only).

    `near` names external call targets that sit in the same original translation unit as this C
    file ("backward" before it, "forward" after it): a C file holding only part of an original unit
    calls the rest the way the original assembler did, PC-relative (forward: plus NOP).
    """
    near = near or {}
    lines: list[str] = []
    defined: set[str] = set()
    local = code_labels(listing)
    for raw in listing.splitlines():
        stripped = raw.strip()
        if not stripped or stripped.startswith("*"):
            continue
        word = stripped.split()[0].upper()
        if word in ("OPT", "END", "XREF") or re.match(r"^\S+\s+IDNT\b", stripped, re.I):
            continue
        if word == "SECTION":
            number = int(re.match(r"SECTION\s+(\d+)", stripped, re.I).group(1))
            lines.append(f'\t.section .text.{unit},"ax"' if number == 9 else "\t.data")
            continue
        if word == "XDEF":
            names = [DOTTED.sub(r"\1", n.strip()) for n in stripped.split(None, 1)[1].split(",")]
            lines.append("\t.globl " + ", ".join(names))
            continue
        stripped = HEX.sub(lambda m: "0x" + m.group(1), DOTTED.sub(r"\1", stripped))
        label = LABEL.match(stripped)
        if label:
            defined.add(label.group(1))
            lines.append(f"{label.group(1)}:")
            stripped = label.group(2)
            if not stripped:
                continue
        stripped = strip_comment(stripped)
        branch = BRANCH.match(stripped)
        if branch:
            mnemonic, gap, target = branch.groups()
            if target in defined:   # backward: shortest form (gas jbra/jbsr/jb<cc> relax to byte when it fits)
                lines.append(f"\tjb{mnemonic[1:].lower()}{gap}{target}")
            else:                   # forward: the one-pass assembler reserved a word displacement
                lines.append(f"\t{mnemonic}.w{gap}{target}")
            continue
        call = CALL.match(stripped)
        if call:
            mnemonic, gap, target = call.groups()
            if target in defined:
                lines.append(f"\t{mnemonic}{gap}({target},PC)")
            elif target in local or near.get(target) == "forward":
                lines.append(f"\t{mnemonic}{gap}({target},PC)")
                lines.append("\tNOP")   # forward: PC-relative inside the 6 bytes reserved for absolute long
            elif near.get(target) == "backward":
                lines.append(f"\t{mnemonic}{gap}({target},PC)")
            else:
                lines.append(f"\t{mnemonic}{gap}({target}).L")
            continue
        lines.append("\t" + stripped)
    return "\n".join(lines) + "\n"


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
        if span and address is not None and span[0] <= address < span[1]:
            near[target] = "backward" if address < base else "forward"
    return near


def compile_to_gas(source: Path, unit: str, include_dirs: list[Path] = (), options: list[str] = (),
                   address_of=None, spans: list[tuple[int, int]] = ()) -> Compiled:
    listing = compile_c(source, include_dirs, options)
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
