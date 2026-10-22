# Core: reset, interrupts, and the game scheduler

## Purpose and evidence

The core owns machine start-up, global display synchronization, input sampling,
and the alternating gameplay schedule. It also owns the decision to share or
split the display; the camera subsystem supplies the scrolling implementation.

C evidence is `src/core/startup_and_main_loop.c` and
`include/ram_system.h`; assembly evidence is `src/asm/startup.s` and
`src/asm/interrupts.s`. All source paths are repository-root paths in the
current subsystem layout. Function names are the source/canonical identifiers.
`README.md` summarizes current progress; `docs/STATUS.md` retains its history.

## Files and roles

- `src/core/startup_and_main_loop.c`: VDP initialization, controller mapping,
  interrupt work, input dispatch, camera-mode transitions, and the permanent loop.
- `src/core/error_trap.c`: `ErrorTrap`, an infinite loop for unhandled exceptions.
- `src/asm/startup.s`: `_start`, the reset-time Sega machine initialization.
- `src/asm/interrupts.s`: `GameProgram`, interrupt masks, exception wrappers,
  and `RestartFromResetEntry`.
- `include/core_calls.h`: the K&R call boundary shared with other subsystems.
- `include/ram_system.h`: frame counter, input aliases, VBlank controls, RNG state.
- `include/hardware.h`: typed-width hardware accesses and packed VDP commands.
- `include/entities.h`: player entity fields consumed by input and view decisions.
- `src/camera/toggle_camera_flag.c` owns the tile-priority attribute setter.

## Key RAM state

- `gFrameCounter` is an unsigned short incremented by normal `HandleVBlank`.
- `gFrameCounterLowByte` aliases its low byte on the big-endian target; it is
  not a separately advanced input clock (`ram_system.h`).
- `gVBlankFunc` replaces the ordinary VBlank body when non-null.
- `gVBlankGraphicsEnabled` and `gHBlankGraphicsEnabled` enable the corresponding
  gameplay display work in these C handlers.
- Sprite-upload, graphics-pause, inventory-phase, HUD-redraw, and music-selection
  variables are separate shared controls, not a general-purpose bitfield or queue.
- `gTerrainPriorityUpdatePending` requests the special map-priority service path.
- `gController1RawInput` and `gController2RawInput` are physical-controller samples.
- `gPlayerProcessedInput[]` and `gPlayerPreviousProcessedInput[]` are logical-player inputs.
- `gPlayer2PreviousProcessedInput` aliases the second previous-input byte.
- `gControllerPortsSwapped` swaps controllers, not the player records.
- `gControllerButtonsAcSwapped` exchanges the `0x20` and `0x40` button bits.
- `gMode2RegSoftcopy` mirrors VDP mode state; bit `0x40` gates display-sensitive waits.
- `gPlayer2InventoryOpen` aliases `gPlayerInventoryOpen[1]`, not independent interrupt storage.
- `gSplitScreenEnabled`, current camera coordinates, and target camera coordinates live
  in `ram_camera.h`; menu-open bytes live in `ram_ui.h`.
- `gCachedRandomValues[59]`, `gRandomCacheIndex`, and `gRNGState` are shared RNG state.

## Reset and boot flow

1. `_start` tests existing controller I/O control state before taking the full
   hardware-reset path (`startup.s`).
2. Full reset consumes the typed `SetupValues` command stream and port pointers.
3. A nonzero hardware version nibble causes the TMSS SEGA signature write.
4. The reset path initializes 24 VDP registers, obtains the Z80 bus, installs
   initial Z80 bytes, clears mirrored work RAM, CRAM, and VSRAM, and silences PSG.
5. `_start` enters `GameProgram`, also used after the abbreviated reset path.
6. `GameProgram` reads VDP status, masks interrupts, clears all 64 KiB of work RAM,
   and calls `VDPInitialisation` (`interrupts.s`).
7. `VDPInitialisation` calls the ordinary C `ResetVDP`, then `FurtherInitialisation`.
8. `ResetVDP` writes the register sequence selecting the gameplay plane/window,
   sprite, scrolling, and DMA arrangement (`startup_and_main_loop.c`).

`DisableAllInterrupts` changes only the SR interrupt-priority bits to seven;
`EnableAllInterrupts` clears those bits. Level seven remains non-maskable.
`RestartFromResetEntry` masks interrupts, reloads the cartridge's initial stack
pointer, and jumps to `_start`; it is not merely a return to the title screen.

## High-level initialization

`FurtherInitialisation` performs this observable ordering:

1. Establish the mode-register soft copy and disable gameplay interrupt work.
2. Initialize DMA/RAM support, reset Z80, load the sound driver, service sound once.
3. Enable interrupts and wait for a VBlank before disabling visible display.
4. Seed RNG with `12345`, pre-generate its cache, and clear palette/scroll/video RAM.
5. Upload font and starfield graphics; create writable font banks and RAM indices.
6. Run `SEGAAndIntroVideoRoutine`, then `NewGameMainMenu`.
7. Upload level, inventory-label, and HUD graphics and draw the starfield plane.
8. Load gameplay palettes, set up players/game state, and position the first level.
9. Set the camera flag, select initial shared/split mode, and reseed RNG using
   `GetRandomNumber() | ((long)gFrameCounter << 16)` before generating level seeds.
10. Draw HUD and initialize elevators, inventory, and earthlings.
11. Sample input, publish camera coordinates, enable display and gameplay handlers.

`DrawStarfieldPlane` fills a 32-by-32 source map with repeated tile writes.
`DrawLevelGraphics` cycles four palette colors once per four calls, selecting
between two palette-animation tables according to level `< 26` versus `>= 26`.

