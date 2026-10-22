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
- branch sizing: Microtec's one-pass assembler gives an unsized branch to a label that is not yet
  defined (a forward reference) a 16-bit displacement; GNU as would shrink it, so unsized forward
  Bcc/BRA/BSR get an explicit `.w`. Backward branches stay unsized (both assemblers pick the
  shortest form for a known distance).
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
        for k, folder in enumerate(include_dirs):
            target = drive / f"INC{k}"
            target.mkdir()
            for header in Path(folder).glob("*.h"):
                shutil.copy2(header, target / header.name.upper())
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


def adapt(listing: str, unit: str) -> str:
    """Microtec assembly -> GNU as assembly (directives and spellings only)."""
    lines: list[str] = []
    defined: set[str] = set()
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
        branch = BRANCH.match(stripped)
        if branch:
            mnemonic, gap, target = branch.groups()
            size = "" if target in defined else ".w"   # forward reference: the one-pass assembler uses a word
            lines.append(f"\t{mnemonic}{size}{gap}{target}")
            continue
        lines.append("\t" + stripped)
    return "\n".join(lines) + "\n"


def compile_to_gas(source: Path, unit: str, include_dirs: list[Path] = (), options: list[str] = ()) -> Compiled:
    listing = compile_c(source, include_dirs, options)
    return Compiled(source, listing, adapt(listing, unit))


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
