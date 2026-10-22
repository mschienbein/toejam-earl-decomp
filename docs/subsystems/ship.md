# Ship pieces and the ship screen

## Purpose and evidence boundary

The ship subsystem assigns ten missing pieces to levels, responds to collecting a
piece, displays progress/name/summary screens, and performs the completion transition
into the ending and final map (`AssignShipPieceLevels`, `ShipPieceAbility`,
`ShowMissingShipPieceSummary`, `PlayShipCompletionSequence`).

Source paths describe the repository layout. Two files live under `src/ship`;
bare C filenames below abbreviate that directory. The mailbox predicates belong
to `src/earthlings/mailbox_earthling.c` and are covered in the Earthlings document,
not as ship-piece logic.

`ShowShipPieces` now has a matched body in
`src/ship/ship_screen_and_collection.c`. The complete unit exposes the collection
mutation, composed display, restoration, and completion branches rather than
requiring those steps to be inferred from callers.

Content/entity identifiers come from `include/earthling_types.h`,
`include/present_types.h`, and `include/entities.h`.

## Files and their roles

- `src/ship/piece_level_assignment.c`: `AssignShipPieceLevels`; fixed-world and
  randomized ten-piece level assignments.
- `src/ship/ship_screen_and_collection.c`: ship-piece collection and notices,
  silhouette/background animation, tilemap/palette setup, full piece composition,
  piece-name/summary text, timed/input dismissal, gameplay restoration, and the
  completion/ending transaction.
- `include/ship.h` is the public K&R call interface; it also declares the mailbox
  helpers implemented in `src/earthlings/mailbox_earthling.c`.

## Key data and RAM state

`include/ram_ship.h` distinguishes two ten-byte arrays with very different roles:

- `gShipPieceLevelAssignments[10]` maps piece index to the level containing it.
  Zero marks an already absent/collected piece; the initializer writes nonzero
  level assignments, and summaries count nonzero entries.
- `gShipScreenState[10]` is display state, not a second copy of that assignment
  table. Byte zero holds the selected background frame in the shown animation
  routines; byte four is used as a pending-piece notice; named views of byte two
  and byte six are the background timer and indicator entity handle.

`gShipBackgroundAnimationTimer` and `gShipPieceIndicatorEntity` are existing aliases
of display-state bytes, not separately allocated fields (`include/ram_ship.h`).
This distinction matters because historical/address-derived display names can
otherwise be mistaken for the piece-to-level assignment table.

`include/assets_scenes.h` declares `FixedWorldShipPieces[10]`,
`TransitionBackgroundTileOffsets[8][8]`, `Spr_TransitionBackground[50]`, and
`IntroAndShipAnimationPointers[2]`. Only pointer-word slot one is proved to be a
frame-table pointer; the header deliberately leaves the other slot opaque.

Frames/pieces use `render_frame` and `render_piece` from `include/sprites.h`.
A frame has a four-byte prefix followed by its own count of pieces, not a universal
fixed twelve-piece tail. Ship-background upload code consumes eight pieces;
transition upload code consumes four (`AnimateShipScreenBackground`,
`AnimateTransitionBackground`). Their shared access views are declared in
`include/sprite_frame_views.h`; the twelve/four member bounds are not fixed ROM
frame allocations or strides.

`include/ram_sprites.h` supplies the shared unpack buffer, upload-run queue, and
hardware sprite table. `include/ram_system.h` supplies `gFrameCounter`, rendering
flags, and music state. Camera, player, effects, speech, and inventory state come
from their respective `ram_*.h` headers; ship presentation does not own independent
copies of those systems.

## Assigning pieces to the world

`AssignShipPieceLevels` uses `gWorldType` from `include/ram_map.h`. Nonzero selects
the ten entries in `FixedWorldShipPieces` and returns. Random-world assignment is:

1. For each of five groups, choose a currently unassigned piece index.
2. Draw its level from that group's five-level band, retrying values outside 2–24.
   The bands are 1–5, 6–10, 11–15, 16–20, and 21–25 before that rejection.
3. Choose the remaining five piece indexes, assigning each a random level 2–24
   that does not equal any assignment already in the table.
4. Replace one randomly selected piece's assignment with level 25.

The function tests existing nonzero entries when choosing free piece slots; it does
not clear the table itself. Correct initialization is therefore a caller/preexisting
RAM-state requirement, not work hidden inside this function.

