# Resume

State as of 2026-10-21, night (newest first in `docs/STATUS.md`). Goal: 100% matching, readable C
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

1. Pick a function (or a whole span from `config/rev02/tus.json`); read its generated assembly
   (`build/rev02/asm/<unit>.s`, names applied) and the C conventions in `docs/COMPILER.md`.
2. Write K&R C named with the canonical names (`config/rev02/names.json`, else `func_XXXXXX`).
3. `uv run python tools/match.py path/to/file.c` until every function reports `MATCH`.
4. Main adds the file to `config/sources.json`, then `tools/accept.py prepare/review/finalize`.

## Where things are

- Catalogues `config/<version>/catalogue.json`, names, classification (rev02), TU spans, seeds;
  denominator history `docs/denominator-history.jsonl` (r2).
- Sub-agent records: genesis-experiments `records/tje/subagent-log.jsonl`; lane scratch roots in
  `scratch/agents/<Name>/` (private).

## In flight

- Reviews: Toolsmith r2 (audit_public/packet/diff), MatchInfraReview (Main's pipeline commits up to
  `18a6e48`; `d269cb4` near calls needs a delta review), DataReview (data-closure proposal).
- CompilerHunter: packaging its matched functions as the first checkpoint's C units.
- Local commits after `986837f` are unpushed until their reviews pass.