## Normal frame scheduler

The permanent loop snapshots `gFrameCounter` at the start of each iteration.
Its local `frame` counter selects three different work groups:

- Odd iterations: `UpdateScreenSplitMode`, `UpdateCameraFollowing`, `ProcessLand`,
  then `UpdateAndQueueEntitySprites`.
- Even iterations with bit two set: build pending camera tiles, process earthlings,
  elevator visibility/elevators, item/present updates, and tomato rain.
- Other even iterations: build pending camera tiles, update land around both
  players, process input, process players, and process entity flag `0x2f` work.
- Both even cases then run movement, per-viewport sprites, animation motion,
  and speech/dialogue updates.

Finally it repeatedly calls `GetRandomNumber` until `gFrameCounter` changes.
This is observable RNG consumption during spare CPU time, not an inert wait.
[INFERENCE] A native fixed-step loop cannot reproduce the original random stream
merely by keeping the initial seed and the visible update-call order.

## VBlank and HBlank work

`Level6Isr` saves all data/address registers around `HandleVBlank`.
`Level4Isr` saves the C-call volatile register set around `HandleHBlank`.

When `gVBlankFunc` exists, `HandleVBlank` calls it instead of the ordinary body;
the replacement is responsible for its own timing behavior.
Otherwise the gameplay branch can publish target camera coordinates, select a
sprite chain, program the horizontal interrupt line, and establish upper-view
window/plane and parallax scrolling.

Display-service precedence inside that branch is:

1. `gTerrainPriorityUpdatePending`: `MapClearTilePriorityNearPlayer`, then frame increment.
2. If `gVBlankGraphicsPaused` is clear, `gInventoryRedrawPhase`: inventory redraw service.
3. Otherwise `gSpriteGraphicsUploadPending`: sprite loading and viewport-chain selection.
4. Otherwise `gHudRedrawPending`: lives tiles and complete HUD redraw.
5. Otherwise font-bank animation; if idle, palette, camera rows/columns, and
   both health bars; alternate player sprite hiding each such call.

Outside the display branch, unless `gVBlankGraphicsPaused` blocks it without
`gVBlankAudioInputOverride`, VBlank changes music-template bank if
`gRequestedMusicSelection != gCurrentMusicSelection`, otherwise runs
`MusicUpdateSequencer`, then reads one physical controller according to the
frame-counter low-bit alias.
The ordinary body increments `gFrameCounter` even when gameplay display work is off.

`HandleHBlank` selects the lower-view sprite chain when enabled on gameplay levels.
It switches to the lower menu window or plane A base/scrolling as appropriate,
including the `112`-pixel vertical split offset, then disables further line work
for that frame with the final VDP register write.

## Input and shared-view decisions

`GetPlayerInput` maps controllers and suppresses buttons according to player
animation groups, reaction/ghost states, stationary/falling states, and skates.
`ProcessInput` checks A, Start, then gameplay B/map input before player movement.
Map exploration writes `gMapVisitedCoverBits`, awards one point, and also awards
the partner when both are alive and sharing the screen.
`CheckStartButton` invokes global `GamePaused` only on a new Start press.

`PlayersRequireSeparateViews` checks living players, differing levels, absolute
separation, and extra vertical extent for elevated flight/skating states.
`UpdateScreenSplitMode` uses `>124` vertical or `>220` horizontal separation to
split; merging requires `<104` and `<200` and neither menu open.
That hysteresis is separate from camera-follow limits.
Forced updates also handle a missing player and differing levels.
`EnterSplitScreen`, `EnterSharedScreen`, and `CenterSharedCameraOnPlayer` disable
or gate display work around tilemap/HUD changes and then restore it.

## Interactions

- Camera prepares targets and tile buffers; core publishes them during VBlank.
- Sprites supplies chain selection and loading; HBlank changes visible chain.
- UI/map operations temporarily replace ordinary display-service priorities.
- Audio sequencer service occurs in VBlank, not in the main gameplay groups.
- Player simulation, earthling simulation, item simulation, and rendering use
  different phases; treating every loop iteration as one full game tick is wrong.
- Cutscenes can replace `gVBlankFunc` and use the same global frame counter.

## ORIGINAL BUG list

- `FurtherInitialisation`: `frame` is read uninitialized. The source comment
  observes the allocated register previously held `gFrameCounter << 16`; any
  historical reliance on that residue is explicitly [INFERENCE].

## Open questions

- The source does not define an authoritative real-time duration for a simulation
  tick independent of video mode; a port must choose one explicitly.
- [INFERENCE] The exact idle-wait random-call count is hardware/load dependent;
  no cross-platform deterministic equivalence follows from this code alone.
- The boot exception trap has no diagnostic recovery contract beyond looping.

## Port notes

- Platform layer: replace reset, SR masks, exception entry, VDP accesses, Z80 bus,
  and controller polling; retain game initialization ordering as an explicit flow.
- Shared-screen coupling: global pause, `gSplitScreenEnabled`, window aliases, and
  VBlank service flags must not become network-wide menu locks. Per-player menus
  need separate UI state while partner/world simulation continues.
- Determinism: use an explicit fixed simulation schedule, versioned RNG algorithm,
  and saved RNG state. Remove wall-clock/frame-wait RNG consumption deliberately;
  do not claim original-seed identity after doing so.
- Content tables: fonts, palettes, starfield maps, level tiles, and labels are
  resource inputs, not native-port executable constants.
- Camera/visibility: widescreen changes separation thresholds, exploration
  point-sharing conditions, visible sprite sets, and possibly spawn decisions;
  separate gameplay policy from presentation dimensions before changing them.
