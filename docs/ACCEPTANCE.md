# Acceptance

## Harness baseline (now)

A build is **PASS** only when `tools/build.py` writes a receipt whose `actual.sha256` equals the
pinned sha256 of the version (`config/title.json`). The receipt binds:

- the catalogue sha256 (`config/<version>/catalogue.json`),
- the sha256 of `tools/build.py`, `m68k.py`, `names.py`, `rom.py`, `toolchain.py`,
- the sha256 of `config/<version>/names.json` (`names_sha256`; `null` when the version has no names file),
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
assembly, runtime helpers and data are never C progress.

## Tooling

- `tools/match.py src/x.c` — author loop: compile with MCC68K 3.3K (under emu2), adapt the
  Microtec syntax for GNU as (`tools/mcc.py`; no instruction changes), link at the original address
  with every reference resolved to its original address, compare bytes; side-by-side diff on a miss.
- `config/sources.json` — the C units in the build: `{"source", "versions", "options"}`. Each unit's
  functions carry their canonical names (imported name, else `func_XXXXXX`) and replace one
  contiguous run of catalogued units exactly (`tools/cunits.py`); receipts list every C unit with
  its sha256, address and size and bind the compiler identity. A C unit never falls back to raw
  words: a mismatch fails the build.
- `tools/accept.py prepare --checkpoint cNNN-name` — two fresh builds of REV 02 and REV 00 (both PASS,
  identical images) and the full suite, recorded once in `accept/<checkpoint>/prepare.json`;
  `review` records an independent reviewer's verdict; `finalize` (needs a PASS prepare, a PASS
  review, no FAIL review and unchanged sources) credits the units in `config/matches.json` and
  appends `docs/progress-history.jsonl`.