`SetupInitialEarthlings` calls the assignment routine after initializing/retaining
level-one Earthling banks (`src/earthlings/spawning_and_banks.c`).
`EarthlingRetainLevelBank` checks the assignment table when
building another level and creates an `EARTHLING_SHIP_PIECE` record at the ship anchor
when a piece belongs there. Thus world ship pieces reuse Earthling storage,
visibility, allocation, and contact dispatch rather than a separate ship entity bank.

`MapGetShipPieceWorldPosition` converts the ship anchor map cell to world position.
When that level is the highest reached, retaining its Earthling bank sets
`gShipScreenState[4]` for a notice (`spawning_and_banks.c`). `Map_DrawShipPiece` in
`src/map/map_drawing_and_controller.c` locates the `EARTHLING_SHIP_PIECE` record among the first two
loaded Earthlings and draws its map icon.

## Collection and pending indication

`ShipPieceAbility` requires the record to have a display object. It finds the piece
whose assigned level equals the collector's current level, frees the displayed
object, marks the Earthling kind `EARTHLING_UNUSED`, clears its handle, and invokes
`ShowShipPieces` with that piece and player (`ship_screen_and_collection.c`).

After the display call it starts dialogue 2 for player one or a shared screen;
player two on a split screen instead receives the short "Jammin" speech. The
function delegates assignment mutation to `ShowShipPieces`, which clears the
selected nonnegative piece entry before composing the display.

`ShipUpdatePendingPieceIndicator` runs from `ProcessEarthlings`. An existing notice
entity is released if it is no longer at Y = camera Y + 224. A pending notice
allocates a 96×32, three-sprite, 48-tile object only outside split-screen, positioned
at camera X + 160 and that fixed Y row with height 112. The pending flag is cleared
whether or not split-screen allowed a notice (`ship_screen_and_collection.c`).

`ShowRandomUncollectedShipPiece` starts from the low byte of a random draw, advances
cyclically modulo ten until finding a nonzero assignment, and calls
`ShowShipPieces` with player selector two. That selector also enables either pad
in `WaitForShipScreenAcknowledgement`; it is not a third player record.

## Ship-screen rendering and acknowledgement

`ShowShipPieces` disables gameplay graphics, flushes outstanding tiles, initializes
the ship screen, and clears a nonnegative selected piece from
`gShipPieceLevelAssignments`. It uses frame-table slot one of
`IntroAndShipAnimationPointers` and draws collected pieces first, then uncollected
pieces. Each piece is unpacked and queued; uncollected pixels become silhouettes,
while the selected piece uses palette three. The generated hardware pieces are
linked and uploaded before the name/summary display.

If any assignment remains nonzero, it waits for acknowledgement, restores gameplay
graphics, and redraws open inventories. If none remain, it calls
`PlayShipCompletionSequence`. A negative piece selector displays progress without
changing the assignment table; player selector two suppresses the player-specific
summary and enables either pad for acknowledgement.

`InitializeShipScreenTilemapAndPalettes` fills four eight-row background pages,
constructs a random star border on a 40×28 screen, selects scroll/window registers,
installs ship palettes plus a black palette slot, changes a VDP mode register,
sets the shared graphics flag, and forces the first animated frame.

`AnimateShipScreenBackground` alternates two selected frames every twelve calls.
A forced call selects its first frame immediately. It unpacks eight pieces into
shared sprite tile memory and queues a 128-tile upload. Calls without a frame change
still wait one VBlank and animate font banks. The animation delay is therefore a
call/tick policy, not a duration independent of caller execution.

`FillSpriteOpaquePixels(tile_count)` changes each nonzero packed pixel nibble in the
current upload-buffer region to palette index fifteen; zero stays transparent.
`ShowShipPieces` calls it on uncollected pieces, directly establishing that
silhouette treatment.

`ShowShipPieceName` maps indexes 0–9 to ten embedded names, centers the chosen string
using its character count, and animates for 90 calls. `ShowMissingShipPieceSummary`
counts remaining assignments, checks whether one matches the requested player's
level, and chooses same-level, ToeJam-level, or Earl-level wording.

