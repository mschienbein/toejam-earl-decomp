# Camera: following, streamed tilemaps, and split views

## Purpose and evidence

Camera converts world/player motion into two possible view origins, maintains
scroll accumulators, and streams the newly exposed background rows and columns.
It also supplies the visible wipe and player-recentering effect used when the
core changes from a shared screen to two half-height views.

Evidence is the C/header implementation at the repository-root paths below.
`config/rev02/names.json` identifies `BuildCameraTileRow` and
`BuildCameraTileColumn`, but their bodies are absent from the represented source.
Their callers and buffers are documented without inventing map-composition rules.

## Files and roles

- `src/camera/camera_follow.c`: `GetPlayerCameraVelocity`, `UpdateCameraFollowing`.
- `src/camera/camera_scroll.c`: movement bounds, dirty-direction recording,
  column uploads, complete redraw, and immediate pending-upload flushes.
- `src/camera/camera_row_transfer.c`: row uploads and column clearing.
- `src/camera/metatile_tiles.c`: row clearing and metatile word lookup/mirroring.
- `src/camera/set_camera_position.c`: position setup and fixed-point synchronization.
- `src/camera/split_views.c`: `AnimatePlayersIntoSplitViews`.
- `src/camera/split_screen_wipe.c`: `AnimateSplitScreenWipe`.
- `src/camera/toggle_camera_flag.c`: `ToggleLOC_00FF8196`, tile-priority attribute.
- `include/ram_camera.h`: coordinates, upload buffers, cursors, and follow state.
- `include/graphics_calls.h`: call interfaces, including the two absent builders.
- `src/core/startup_and_main_loop.c`: mode transitions and interrupt-time scrolling;
  ownership remains core even though it is necessary context for this document.

## State and coordinate spaces

- `TOEJAM_CAMERA_NEWX/Y` and `EARL_CAMERA_NEWX/Y` are pending world-pixel origins.
- `P1_CAMERA_X/Y` and `P2_CAMERA_X/Y` are the committed display origins.
- `gCamera1FollowXFixed/YFixed` and camera-two equivalents are 24.8 accumulators.
- `SetCameraPosition` calls `SetCameraViewOrigin` before initializing those
  accumulators from the clamped coordinates.
- `gCamera1TileRowCursor/TileColumnCursor` and camera-two equivalents track the
  streamed tile position, initialized to `(pixel >> 3) - 1`.
- `TOEJAM_CAMERA_X` and `EARL_CAMERA_X` are pending tile-column state, not the
  same units as the similarly named `*_NEWX` pixel coordinates.
- `gCamera1PendingTileDirections` and camera-two equivalent encode up `1`,
  right `2`, down `4`, and left `8` (`MoveCamera*`).
- `gCamera1PendingTileRow` and its partner record the requested row index.
- Row upload storage is explicitly `gCameraRowUploadWords[2][45]`.
- Column upload storage is explicitly `gCameraColumnUploadWords[2][32]`.
- Per-camera metadata records upload pending state, count, and wrapped VRAM origin.
- `gCameraColumnUploadHeight` is 32 for shared view and 19 for split view.
- `gCameraColumnUploadHeightLowByte` aliases that word's low byte on 68000.
- `gCameraTileAttributes` is either zero or `0x8000` in its setter: tile priority.
- `SPLIT_SCREEN`, player lives, and `P1_MENU_OPEN[]` influence almost every policy.

## Following a player

`GetPlayerCameraVelocity` first returns zero for a dead/out-of-lives player or
one with a menu open. Otherwise it selects that player's pending camera origin.

For animation-controlled movement it scans sequence bytes until an `0xf*`
control byte, sums the referenced frame motion, multiplies by entity velocity,
and divides by frame phase times frame count. The low seven bits select frames.
For axes not controlled by animation it uses entity velocity directly, subject
to the observed position/blocked-state conditions.
Skip-physics, blocked axes, takeoff animation, and selected sprite states can
zero the result. Vertical velocity can subtract half the elevation velocity
when the player is not in the falling state (`camera_follow.c`).

The dead zones are explicit:

- One surviving player: center `(160,112)`.
- Two players sharing a screen: X range `50..270`, Y range `40..158`.
- Split view: center `(160,56)`.

When motion-derived velocity is zero but the player is outside its zone, the
function supplies a one-pixel fixed-point correction (`-256` or `256`) while
inside the tested camera bounds. These are camera decisions, not player forces.

## Combining the two players

`UpdateCameraFollowing` computes each player relative to its pending camera,
subtracting half the sprite's pixel height. Elevated flight/skates alter the
vertical point further: half elevation in split view, or full elevation for the
upper player in a shared view.

When both live players share a view with no menus, it combines their velocities
while checking the opposite player's dead-zone margin. Equal-direction motion
can select the stronger contribution; opposing contributions can cancel or add
according to those margins. This is not simply a midpoint camera.

It adds the result to the 24.8 accumulator and calls `MoveCamera*` repeatedly
until the integer target is reached or a move reports a world bound.
A blocked axis resets its accumulator to the actual origin, avoiding continued
fractional accumulation beyond the clamp.
In shared mode Earl's pending origin is copied from ToeJam's.
Otherwise the living-player/view branches advance the appropriate accumulators;
when ToeJam is absent the primary camera follows Earl.

## Pixel motion and streaming requests

`MoveCameraUp/Down/Left/Right` move exactly one world pixel per successful call.
They return one at a bound, zero otherwise.

