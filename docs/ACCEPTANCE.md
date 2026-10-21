# Acceptance

## Harness baseline (now)

A build is **PASS** only when `tools/build.py` writes a receipt whose `actual.sha256` equals the
pinned sha256 of the version (`config/title.json`). The receipt binds:

- the catalogue sha256 (`config/<version>/catalogue.json`),
- the sha256 of `tools/build.py`, `m68k.py`, `rom.py`, `toolchain.py`,
- the sha256 of the `as`, `ld` and `objcopy` binaries and the `as` banner, and the assembler flags,
- counts: units by kind, instructions, how many are readable assembly vs raw words, switch tables.

The committed catalogue must equal what `tools/analyze.py` produces from the ROM and seeds
(`--check`, and `tests/test_build.py`). The baseline proves the harness; it earns no progress.

## Matched C (from the first C function on)

A function is credited in `config/matches.json` only when all hold:

1. Its bytes come from compiled C: the C unit replaces the function's assembly unit in a full-ROM
   build, and the image is byte-identical to the original (PASS receipt).
2. Two fresh builds from a clean tree produce the same image.
3. The source follows the rules in `AGENTS.md` (no inline assembly or byte blobs standing in for
   code; real types on cross-unit interfaces; qualifications recorded).
4. An independent reviewer re-derives the result from the candidate and the tools.
5. The compiler and its exact version/flags are pinned (`toolchain.lock.json`) and recorded in the
   receipt.

`ci/progress_report.py` counts only `config/matches.json`; generated assembly, hand-written
assembly, runtime helpers and data are never C progress. Tooling for steps 1–5 (`match.py`,
`accept.py`) is built once a compiler is identified (`docs/COMPILER.md`).
