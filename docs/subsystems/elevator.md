# Elevators and level rides

## Purpose

Elevators are persistent level records with separately allocated display entities,
not just player animations. They open around nearby players, enforce their threshold
geometry, attach riders, coordinate departure/arrival state machines, and move riders
between levels. A special shared ride scene temporarily replaces normal VBlank work
(`SetupElevator`, `ProcessElevators`, `ElevatorAdvanceRidersToNextLevel`).

Source paths describe the repository layout in `src/elevator`; identifiers match
the shared headers and canonical identities in `config/rev02/names.json`.
Elevator state numbers have no dedicated constants; narrative roles are
[INFERENCE] where not explicit.

## Files and roles

- `src/elevator/elevator_scene.c`: record setup, animation installation, entity-pair
  allocation/visibility, collision predicates, final-level setup, ride graphics,
  ride dialogue/VBlank, and the actual cross-level transaction.
- `src/elevator/elevator_transitions.c`: proximity/input detection, accelerating
  departure height cycles, departure and arrival state machines, and their order.
- `src/elevator/elevator_query.c`: `ProcessElevator`, a state-3 query despite its name.
- Related ownership: map generation fills anchor cells; players own their current
  elevator identifiers; map rendering draws an icon in `src/map/elevator_icon.c`.

## Data structures and RAM

`include/elevator.h` defines the eight-byte `elevator_record`: world `x/y`, `level`,
`state`, display `entity` handle, and `timer`. `elevator_position` is a two-byte
map-cell coordinate pair, not a pair of world-position shorts.

`include/ram_elevator.h` declares `gElevatorRecords[2][2]`. The first index selects
one of two retained level pairs; member zero is used for departure processing and
member one for arrival processing (`ProcessElevatorStep1`/`ProcessElevatorStep2`).
A public ID is one plus twice the pair index plus the member index.
Zero in `gPlayerElevatorFlags[player]` means no attached
elevator; IDs 1/3 are departure records and 2/4 arrival records.

`gPrimaryArrivalElevatorLevel` aliases `gElevatorRecords[0][1].level`, and
`gPlayer2ElevatorId` aliases `gPlayerElevatorFlags[1]`; these are not independent state.
`gSecondaryArrivalElevatorLevel` is used alongside the first arrival-level view when
deciding whether a blocked departure can become active (`ProcessElevatorStep1`).

`gLevelAnchorPositions[8]` holds generated map-cell anchors. Arrival and ship-spawn
coordinate names alias selected bytes in this table (`include/ram_elevator.h`).
`SetupElevator` uses map-cell coordinates for ordinary levels and fixed cells for
level one and level 26. It converts them with 128-world-unit X and 64-world-unit Y
strides, then applies fixed offsets. `PlayerSetPositionFromSavedMapCell` uses the
same conversion for the arrival anchor.

`gElevatorDepartureInProgress`, per-player `gPlayerElevatorApproachInputGate`,
`gArrivalElevatorCloseDelay`, `gDepartureElevatorShakeCycles`, and `gDepartureElevatorShakeDelay` coordinate
transitions. The latter timers are global, not fields in each independent elevator
record (`include/ram_elevator.h`, `elevator_transitions.c`).

Ride state includes an object pointer, fixed-point scroll position and its existing
low-word alias, velocity, acceleration, four speech handles, and an already-used
dialogue bitmask. `gFrameCounter` and `gVBlankFunc` are shared system state in
`include/ram_system.h`; the ride resets and owns them while installed.

The two display halves use `object_state` from `include/entities.h`. The primary
handle is slot 2 or 3; the secondary half is exactly two records later, slot 4 or 5.
`ElevatorPrimaryAnimations` and `ElevatorSecondaryAnimations` are timed-animation
arrays declared in `include/assets_sprites.h`; state indexes both tables.
Riders are indexed by `PLAYER_TOEJAM`/`PLAYER_EARL`. The two halves' `draw_flags`
and `behavior_flags` use the named entity policies in `include/entities.h`;
`animation_motion_axes` delegates motion axes, while `owner` and attachment fields
retain shared effect meanings rather than elevator-specific record layouts.
The ride-background uploader accesses its four pieces through
`render_frame_four_piece_view` in `include/sprite_frame_views.h`; that is a
bounded consumer view, not a fixed ROM-frame allocation or stride.

## Setup, visibility, and threshold flow

