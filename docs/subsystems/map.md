# Map, world generation, terrain, and random numbers

## Purpose and reading boundary

The map subsystem maintains the players' current level geometry, generates ordinary
levels from stored seeds, answers terrain queries, animates disappearing paths, and
runs the full-screen map display. Its geometry is not a collection of independent
screen-sized levels: `ChangeLevel` manages two active maps and one saved map, while
`GetMapTerrainAtWorldPosition` translates world coordinates into metatiles and tiles.

Source paths describe the repository layout. Map C files live in `src/map`;
bare C filenames below abbreviate that directory, including `random_regions_and_cleanup.c`.
Function names are canonical source/header identities also recorded in
`config/rev02/names.json`, not proof of historical source spelling.
`include/terrain.h` names tile terrain and final metatile fields. Generation-region
labels below remain numeric: they are not terrain classes or similarly valued
VDP attribute constants.

## Files and their roles

- `level_generation_and_changes.c`: `GenerateLevelSeeds`, anchor generation and
  connection, filled/path shapes, scattered features, metatile alternatives,
  `ChangeLevel`, `MapLoadPlayerLevel`, `LoadFinalLevel`, and player/saved-bank exchange.
- `random_regions_and_cleanup.c`: Park–Miller RNG, seed exchange, random cache,
  generation parameters, directional path carving, region-origin searches/growth,
  rectangular region merging, boundary widening, path-side expansion, interior
  seeding, and region-edge shape masks.
- `map_population.c`: recursive anchor connectivity test, despite its broad name.
- `transition_paths.c`: descriptor membership and recursive overlay-chain selection.
- `terrain_at_position.c`: point queries and the two five-by-five player caches.
- `terrain_placement.c`: tile-property rectangle validation and player placement.
- `land_processing.c`: marked-path consumption, animated redraws, and a shared
  entity allocator with eviction; these are live-game operations, not generation.
- `map_drawing_and_controller.c`: map terrain/background, cover animation/uploads,
  icons, marker blinking, and the blocking `DrawGameMap` lifecycle.
- `elevator_icon.c`: map elevator marker in `Map_DrawElevator`.

## Data structures and RAM

`include/ram_map.h` declares `gPlayer1LevelMap`, `gPlayer2LevelMap`, and `gSavedLevelMap`, each
26 rows by 19 unsigned-short cells. A and B are active map planes and C is the
saved plane (`ChangeLevel`, `CopyLevelBetweenPlayers`, `MapSwapPlayerWithSavedBank`).
`gRequestedMapLevel` selects the level being loaded; `gPlayer1MapLevel` and `gPlayer2MapLevel` identify
the active planes; `gSavedMapLevel` identifies the saved plane. These identities
belong to map-cache banks, not four independent player-entity level fields.

`gMapPathDirectionBanks[52][19]` supplies packed per-active-map direction nibbles; the existing
`gSavedMapPathDirections[26][19]` view starts at its row 26. Copy and swap routines
move the appropriate nibble while preserving the other player's nibble. These are
aliases, not separately allocated maps (`include/ram_map.h`, `level_generation_and_changes.c`).

Map-word meaning changes during generation. `MapCarveDirectionalPath` writes
class `0x2000` plus direction bits 1/2/4/8. Region cleanup uses classes `0x1000`
and `0x3000`, region bits, and temporary `0x0800` marks. Final alternative selection
preserves upper flags, writes the low-byte metatile, and can retain an overlay
number selected by `METATILE_OVERLAY_MASK` (`MapChooseMetatileAlternatives`).
Do not apply one final-map bitfield interpretation to every generation stage.

`include/map.h` defines `map_generation_tables`: base path tiles, plain/alternate
edge and filled-shape tables, direction choices, excluded present terrain, and
control-remap directions. It also defines `map_transition_descriptor`, whose shape
list is count-prefixed and whose alternative list supplies replacement choices.
The bank is shared with presents, not solely map generation.

`include/assets_map.h` declares `FixedWorldSeeds[26]`, `Level0MapData`,
`Level1MapData`, `FunkatronMapData`, `LevelMetatileMaps[122][8][16]`,
`OverlayMetatileMaps[15][8][16]`, `PlayerTilePropertyLookup`,
`TileAlternativePointers`, `MapTransitionDescriptors[16]`, and the range tables.
`MapGenerationRangeRow` selects a row of `MapGenerationRangePairs[8][8][2]`
using the current ordinary level (`MapGetRandomGenerationParameter`).