- X bounds are `8..2360`.
- Shared-view Y bounds are `8..1560`.
- Split-view Y may extend to `1672`.
- If ToeJam is absent, the selected camera is forced to primary and the second
  pending origin is synchronized afterward.

On an eight-pixel tile boundary, the movement helpers first flush an already
pending upload of the same orientation, then record the new direction and row
or column. They do not upload each moved pixel.
`BuildPendingCameraTiles` consumes opposite-direction pairs independently:
up/down mask `5`, left/right mask `10`; a pair with both bits set is skipped.
It installs the pending cursor, calls the relevant builder, and clears dirty bits.

## Buffer construction and metatile lookup

`ClearCameraTileRow` fills 45 words with `gCameraTileAttributes` and marks ready.
`ClearCameraTileColumn` fills the selected upload height and marks ready.
These are observed blank-buffer primitives, not evidence that all terrain is blank.

`GetLevelMetatileTile` reads `LevelMetatileMaps[metatile][row][column]`.
Bit `0x80` of the metatile selects column `15-column` and toggles tile flip `0x800`.
`GetOverlayMetatileTile` treats overlay IDs as one-based; bit `0x10` selects
mirroring. Empty overlay words stay zero instead of acquiring a flip bit.
The lookup interfaces reveal 16-column metatile rows, not every map-format rule.

`RedrawCameraTileRows` iterates the viewport's buffered height, calls
`BuildCameraTileRow(1,camera)` and `UpdateCameraY`, restores the starting row cursor,
then clears pending row, column, and direction state.
`SetCameraViewOrigin` clamps position, chooses height, sets cursors, and redraws.

## VBlank upload and wrap handling

`UpdateCameraY` uploads each pending horizontal row, splitting at the
64-column VRAM-map wrap. It uses the primary or secondary plane-map command
and clears that camera's row-pending flag after the upload.
`UpdateCameraX` temporarily sets VDP auto-increment to 128 bytes, uploads each
pending vertical column, splits at 32 rows, then restores word auto-increment.
Both functions call `MoveDataToVRAM` with word counts.

`TryUpdateCameraY/X` temporarily hide the other camera's pending flag, set
`VBLANK_FLAG_3` and `_6`, wait one VBlank, gate ordinary display work, perform the
upload, restore the other flag, and restore display-service controls.
This is a bounded-buffer ownership protocol implemented using global flags and
video waits, not an independently running camera worker.
Normal core VBlank calls `UpdateCameraY` then `UpdateCameraX` when font animation
and higher-priority display tasks leave the ordinary graphics slot available.

## Split-screen presentation

`AnimateSplitScreenWipe` ensures both players have primary-view sprite slots,
releasing lower-priority entity resources until allocation succeeds.
It waits for both sprite frames, links the players for the wipe, then updates
paired map rows inward for 15 VBlank-synchronized steps.
It fills a 28-by-40 window destination and sets VDP window register state.

`AnimatePlayersIntoSplitViews` saves world positions, derives 26.6 interpolation
steps toward X center 160 and Y centers 56/168, and repeatedly positions and
uploads the sprites. It restores the original world positions at the end.
The temporary entity-coordinate edits are presentation work, not lasting movement.
[INFERENCE] Interleaving ordinary simulation or remote replication with this
blocking effect would expose transient positions unless explicitly isolated.

Core's `UpdateScreenSplitMode` supplies split/merge hysteresis and level/life
rules; core's HBlank handler switches lower-view scrolling and sprite chains.
See `docs/subsystems/core.md` for separation thresholds and interrupt-service precedence.

## Interactions

- Player animation frames influence predicted camera velocity.
- Entity visibility and sprite coordinates use camera origins; camera is not
  merely a final renderer transform (`src/sprites/entity_visibility.c`).
- Menus suppress following and prevent ordinary shared-view merging.
- Map builders supply terrain words; metatile tables supply content.
- HUD and sprite-chain state must change together during a mode transition.
- Core's odd/even schedule separates following, tile building, and VBlank upload.

## ORIGINAL BUG list

No `ORIGINAL BUG:` comments occur in the represented camera files. This is not
a claim that every edge case has been proven safe; the absent builders are
outside that evidence.

## Open questions

- The two builder bodies are absent from the supplied integration source tree;
  overlay precedence, level selection, and out-of-world tile rules remain open.
- `ToggleLOC_00FF8196` retains an address-era name despite its observed priority role.
- [INFERENCE] A zero-distance split interpolation may need special treatment in
  a redesigned implementation; no new original-bug classification is asserted.
- View-size constants are scattered between camera, core, and sprite/UI code.

## Port notes

- Platform layer: replace `TryUpdateCameraX` and `TryUpdateCameraY` VBlank/VDP
  synchronization with renderer viewports and explicit upload ownership.
- Shared-screen coupling: separate `SPLIT_SCREEN` and `P1_MENU_OPEN` view policy
  from each local/remote player's camera/menu context; one inventory must not
  stall another camera's simulation.
- Determinism: keep `UpdateCameraFollowing` interpolation/render cadence outside
  authoritative fixed-step motion; audit camera-dependent visibility first.
- Content tables: make metatile maps, overlays, blank/priority tile policy, and
  viewport dimensions resource/configuration data rather than packed VRAM words.
- Camera/visibility: rederive `GetPlayerCameraVelocity` dead zones, streaming
  extents, clamps, split thresholds, and sprite culling together for widescreen.
