# Resume

State as of 2026-10-21 (newest first in `docs/STATUS.md`).

## Machine setup from scratch

```sh
git clone git@github.com:mschienbein/toejam-earl-decomp.git && cd toejam-earl-decomp
uv sync
uv run python tools/setup.py                 # ~100 s: binutils 2.47 m68k-elf + z80-elf
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World) (Rev A).zip"
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World).zip"
uv run python tools/analyze.py --check && uv run python tools/analyze.py --check --version rev00
uv run python tools/build.py && uv run python tools/build.py --version rev00
uv run python -m unittest discover -s tests
```

Private inputs are pinned in the umbrella repository's `inputs/manifest.json`
(`mschienbein/genesis-experiments`, private): REV 02 from the user's download (primary), REV 00 from
the user's download (identical to the copy on the `\\NS1\Media` share).

## Where things are

- Catalogues: `config/rev02/catalogue.json`, `config/rev00/catalogue.json` (regenerate after any
  change to `tools/m68k.py`, `tools/analyze.py` or seeds; commit together).
- Seeds: `config/<version>/seeds.json`, derived by genesis-experiments `tools/tje_seeds.py` from
  `tools/tje_disasm_codemap.py` (rebuilds tje-disasm in that repo's `scratch/`).
- Build output (private): `build/<version>/asm/*.s`, `toejam.elf`, `toejam.map`, `toejam.bin`,
  `receipt.json`.

## In flight

Nothing. Next steps are listed under "Next" in `docs/STATUS.md`; the compiler hunt is first.
