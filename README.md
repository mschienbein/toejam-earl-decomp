# ToeJam & Earl decompilation

[![REV 02 C code](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2002%20C%20code&measure=matched_code_percent&version=rev02)](https://bytematch.dev/mschienbein/toejam-earl-decomp) [![REV 02 C functions](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2002%20C%20functions&measure=matched_functions&version=rev02)](https://bytematch.dev/mschienbein/toejam-earl-decomp) [![REV 02 typed data](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2002%20typed%20data&measure=matched_data_percent&version=rev02)](https://bytematch.dev/mschienbein/toejam-earl-decomp)  
[![REV 00 C code](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2000%20C%20code&measure=matched_code_percent&version=rev00)](https://bytematch.dev/mschienbein/toejam-earl-decomp) [![REV 00 C functions](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2000%20C%20functions&measure=matched_functions&version=rev00)](https://bytematch.dev/mschienbein/toejam-earl-decomp) [![REV 00 typed data](https://bytematch.dev/mschienbein/toejam-earl-decomp.svg?label=REV%2000%20typed%20data&measure=matched_data_percent&version=rev00)](https://bytematch.dev/mschienbein/toejam-earl-decomp)  
[![progress](https://github.com/mschienbein/toejam-earl-decomp/actions/workflows/progress.yml/badge.svg)](https://github.com/mschienbein/toejam-earl-decomp/actions/workflows/progress.yml)

A work-in-progress **matching decompilation** of *ToeJam & Earl* (Sega Genesis / Mega Drive, 1991,
Johnson Voorsanger Productions, `GM MK-1020`): C source that compiles back to the exact original ROM.

**Status:** both retail revisions rebuild **byte-identically** from this repository's sources plus
assets extracted from your own dump at build time. The badges above are live (every push to `main`
uploads a report per revision to [ByteMatch](https://bytematch.dev/mschienbein/toejam-earl-decomp)).
The C code and C function badges count only the code written in C; the functions classified as
hand-written assembly or compiler runtime are not in that count, because they are built from
assembly source instead. The typed-data badges count data bytes rebuilt from typed assets.

| Part of the ROM | Built from | REV 02 (checkpoint c026) | REV 00 (checkpoint c026) |
|---|---|---|---|
| C functions (603 in each) | matched C in `src/` (MCC68K 3.3K) | **588 functions, 169,216 / 175,412 bytes (96.5%)** | **588 functions, 169,104 / 175,300 bytes (96.5%)** |
| 36 hand-written routines + 6 compiler runtime helpers | readable 68000 assembly, `src/asm/` (ruling [R007](docs/rulings/R007-assembly-sources.md)) | all 42 | all 42 |
| Z80 sound driver + boot stub (1,332 bytes) | Z80 assembly, `src/z80/` | all | all |
| Other data | typed assets extracted from your dump at build time (`docs/DATA.md`) | all 865,932 bytes (6 typed `unreferenced`) | all 866,044 bytes (6 typed `unreferenced`) |
| Still generated assembly | — | 15 C functions | the same 15 |

Both revisions build from the same sources; the per-build `REVISION` value covers their few
differences (seven bug fixes in REV 02, [docs/REVISIONS.md](docs/REVISIONS.md)), so a function
matched for one is checked and credited for both, and CI reports each revision to ByteMatch
separately. History: [docs/STATUS.md](docs/STATUS.md); what remains: [docs/RESUME.md](docs/RESUME.md).

| Revision | Role | Functions | Rebuild |
|---|---|---:|---|
| REV 02 — *ToeJam & Earl (World) (Rev A)*, sha1 `85e8d0a4…` | primary target | 645 | byte-identical |
| REV 00 — *ToeJam & Earl (World)*, sha1 `7f82d8b5…` | same source, `REVISION` | 645 | byte-identical |

## Quick start

Needs Linux or macOS with a C compiler and make (for binutils and the emu2 DOS emulator),
[uv](https://docs.astral.sh/uv/) and your own dump of the game.

```sh
uv sync                                   # Python 3.12 + capstone, pyelftools (locked)
uv run python tools/setup.py              # binutils 2.47 (m68k-elf, z80-elf), emu2, Microtec MCC68K 3.3K
uv run python tools/rom.py import --rom "ToeJam & Earl (World) (Rev A).zip"   # -> orig/rev02/
uv run python tools/build.py              # -> build/rev02/toejam.bin + receipt.json (PASS)
uv run python tools/build.py --version rev00
uv run python -m unittest discover -s tests
```

`tools/rom.py import` accepts `.md`/`.bin`/`.gen` or a `.zip` holding one dump and identifies the
revision by SHA-256 (`config/title.json`). Writing C: [docs/AUTHORING.md](docs/AUTHORING.md).

## How the build works

```mermaid
flowchart LR
  rom[your dump] --> extract[tools/assets.py<br/>typed data units] --> data[assets: graphics, maps, music, text, tables]
  c[src/**/*.c] --> mcc[MCC68K 3.3K in emu2<br/>tools/mcc.py] --> objs
  s[src/asm/*.s] --> as[m68k-elf-as] --> objs
  z80[src/z80/*.s] --> z80as[z80-elf-as] --> data
  rest[unmatched functions<br/>generated assembly] --> as
  objs[objects at original addresses] --> ld[m68k-elf-ld] --> bin[toejam.bin]
  data --> ld
  bin --> cmp{== original?} --> receipt[receipt.json]
```

- The original game was compiled with **Microtec MCC68K 3.3K** (default options plus `/STRINTEXT`)
  and assembled with ASM68K; `docs/COMPILER.md` records how its output is reproduced.
- `config/sources.json` lists the source units; each replaces exactly the catalogued functions it
  defines. A C unit that does not compile to the original bytes fails the build.
- `tools/accept.py` turns matching sources into credit only through checkpoints: two fresh builds
  of both revisions, the test suite, an independent review, then `config/matches.json`.
- Functions without source yet are rendered from the catalogue as GNU assembly at build time
  (git-ignored, never committed).

## Layout

| Path | Contents |
|---|---|
| `src/` | matched C, one folder per subsystem; `src/asm/` hand-written 68000, `src/z80/` the Z80 driver |
| `include/` | K&R headers per subsystem: records, constants, RAM, assets, calls |
| `docs/subsystems/` | [how each subsystem works](docs/subsystems/README.md), with notes for a native port |
| `tools/` | toolchain setup, ROM import, catalogue, build, compiler driver, matcher, acceptance, public audit |
| `config/` | catalogues, names, classification, data units, sources, accepted matches per revision |
| `accept/` | checkpoint records (prepare, reviews, finalize) |
| `ci/progress_report.py` | objdiff-format report for [ByteMatch](https://bytematch.dev), no ROM needed |
| `docs/` | [STATUS](docs/STATUS.md), [RESUME](docs/RESUME.md), [AUTHORING](docs/AUTHORING.md), [ACCEPTANCE](docs/ACCEPTANCE.md), [COMPILER](docs/COMPILER.md), [DATA](docs/DATA.md), [TOOLCHAIN](docs/TOOLCHAIN.md), [BYTEMATCH](docs/BYTEMATCH.md), `docs/rulings/` |
| `tests/` | synthetic tests (CI) + real-ROM builds (skipped without `orig/`) |

## Credits and prior work

This project builds on two disassemblies of the game (ruling [R003](docs/rulings/R003-existing-work.md)):

- [bstreiff/tje-disasm](https://github.com/bstreiff/tje-disasm) (REV 02, GNU as): code map, labels,
  RAM variables, structure layouts, resource formats and the compiler-hunting notes;
- [RyanFaeScotland's ToeJam & Earl disassembly](https://bitbucket.org/RyanFaeScotland/toejamandearldisassembly)
  (REV 00, vasm; [project page](https://www.mybrillgamesite.com/projects/toejam-and-earl-disassembly)):
  commented menus, cutscenes, player controls, level seeds and presents.

Names, addresses, layouts and formats taken from them are restated here with per-entry provenance;
none of their source files are copied.

## Legal

This repository contains no ROM data, extracted assets or generated disassembly: only
reconstructed source (C, and readable assembly for the hand-written routines per ruling R007),
tools, addresses, sizes and hashes. Build outputs (`build/`) embed ROM bytes and are git-ignored;
`tools/audit_public.py` checks every commit. You need your own legally obtained copy of the game. ToeJam & Earl is a trademark of its owners; this
project is not affiliated with them or with Sega.
