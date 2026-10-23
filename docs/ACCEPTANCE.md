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

`ci/progress_report.py` counts only `config/matches.json`; generated assembly, hand-written or
otherwise non-C assembly source, runtime helpers and data are never C progress.

## The denominator (what 100% means)

The C denominator is every catalogued REV 02 function (`config/rev02/catalogue.json`) except those
`config/rev02/classification.json` classifies **with high confidence** as not C (`handwritten_asm`,
`runtime_helper`, `sega_library`, and `not_reproducible_c` for functions shown not to be reproducible
from plain C by the pinned compiler, ruling R011). Medium/low-confidence non-C and `unknown` functions
stay in the denominator until their evidence is upgraded. REV 00 takes REV 02's classification through
the function correspondence. Catalogue and classification change only together with a
recorded revision in `docs/denominator-history.jsonl` (`tools/denominator.py`), which CI verifies is
append-only.

## Tooling

- `tools/match.py src/x.c` — author loop: compile with MCC68K 3.3K (under emu2), adapt the
  Microtec syntax for GNU as (`tools/mcc.py`; no instruction changes), link at the original address
  with every reference resolved to its original address, compare bytes; side-by-side diff on a miss.
  Addresses come only from `config/<version>/names.json` (REV 02 identities in other revisions) and
  address spellings (REV 02 addresses); `--define` may add an unknown name for an experiment but
  never move a known one, and a named place must be written with its name.
- `config/sources.json` — the source units in the build: `{"source", "versions", "options"}`. A `.c`
  unit is compiled with MCC68K; a `.s` unit under `src/asm/` is hand-written 68000 assembly (ruling
  R007, GNU `as -m68000 --pcrel`). Each unit's routines carry their canonical names and replace one
  contiguous run of catalogued units exactly (`tools/cunits.py`); receipts list every unit, its kind
  and every routine with its sha256, address and size and the hashes of the compiler, emulator and
  binutils actually run (checked against `toolchain.lock.json` and the installation record before
  use). A unit never falls back to raw words: a mismatch fails the build. Only C units earn credit.
  An entry `{"source": "src/z80/x.s", "kind": "z80", "unit": NAME}` assembles Z80 source with the
  pinned `z80-elf` binutils at origin 0 in place of the extracted data unit NAME (same size; the
  full-image check proves the bytes).
- `tools/accept.py prepare --checkpoint cNNN-name` — claims the name, records a manifest of every
  input (`src`, `include`, `config`, `tools`, `ci`, `tests`, the lock file), builds REV 02 and REV 00
  twice from scratch (both PASS, identical images) and runs the suite; the record is written even
  when something fails. `review` records an independent reviewer's verdict. `finalize` needs a PASS
  prepare, a PASS review and no FAIL review, C sources and headers identical to the prepared ones,
  and a fresh build of both revisions that PASSes with exactly the prepared C units; then it writes
  `finalize.json` once, credits each function by version and address in `config/matches.json`
  (schema 2: a function moved to another file replaces its record, never counts twice) and appends
  `docs/progress-history.jsonl`.
