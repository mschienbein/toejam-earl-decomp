# REV 00 and REV 02

Both retail revisions build from the same sources: `tools/mcc.py` generates the per-build revision
header (`REVISION` 0 or 2), and `#if REVISION` blocks hold the few real differences.
Checkpoint c033 matches all 598 of 598 C functions (100%) in each revision
under denominator r12; the five R011 collision queries are readable assembly source. Names, addresses
spelled in the source and all documents refer to REV 02; `tools/revmap.py` maps REV 02 addresses
to REV 00.

| | REV 00 — *ToeJam & Earl (World)* | REV 02 — *(Rev A)* |
|---|---|---|
| Cartridge header | `(C)SEGA 1991.JUL`, `GM MK-1020 -00` | `(C)SEGA 1991.OCT`, `GM MK-1020 -02` |
| Code | 112 bytes shorter | eight functions changed |
| Data | identical content; addresses inside moved | same |

## What changed in the code

Of 648 paired code entries (645 functions and three fragments), 223 are byte-identical and 417
differ only in addresses, because the code or data they refer to moved (REV 02's changes push
everything after them by up to 112 bytes); the pairing verifies this with address-masked
instruction signatures, and every one of them builds both revisions from one unchanged source.
Eight functions differ in their logic; all eight read as fixes to REV 00 behaviour:

| Function (file) | REV 02 change |
|---|---|
| `SetupSelectedPresent` (`src/presents/present_effects.c`) | Rocket skates no longer start while the player is falling (`gPlayerFallStates` check around `OpenRocketSkates`), and one more animation group (bit 27 of `animation_group_mask`) lets the active-present pose replace the current one. |
| `ProcessLand` (`src/map/land_processing.c`) | A player caught in a whirlwind (`PLAYER_ANIM_WHIRLWIND`, `PLAYER_ANIM_WHIRLWIND_RECOVERY`) no longer starts falling when the land under them turns to void; applied to ToeJam and to Earl in both shared and split screen. |
| `ProcessSleep` (`src/player/sleep.c`) | The idle counter that puts a player to sleep does not run while an interaction choice is open (`gPlayerInteractionChoiceActive`), so a player no longer falls asleep in the middle of a choice prompt. |
| `DrawGameMap` (`src/map/map_drawing_and_controller.c`) | Opening the map resets the opener's idle counter (`gPlayerIdleTicks`). |
| `DrawInventory` (`src/ui/inventory.c`) | Opening the inventory resets the opener's idle counter too. |
| `TitleScreenVideo` (`src/cutscenes/title_and_transition.c`) | The 1,200-tick title wait restarts the global `gFrameCounter` at zero instead of measuring from the value it had on entry. |
| `SEGAVideo` (`src/cutscenes/sega_logo.c`) | The Sega logo restarts `gFrameCounter` at zero before its 120-tick fade, as above. |
| `PlayerProcessTerrainMotion` (`src/player/entity_motion_terrain.c`) | Terrain motion is also skipped while the player plays a lean animation (`PLAYER_GROUP_LEAN` joins `TERRAIN_MOTION_PAUSED_GROUPS`); same size, one wider mask constant. |

The two idle-counter resets and the sleep guard together keep a player from dozing off because
time spent in the map, the inventory or a choice prompt counted as idle time. The `gFrameCounter`
restarts make the attract-sequence timing start from a known value [INFERENCE: it also changes
how far the counter has advanced when the game begins].

## What changed in the data

All 4,163 typed data units exist in both revisions under the same names; all but the final fill
(`ROMTailFill`, 424 bytes in REV 02 and 536 in REV 00, absorbing the 112 bytes the code grew)
have the same sizes. 2,519 units are identical and 1,642 differ only in addresses they contain
(pointers to code, animation frames or other data that moved; `FunkatronSceneDescriptors` stores
the low words of such addresses). Apart from the cartridge header (release month, serial suffix,
checksum) and that fill, no graphics, text, music, sound or level data changed.

## How this was established

`tools/revmap.py` pairs the catalogued functions of both revisions and verifies the pairing with
masked instruction signatures; the eight logic changes are the functions whose REV 00 bytes need
a `#if REVISION` block to match (every other function matches both revisions from one unchanged
source). The data comparison translates every differing address through the same map.