`SetupInitialElevator` installs the level-one departure and invalidates the other
three records. `ElevatorInvalidateSecondaryRecords` separately invalidates both
arrival records by assigning state 11 and level -1 (`elevator_scene.c`).

`SetupElevator` first reuses an existing matching-level record. When
players occupy different levels it can choose the alternate retained record so it
does not replace the partner's current level. Replacing a record frees its two
display entities. Departure setup selects state 13 if an arrival on level+1 still
exists, otherwise state 0; arrival setup selects state 8.

`ElevatorSetState` always writes the record state. State 11 also sets level -1 and
returns. Other states install both animation sequences/frame tables and reset their
animation counters when entities exist; the secondary descriptor supplies its Y
offset. A logical state change can therefore precede entity allocation.

`ElevatorUpdateEntityVisibility` allocates near either active camera and releases
entities outside both camera regions or in states 11/13. The explicit original
bounds are X in (-160,480), Y in (-112,336), relative to the pending camera; the
second camera is considered only with `gSplitScreenEnabled`. If both fixed display pairs
are occupied, `ElevatorCreateEntityPair` simply returns without allocating.

`ElevatorBlocksPlayerMovement` compares proposed movement against every active,
same-level threshold. Inside riders must stay centered in X, may exit only in
state 3, and clear their ID after moving sufficiently below the doorway. Outside
players are blocked when crossing the threshold or approaching it while elevated;
a grounded, centered player entering state 3 can acquire the elevator ID.

`ElevatorBlocksEntityMovement` checks crossing for other objects without attaching
them. `ElevatorContainsPosition` tests footprint overlap but intentionally does not
filter state; placement callers use it to avoid elevator regions. `ProcessElevator`
decodes an ID and returns whether its record is in state 3, not whether any
transition work ran (`elevator_query.c`).

## Update order and state machines

The gameplay loop calls `ElevatorUpdateEntityVisibility` before `ProcessElevators`
on its Earthling/item quarter-phase. `ProcessElevators` always runs departure
processing before arrival processing (`FurtherInitialisation`,
`src/core/startup_and_main_loop.c`; `elevator_transitions.c`).

`ElevatorCheckPlayerProximityAndInput` considers same-level players. Attached riders'
vertical input keeps the elevator active; approaching players in the narrow entry
strip set their own `gPlayerElevatorApproachInputGate` and count as nearby.

Departure transitions in `ProcessElevatorStep1`:

- State 11 is inactive. State 13 waits until neither arrival record occupies
  level+1, then switches to 10 and can push a player away from the doorway.
- State 10 waits for the primary animation, then chooses 1 with a rider or 0
  without one, clearing the attached player's `ENTITY_HIDDEN` behavior bit.
- Proximity moves eligible states to 2; secondary animation completion moves
  2 to 3. Loss of proximity initiates 4, whose completion returns to 1 or 0.
- Departure readiness requires both attached players with both menus closed, or
  an attached player on a lower level than the partner with that rider's menu
  closed. Thus one rider can progress while the other remains on a higher level.
- Ready state 3 first goes to 4; ready state 1 goes to 6, sets the shared
  departure flag, and sets attached riders' `ENTITY_HIDDEN` behavior bit.
- State 6 waits for its secondary animation, then enters 5 and initializes five
  height cycles. If every active player rides, it also requests music reset.
- State 5 repeatedly raises the secondary half four units, wraps above height 28,
  and shortens the delay with each cycle (`ElevatorStepDepartureHeightCycles`).
- Cycle completion selects 7; its animation completion chooses 10 if same-level
  players have different elevator IDs, otherwise 11 and the cross-level transaction.

Arrival transitions in `ProcessElevatorStep2`:

- State 8 waits for the primary animation, clears rider `ENTITY_HIDDEN`, pushes
  any non-rider away from the threshold, plays the arrival sound, and goes to 1.
- Near/input-driven opening follows 2→3; closing state 4 selects 1 with a rider,
  11 with no entity, or 0 plus a 30-update countdown otherwise.
- State 0 counts down to 9; completion of 9 invalidates the record with state 11.
- Both state machines keep attached players' X centered and can stop the players'
  resting animation at a specific frame (`ProcessElevatorStep1`/`ProcessElevatorStep2`).

## Crossing levels and the shared ride

`ElevatorAdvanceRidersToNextLevel` releases each rider's Earthling/item bank
reference, increments that player's level, then loads/references/populates the
destination. Non-riders are not incremented. It sets up destination departure and
arrival records, places riders at the arrival center, refreshes HUD level digits,
places telephones, updates split mode, and clears the shared departure flag.