`include/ram_map.h` also owns `gMapVisitedCoverBits`, `gMapRevealedCoverBits`,
`gMapRevealMode`, `gMapDirectionalPathMode`, transition counters, eight land-effect
handles, and the shared terrain-query word. Its visited-table outer extent is
explicitly marked [INFERENCE] in that header. The query's low-byte alias assumes
the original big-endian layout.

`include/ram_system.h` owns `gRNGState`, `gCachedRandomValues[59]`, and
`gRandomCacheIndex`; `include/random.h` declares their public operations.
Player terrain neighborhoods are in `include/ram_player.h`; elevator and ship
anchor coordinates are views of `gLevelAnchorPositions` in `include/ram_elevator.h`.

## Random-number behavior

`GetRandomNumber` implements the Park–Miller recurrence using Schrage's split:
it divides the state by 127773, combines the quotient and remainder using 16807
and 2836, and adds 2147483647 when the result is nonpositive. It stores the full
new state but returns only its low unsigned 16 bits (`random_regions_and_cleanup.c`).

`StackBasedRandom(seed)` returns the previous state while normalizing and installing
the supplied seed. It negates negative values, subtracts the modulus once when
necessary, and turns zero into one. Its name does not imply an independent RNG.

`PreGenerateRandoms` draws 59 words, resets the cache index, then restores the
original generator state. Reading cached values therefore has different effects
from calling `GetRandomNumber`; `DisplayMap` explicitly cycles through the cache.

`GenerateLevelSeeds` copies `FixedWorldSeeds` when `gWorldType` is nonzero;
otherwise it draws all 26 seeds. It initializes both active level identities to
one, the saved identity to -1, and the highest level to one (`level_generation_and_changes.c`).

The main loop also advances the RNG repeatedly while waiting for `gFrameCounter`
to change, and initial world seeding combines a draw with `gFrameCounter`
(`FurtherInitialisation`, `src/core/startup_and_main_loop.c`). Thus a stored
level seed is useful evidence for generation, but is not by itself a complete
record of every runtime random decision.

## Level-loading and generation pipeline

1. `MapLoadPlayerLevel` sends levels 26 and above to `LoadFinalLevel`; ordinary
   requests set `gRequestedMapLevel` and call `ChangeLevel` until it succeeds.
2. `ChangeLevel` exchanges in `gLevelSeeds[level]`, retaining the previous RNG state.
   For levels above one it first regenerates separated anchor coordinates.
3. It reuses the requested player's active bank, copies the partner's bank, or
   exchanges with the saved bank when the requested level is already represented.
   Player-two generation may temporarily swap A and B (`ChangeLevel`).
4. Level zero is initialized to metatile 34 with `Level0MapData` placed at its fixed
   offset. Level one copies `Level1MapData`. `LoadFinalLevel` copies
   `FunkatronMapData` to both active planes and sets their level identities.
5. For a new ordinary map, `ChangeLevel` waits for pending VBlank work, clears A
   and its direction nibble, then calls `MapConnectAnchorPositions`.
6. It converts generation parameter 3 to a percentage of 19×26 cells and requests
   class-`0x1000` growth around each anchor through `MapGrowTerrainRegion`.
7. `MapStabilizeRegionConnections` widens horizontal/vertical boundaries, then
   repeatedly expands class-`0x2000` path neighborhoods until stable.
8. `MapIsAnchorConnectionMissing` rejects a map when the recursive
   `MapFindRouteToSecondAnchor` cannot reach the second anchor from the first.
   The walk uses temporary visited marks and checks reciprocal path connections.
9. `MapSeedFilledTerrainRegions` clears visited marks, searches eligible uniform
   four-by-four neighborhoods excluding the first three anchors, and requests
   class-`0x3000` growth according to generation parameter 4.
10. `MapBuildRegionEdgeShapes`, `MapBuildFilledTerrainShapes` (including
    `MapRefineFilledShapeMasks`), and `MapBuildDirectionalPathShapes` turn region
    classes and neighbor masks into base shapes.
11. `MapScatterTerrainFeatures` uses neighboring terrain and random thresholds to
    add single-cell or four-cell features and modify selected existing shapes.
