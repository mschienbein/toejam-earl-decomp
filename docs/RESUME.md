# Resume

State as of 2026-10-22, evening (newest first in `docs/STATUS.md`), through checkpoint c030.
Goal: 100% matching, readable C. Both revisions have 597 of 603 C functions matched:
173,328 of 175,412 bytes in REV 02 and 173,216 of 175,300 bytes in REV 00 (98.8% each).

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

The Microtec archive (archive.org, ruling R006) and emu2 are pinned in
`toolchain.lock.json`; ROMs and extracted inputs remain private and uncommitted.

## Author loop

1. Pick a function (or a whole span from `config/rev02/tus.json`). Run
   `uv run python tools/packet.py` with its canonical name to produce a private packet
   containing named assembly, callers, and callees.
2. Write K&R C with the canonical names (`config/rev02/names.json`), the shared types and
   constants from `include/`, following `docs/AUTHORING.md` and `docs/COMPILER.md`.
3. Run `uv run python tools/match.py src/core/error_trap.c`, replacing the existing example
   source path with your candidate, and repeat for REV 00 using the version option until
   every function reports MATCH. Any compiler-coaxing spelling needs a
   `MATCHING QUALIFICATION:` comment.
4. Main adds the file to `config/sources.json`, then uses `tools/accept.py` for prepare,
   an independent review, review recording, and finalize. Push only after
   `tools/audit_public.py --history` passes.

## Where things are

- Sources: `src/` has one folder per C subsystem, plus `src/asm/` (hand-written 68000)
  and `src/z80/` (Z80 driver); headers are in `include/`. Subsystem descriptions and
  native-port notes are in `docs/subsystems/`.
- Catalogues: `config/rev02/catalogue.json` and `config/rev00/catalogue.json`; canonical
  identities: `config/rev02/names.json`; classification: `config/rev02/classification.json`.
  The other per-revision configuration files hold data units, TU spans, seeds, and
  correspondence. Denominator history is `docs/denominator-history.jsonl` (r11),
  accepted matches are in `config/matches.json`, and checkpoint records are in `accept/`.
- Current complete units include `src/camera/camera_tiles_and_scroll.c`,
  `src/map/random_regions_and_cleanup.c`, `src/map/level_generation_and_changes.c`,
  `src/map/map_drawing_and_controller.c`, `src/player/active_present_falling_and_respawn.c`,
  `src/player/sprite_checks_and_movement.c`, `src/player/earthling_damage_reactions.c`,
  `src/sprites/entity_sprite_rendering.c`, `src/speech/speech_and_dialogue.c`,
  `src/ship/ship_screen_and_collection.c`, and `src/ui/hud.c`. Bounded variable-frame
  access views are declared separately in `include/sprite_frame_views.h`.

## Open

6 C functions remain unmatched (`ci/progress_report.py` lists them):

- `PlayerProcessTerrainMotion` is the remaining player terrain-motion body to match.
- Five collision queries await a user decision on classification:
  `EntityFindNarrowCollisionAtPosition`, `EntityFindMenuAwareCollisionAtPosition`,
  `EntityFindVolumeCollisionAtPosition`, `ProjectileFindCollisionExcludingOwner`,
  and `EntityFindHorizontalCollisionAtX`. They remain in the C denominator unless
  that decision changes their classification.