Only when every active player is riding does it wait for `MUSIC_TRANSITION` and
call `ElevatorBeginRideScene`. That scene centers the elevator, installs state 12,
clears transient entities, waits for sprite readiness, replaces background/scroll/
palette state, hides the HUD, resets `gFrameCounter`, and installs
`ElevatorRideVBlank` as the interrupt override (`elevator_scene.c`).

`ElevatorRideVBlank` integrates acceleration, speed, and scrolling; updates dialogue;
alternates the two displayed elevator halves every sixteen ticks; runs the shared
hardware routine; then advances time. `ElevatorEndRideScene` waits for peak speed,
sets negative acceleration, waits for speed zero, removes the override, clears
speech, and restores normal tile/palette resources.

`ElevatorPrepareRideDialogue` requires both players and a 75%-threshold random
choice. It selects one of fourteen exchanges not already in the used mask and
creates up to four hidden speech lines. `ElevatorUpdateRideDialogue` reveals/hides
lines at fixed counter values; missing second lines explicitly advance the global
counter to skip empty time. These are not free-running independent speech timers.

`MapClearActiveLevelObjects`, `MapEnterFinalLevel`, and `LoadFinalLevelMap` are
shared transition helpers housed here: they release world resources, move active
players to level 26, reset movement/fall state, load Funkatron graphics/palettes,
set fixed positions, and rebuild the shared/split presentation.

## Interactions with other subsystems

- Map generation supplies elevator anchors; `MapLoadPlayerLevel` supplies geometry.
- Earthling/item bank retain/release brackets riding; each subsystem supports two
  active levels, so the elevator transaction must not be reduced to a camera move.
- Player movement calls threshold predicates; falls inspect the departure flag
  to avoid conflicting relocation (`src/player/active_present_falling_and_respawn.c`).
- Inventory flags gate departure; telephone ring sound is suppressed during
  departure (`telephone.c`); audio resets accompany shared departure and arrival.
- Sprite allocation and animation readiness directly gate state transitions.
  Speech resources and the shared interrupt callback are temporarily owned by rides.

## ORIGINAL BUG list

- `SetupElevator`: an out-of-range ordinary departure-level request returns from
  a `char` function without a value. The source preserves that original behavior;
  it is not a defined zero/no-elevator result.

No other `ORIGINAL BUG:` comments occur in these three inspected source files.

## Open questions

- Numeric states are shared with animation tables; complete user-facing enum names
  would require checking descriptor content, not merely renaming the branch numbers.
- `ProcessElevator` assumes a valid nonzero ID; invalid-ID behavior is not specified
  by its caller-facing header and should not be invented as a supported case.
- The global arrival/departure timers encode original concurrency assumptions.
  [INFERENCE] More than two independently active level transitions would need new
  ownership rules, not larger copies of the same global arrays.

## Port notes

- **Platform layer:** `ElevatorBeginRideScene` replaces tiles/palettes and installs
  `ElevatorRideVBlank`; `ElevatorEndRideScene` busy-waits during deceleration.
  [INFERENCE] Replace those waits/register writes with platform services and an
  explicitly ticked scene rather than an interrupt owning global time.
- **Shared-screen coupling:** `ProcessElevatorStep1` reads `gPlayerInventoryOpen` and
  `gPlayer2InventoryOpen`; `ElevatorAdvanceRidersToNextLevel` shows the shared ride only when
  all active players ride. [INFERENCE] Online co-op needs per-player presentation
  and menu rules while attachment/level/bank changes remain shared simulation events.
- **Determinism:** `ProcessElevatorStep1`/`ProcessElevatorStep2` rely on animation
  completion; `ElevatorUpdateRideDialogue` can jump `gFrameCounter`. [INFERENCE]
  Keep logical transition ticks separate from render frames and cosmetic dialogue RNG.
- **Content tables:** `ElevatorSetState` consumes paired animation descriptors;
  `ElevatorPrepareRideDialogue` consumes exchange data. [INFERENCE] Externalize
  those inputs, offsets, backgrounds, and sounds with stable state IDs, without
  encoding the original slot+2 entity relationship as a native content constraint.
- **Camera/visibility:** `ElevatorUpdateEntityVisibility` controls display allocation,
  and `ProcessElevatorStep1` waits for displayed animation completion. [INFERENCE]
  Expand bounds consistently for widescreen or decouple logical progress from
  display allocation so merely seeing farther does not change transition timing.