12. `MapAssignTransitionChain` clears temporary descriptor bits, recursively searches
    for a chain of at least 25 visits, and retries with a minimum of 16. It avoids
    elevator/ship anchor cells (`MapTraceTransitionChain`).
13. `MapChooseMetatileAlternatives` chooses from count-prefixed ordinary alternatives
    or a transition descriptor's paired alternatives, then installs the final cell.
14. Successful loading restores the previous RNG state. A connectivity rejection
    returns before restoration; `MapLoadPlayerLevel` draws a replacement seed,
    invalidates the active bank identity, and retries.

`MapGenerateSeparatedAnchors` uses narrower coordinate bounds before level four;
it rejects a new anchor if both its X distance is below three and its Y distance
is below five from an earlier anchor. `MapCarveDirectionalPath` writes reciprocal
cardinal links and stops wandering after the requested random-step allowance.
`MapConnectAnchorPositions` first connects anchors zero to one and one to two
through `MapCarveDirectionalPath`, then extends a randomized path from each anchor.
The additional targets are 4–7 cells away horizontally and 5–8 vertically; bounds
and random direction tests can retry an anchor rather than accepting an invalid
target (`level_generation_and_changes.c`).

`MapGrowTerrainRegion` writes labelled two-by-two blocks. For class `0x1000`,
encountering another labelled region relabels every matching cell across the map.
It chooses preferred turns and retries untried cardinal directions through
`MapFindRegionGrowthOrigin` or `MapFindInteriorGrowthOrigin`; exhausting all four
directions returns immediately. Otherwise a random stop test ends growth only
when the requested minimum count for that terrain class has been reached
(`random_regions_and_cleanup.c`). These c026 bodies make stopping rules and
random-call order directly inspectable rather than inferred from their names.

## Terrain and live land changes

`GetMapTerrainAtWorldPosition` selects A/B from its player argument, derives a
128×64-world-unit metatile and an 8-pixel tile within it, fetches a base tile,
and returns the high terrain nibble. Outside upper/zero map bounds it returns
`TERRAIN_WATER` on levels 1/26 and `TERRAIN_VOID` elsewhere. The source notes that
negative input coordinates would index the map rather than taking that boundary path.

`UpdateLandAroundPlayer` skips absent/menu-open players and fills a five-by-five
neighborhood. Doorway tiles override ordinary terrain with `TERRAIN_GROUND`; otherwise
nonzero overlay tiles take precedence over the base metatile. `PLAYER_EARL` uses B
only when `PLAYER_TOEJAM` is active and the screen is split (`terrain_at_position.c`).

`TerrainValidation` searches rectangle origins within a metatile, checks tile
properties selected by flags, and mirrors X for `METATILE_MIRROR_X`. It has a
special full-metatile path. `SetPositionOnNewLevel` tries ten eligible random
metatiles, then scans cyclically; mode two positions both players 64 X units apart
and checks both positions against elevator footprints (`terrain_placement.c`).

`ProcessLand` examines marked cells under grounded, non-elevator players above
level one when adjacent terrain indicates an edge. `MapAnimateMarkedLandPaths`
freezes entity animation durations, consumes pending directions through
`MapConsumePathCellDirections`, animates each changed metatile, then restores the
durations. Same-level split views are both processed; a shared view instead copies
the changed map to the other bank. The caller rechecks Earthlings and player falls.

`MapQueueVisibleMetatileRow`/`MapQueueVisibleMetatileColumn` and `MapAnimateMetatilePathChange` synchronize
visible changes with camera transfer buffers and VBlank. `AllocateEntityWithEviction`
can reclaim mole effects, speech, shared tomatoes, item entities, and Earthling
secondary/primary objects to obtain animation resources (`land_processing.c`).

## Map-screen flow and interactions

`DrawGameMap` waits for outstanding rendering, replaces global tiles/palettes and
scroll settings, builds 80 hardware sprite records, draws the requested player's
bank, and waits for that player's button press/release. It is a blocking display,
not a concurrently simulated overlay. REV 02 additionally resets that player's
idle counter; the `REVISION` branch preserves REV 00's difference.

