# Agent rules — ToeJam & Earl decomp

Target: REV 02 (*ToeJam & Earl (World) (Rev A)*), `orig/rev02/baserom.bin`, sha256 `65ea2386…`
pinned in `config/title.json`; REV 00 is a secondary version. Private inputs live in git-ignored
`orig/`, produced by `tools/rom.py import`. Read `README.md`, `docs/STATUS.md` (current entry on top)
and `docs/ACCEPTANCE.md` before changing anything.

## Roles

- **Integrator (Main):** one long-lived session. Owns shared state: `config/` (catalogues, seeds,
  matches), compiler profiles, link layout, headers shared across units and every accepted receipt.
  Routes work, verifies results, writes decision records. Never reviews its own work.
- **Authors:** one function or one translation unit each, in a private scratch directory
  (`scratch/<agent>/`). They never edit canonical source, config or catalogues.
- **Reviewers:** independent of the author (another model family where possible). They re-derive
  results from the candidate and the tools and seal their verdict before reading anyone else's.

## Single-writer rule

Only the integrator runs state-changing steps: regenerating catalogues, editing seeds or shared
headers, adopting candidates, building accepted receipts, recording progress. Two state-changing
runs never overlap.

## What counts

- **Matched:** the function's bytes come from compiled C in a full-ROM build that equals the
  original, reproduced by two fresh builds, with the source rules below respected
  (`docs/ACCEPTANCE.md`).
- **Never credited as C:** the generated assembly baseline, hand-written assembly (Sega's start-up
  code, VDP/Z80 drivers, decompressors), compiler runtime helpers, the Z80 sound driver, data.
  These may later be *classified* with evidence, never counted as decompiled. Ruling R007: the
  hand-written routines and the Z80 driver are committed as readable assembly source (`src/asm/`,
  `src/z80/`).
- A byte-identical rebuild from generated assembly proves the harness, not any source.

## Source rules

No inline assembly standing in for C. No byte blobs standing in for code or data. Real types on
cross-unit interfaces. A fake match (code shaped only to steer the compiler) is allowed only as a
recorded qualification. Preserved original bugs carry an `ORIGINAL BUG:` comment.

## Private material

Never commit ROM bytes, extracted assets, generated assembly (`build/`), disassembly listings or
packets. `scratch/`, `build/`, `orig/` and `.cache/` are git-ignored; keep it that way. Commit
addresses, sizes, hashes and tools only. The existing disassemblies (bstreiff/tje-disasm,
RyanFaeScotland's) carry no licence. Ruling R003: import their names, RAM layout, structure layouts
and data formats, restated in this repository with per-entry provenance; never copy their source.

## Evidence

- Records are immutable: versioned names, never overwritten; failures are kept.
- A failed gate is recorded and its cause fixed. Rerunning unchanged input is not evidence.
- Label anything not directly observed as [INFERENCE].
- User decisions are recorded as rulings in `docs/rulings/`; agents surface choices, they do not
  make policy.

## Operations

```sh
uv run python tools/setup.py --check                  # pinned binutils present and unchanged
uv run python tools/rom.py verify --version rev02
uv run python tools/analyze.py --check                # committed catalogue == analyzer output
uv run python tools/build.py                          # PASS = byte-identical
uv run python -m unittest discover -s tests
```

Regenerate both catalogues (`tools/analyze.py`, `--version rev00`) whenever `tools/m68k.py`,
`tools/analyze.py` or a seeds file changes, and commit them with the change.