`WaitForShipScreenAcknowledgement` resets the shared time counter and animates until
240 ticks elapse or A/B/C is pressed on an enabled pad. It also waits for the pressed
buttons to be released while continuing animation. `PLAYER_TOEJAM`/`PLAYER_EARL`
enable their respective pad; selector two enables both (`ship_screen_and_collection.c`).

`RestoreGameplayGraphics` restores level/starfield/inventory-label graphics, the HUD,
the hardware sprite table, all four gameplay palettes, graphics flags, and a VDP
mode. `DrawGameMap` can call `ShowShipPieces` with piece selector -1 on map exit,
showing that the two blocking displays share global presentation resources.

## Completion flow and interactions

`PlayShipCompletionSequence` builds a transition background and randomized border,
resets time, selects `MUSIC_TRANSITION`, animates transition background 20 for 480
ticks, and runs a staged sound sequence. Sound-driver cooldowns gate transitions
between claps, "Hallelujah", and "Jammin" (`ship_screen_and_collection.c`).

It then restores gameplay graphics, explicitly closes the partner's open inventory,
selects the primary sprite chain, clears split-screen, hides player HUD sprites,
and sets both powerup identities to `PRESENT_NONE`. It clears active level objects, runs
`PlayEndingSequence`, and enters the final map through `MapEnterFinalLevel`.
The ending's detailed actors belong to cutscenes; the resource handoff belongs here.

Important cross-subsystem boundaries are explicit: Earthlings host collectible
pieces; map generation supplies anchors; inventory/map screens share tile resources;
audio cooldowns affect the completion timeline; elevator helpers own final-map
entry and cleanup. `GamePaused` in `src/ui/pause.c` can also rewrite/clear piece
assignments in its special input path, so not every mutation is ordinary collection.

## ORIGINAL BUG list

- `ShowRandomUncollectedShipPiece`: the search has no exit when all ten assignments
  are zero; there is no remaining-piece guard inside the routine.
- `InitializeShipScreenTilemapAndPalettes`: an attempted `page == 4` early exit lies
  inside a loop restricted to page < 4, so it cannot run. Its row test is before
  row initialization, but short-circuiting the impossible page test prevents that
  row operand from being evaluated on this path.
- `ShowShipPieceName`: an out-of-range piece index leaves the name pointer
  uninitialized before the string-length loop.

These are the `ORIGINAL BUG:` findings owned by the inspected ship source files.
Earthling-owned findings are documented in the Earthlings subsystem.

## Open questions

- `IntroAndShipAnimationPointers[0]` has no established pointee type; treating the
  pair as an ordinary sequence/frame descriptor would assert more than the header.
- The collection scan assumes some piece belongs to the player's level; its body
  can pass index ten onward otherwise. The source has no `ORIGINAL BUG:` comment
  for that case, and reachable invalid-state behavior is not established here.

## Port notes

- **Platform layer:** `InitializeShipScreenTilemapAndPalettes` writes VDP state;
  `AnimateShipScreenBackground` unpacks/uploads tiles and waits for VBlank.
  [INFERENCE] Preserve logical display events while replacing packed-tile work
  with platform textures, audio, and UI services.
- **Shared-screen coupling:** `PlayShipCompletionSequence` closes the partner's
  `gPlayerInventoryOpen`, clears `gSplitScreenEnabled`, and cancels both powers. [INFERENCE]
  Per-player progress menus should not rewrite the partner's view; completion
  should instead be an explicit shared gameplay transition.
- **Determinism:** `AssignShipPieceLevels` consumes RNG and
  `WaitForShipScreenAcknowledgement` resets `gFrameCounter`. [INFERENCE] Separate
  world assignment RNG from cosmetic display RNG, and serialize
  `gShipPieceLevelAssignments`/collected state when reproducing seeded worlds.
- **Content tables:** `AssignShipPieceLevels` reads fixed assignments;
  `ShowShipPieceName` embeds names and `PlayShipCompletionSequence` embeds a timeline.
  [INFERENCE] Externalize those inputs/assets with stable piece IDs rather than
  depending on a ten-way string switch.
- **Camera/visibility:** `ShipUpdatePendingPieceIndicator` uses one camera and
  suppresses creation under `gSplitScreenEnabled`; screen setup fixes a 40×28-tile layout.
  [INFERENCE] Widescreen needs anchored layouts and per-view notices, while
  collectible activation follows `LoadInEarthlings`, not UI dimensions.