`MapDrawBackgroundAndTerrain` and `LoadMapIntoVRAM` draw base/overlay cells.
`DisplayMap` retains previously revealed cover, uses cached random choices for
additional reveals, then draws unrevealed blocks. Player, ship-piece, and elevator
markers use `Map_DrawPlayer`, `Map_DrawShipPiece`, and `Map_DrawElevator`;
`DrawMapCharacters` blinks markers from `gFrameCounter` bits 3/4.

`MapUploadCoverAnimationFrame` decodes both pieces of a `Spr_MapTileUncover`
frame, selecting interleaved or contiguous RLE from each format sign, and queues
the 20-tile uncover run into the selected tile bank. Frame seven applies
`VDP_TILE_PALETTE_3` to map sprites from index seven onward. It waits a VBlank,
copies the supplied hardware-sprite table to VRAM, then flushes the queued tiles
(`map_drawing_and_controller.c`). Piece access uses `render_frame_piece_view`
from `include/sprite_frame_views.h`, not a fixed twelve-piece ROM-frame extent.

On exit, `DrawGameMap` restores gameplay graphics/HUD/palettes or calls
`ShowShipPieces`, and redraws the partner's inventory if needed. Telephones invoke
this screen; doorway placement modifies the terrain cache; Earthlings and items
use map validation and level seeds; falls and elevators load/copy banks.

## ORIGINAL BUG list

- `StackBasedRandom`: negating the minimum 32-bit seed leaves it negative on the
  original machine; normalization therefore does not establish a positive state.
- `SetPositionOnNewLevel`: marked metatiles do not consume random attempts, so an
  all-marked map never reaches the exhaustive scan; if the scan finds no valid
  position, the last output coordinates are nevertheless used.
- `MapConsumePathCellDirections`: `shape` is uninitialized when no pending or
  adjacent direction supplies an edge.
- `MapAnimateMarkedLandPaths`: the first `EntityReserveLandPathAnimations` call
  passes uninitialized row/column values; that callee reads but does not use them.
- `MapInitializeScreenSprites`: a second tile-allocation failure triggers stronger
  eviction but no third allocation, leaving the tile base zero.

`MapWidenHorizontalRegionEdges` and `MapWidenVerticalRegionEdges` retain a
reset-to-one run-length quirk described as [INFERENCE] unintended in the source;
it is not an `ORIGINAL BUG:`-labelled finding and should remain distinct.

## Open questions

- Stage-dependent generation flags still require their consumers for interpretation;
  final terrain enum names do not describe every intermediate region label.
- Exact negative-coordinate behavior and invalid-data behavior are outside normal
  call assumptions; a safe native implementation requires an explicit policy.
- Reproducing a historical world requires the accepted replacement seed after a
  connectivity rejection, not merely the originally requested seed (`MapLoadPlayerLevel`).

## Port notes

- **Platform layer:** `DrawGameMap` writes VDP commands, tiles, sprites, and palettes;
  `MapQueueVisibleMetatileRow`/`MapQueueVisibleMetatileColumn` wait for transfers.
  [INFERENCE] Separate map simulation from rendering jobs and replace register
  access with a renderer interface.
- **Shared-screen coupling:** `ChangeLevel` owns A/B/C cache selection;
  `MapAnimateMarkedLandPaths` uses `gSplitScreenEnabled` and freezes entity animations;
  `DrawGameMap` blocks. [INFERENCE] Online menus need independent view state while
  shared terrain mutations remain authoritative.
- **Determinism:** `GetRandomNumber` truncates its return, `ChangeLevel` exchanges
  seeds, and `MapLoadPlayerLevel` reseeds rejected maps. `FurtherInitialisation`
  consumes idle draws. [INFERENCE] Preserve arithmetic/draw order for compatibility;
  remove CPU-speed-dependent idle consumption only as an explicit new seed policy.
- **Content tables:** `MapGetRandomGenerationParameter`, `MapChooseMetatileAlternatives`,
  and `TerrainValidation` consume generation/shape/property tables. [INFERENCE]
  Externalize and version those tables, fixed maps, and seeds so edits do not
  silently change an existing generated world.
- **Camera/visibility:** `MapAnimateMetatilePathChange` clips visible updates and
  `DrawGameMap` fixes map-screen layout; `LoadInEarthlings` controls activation.
  [INFERENCE] Widescreen can retain world dimensions but needs broader streaming
  and a deliberate activation policy without accidental RNG-consumption changes.
