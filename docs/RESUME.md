# Resume

State as of 2026-10-22, evening (newest first in `docs/STATUS.md`). Goal: 100% matching, readable C
(goal text: genesis-experiments `context/session-handoffs/20261021T230518Z-tje-goal/GOAL.md`).

## Machine setup from scratch

```sh
git clone git@github.com:mschienbein/toejam-earl-decomp.git && cd toejam-earl-decomp
uv sync
uv run python tools/setup.py        # binutils 2.47 (m68k-elf, z80-elf), emu2, Microtec MCC68K 3.3K
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World) (Rev A).zip"
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World).zip"
uv run python tools/build.py && uv run python tools/build.py --version rev00
uv run python -m unittest discover -s tests
```

The Microtec archive (archive.org `mri-68-k-c-cross-compiler-toolchain`, ruling R006) and emu2 are
pinned in `toolchain.lock.json`; private inputs in genesis-experiments `inputs/manifest.json`.

## Author loop

1. Pick a function (or a whole span from `config/rev02/tus.json`); `uv run python tools/packet.py
   <name>` writes a private packet (assembly with names, callers, callees) to `scratch/packets/`.
2. Write K&R C with the canonical names (`config/rev02/names.json`), the shared types and
   constants from `include/`, following `docs/AUTHORING.md` and `docs/COMPILER.md`.
3. `uv run python tools/match.py path/to/file.c` (and `--version rev00`) until every function
   reports `MATCH`; any compiler-coaxing spelling needs a `MATCHING QUALIFICATION:` comment.
4. Main adds the file to `config/sources.json`, then `tools/accept.py prepare`, an independent
   review, `review`, `finalize`; push only after `tools/audit_public.py --history` passes.

## Where things are

- Sources: `src/<subsystem>/` (C), `src/asm/` (hand-written 68000), `src/z80/` (Z80 driver);
  headers `include/`; how each subsystem works: `docs/subsystems/`.
- Catalogues `config/<version>/catalogue.json`, names, classification (rev02; REV 00 follows the
  correspondence), data units, TU spans, seeds; denominator history `docs/denominator-history.jsonl`
  (r11); accepted matches `config/matches.json`; checkpoint records `accept/`.
- Sub-agent records: genesis-experiments `records/tje/subagent-log.jsonl`; lane scratch roots in
  `scratch/agents/<Name>/` (private). The automated source-mutation search is in
  `scratch/agents/PermuterSmith/tool/` (private).

## Open

- 22 C functions unmatched (`ci/progress_report.py` lists them): five collision queries whose
  `SUB.W/BPL/NEG.W` sequences MCC68K 3.3K never emits (classification evidence being gathered), and
  17 near misses (frame layout, register choice, scheduling) under automated and manual search.
