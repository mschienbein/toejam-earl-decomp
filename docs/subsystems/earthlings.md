# Earthlings: banks, behavior, targeting, and contact

## Purpose and ownership

Earthlings combine persistent per-level records with disposable entity objects and
a ROM definition table. The table supplies animation states, movement callback,
ability/think callback, touch callback, dimensions, contact/damage values, and random
movement parameters. Ship pieces also occupy this system as `EARTHLING_SHIP_PIECE`
(`EarthlingRetainLevelBank`, `EarthlingAllocateEntities`, `EarthlingDispatchAbilities`).

Source paths describe the repository layout. The subsystem lives in `src/earthlings`;
bare C filenames below abbreviate files in that directory. Player-side contact
abilities remain in the player subsystem, while projectile impacts/remapping belong
to presents. `src/earthlings/mailbox_earthling.c` owns the mailbox predicates.
Canonical names also appear in `config/rev02/names.json`; they are not proof of
historical source spelling.

`include/earthling_types.h` names content kinds without changing their byte storage;
the record and constant identifiers below match the repository headers.

## Files and their roles

- `spawning_and_banks.c`: spawn searches, bank retention, ship-piece injection,
  state installation, entity activation, present-created Earthlings, ability/touch
  dispatch, sleep/wake, damage/removal, and the high-level update pass.
- `bank_expiry.c`: reference-count release and final cleanup of a level bank.
- `terrain_movement.c`: frame-scaled movement callbacks, terrain probes, bounds,
  collision/decoy handling, terrain draw flags, and menu/update blocking.
- `generic_enemy.c`: common target/turn/wander state machine plus type-specific
  attack, delay, and mole handling.
- `terrain_animation.c`: timed movement and `EARTHLING_ICE_CREAM_TRUCK` terrain-dependent
  animation families, plus Cupid's special attack/bounce delegation.
- `animation_behaviours.c`: horizontal wander/turn/reversal and targeted reset cycle.
- `movement_decision.c`: blocked horizontal turn choice and mole decoy collision.
- `targeting.c`: decoy-first/nearest-player acquisition and player-state eligibility.
- `steering.c`: velocity toward a delta and Cupid-style horizontal drift.
- `steering_helpers.c`: vertical-dead-zone steering and a one-shot target sound event.
- `mailbox_earthling.c`: per-level mailbox mode and mail-order predicate.
- `nice_earthling_touch.c`: two-choice UI, mailbox/wizard/wiseman/opera interactions,
  protected friendly states, and cooperative life requests.
- `mole_santa.c`: six-slot mole effects, stolen-present cleanup, Santa logic,
  and Santa's multi-present drop transaction.

## Data structures and RAM

`include/earthlings.h` defines `earthling_state`, an 18-byte record. It stores kind
(`EARTHLING_UNUSED` for an unused slot), primary entity handle, level, state, timer,
X/Y, `saved_x_or_status`, `saved_y`, state delay, secondary handle, and
`health_visibility[2]`. Element zero is health or a phase timer; element one holds
primary/secondary visibility bits (matching `ENTITY_PRIMARY_VIEW`/`ENTITY_SECONDARY_VIEW`).
The array shape and kind-dependent interpretation are intentionally preserved.

`earthling_definition` in the same header is 38 bytes: state animations; update,
ability, and touch pointers; `movement_mode`; dimensions and tile/sprite counts;
target/turn chances and distance; `behavior_flags`; spawn radius; contact ranges;
damage; target/turn/move timer ranges; state delay; and health.
`include/assets_earthlings.h` declares `EarthlingDefinitions` with `EARTHLING_KIND_COUNT`
logical records and `LevelEarthlingChoices[26][20]`. The final record's `unused`
byte overlaps `EarthlingCoordinatePairs`; asset partitions are not logical type bounds.

`include/ram_earthlings.h` owns `gEarthlingBanks[2][20]`, load/reference flags,
level identities, per-level high-bit-first failure/removal masks, update parity,
two six-byte mailbox states, friendly interaction states, and mole theft state.
`gEarthlingBank2Records` and `gEarthlingBank2Users` alias the second bank
(index one), not new banks.

`special_earthling_state` has mode, level, short timer, and two contact bytes.
Its per-player contacts are interpreted by mailbox touch logic. Wizard, wiseman,
and opera-singer state arrays are separate parallel storage, not mailbox fields.

Display objects use `object_state` (`include/entities.h`) and are allocated from
`gPlayerInfo`; they point back to their Earthling record. Other dependencies are
`ram_player.h` for players/terrain, `ram_effects.h` for decoys/projectiles/effects,
`ram_ui.h` for menu/choice gates, `ram_camera.h` for visibility, and `ram_system.h`
for input, direct RNG, and the shared 59-word random cache.

## Bank lifetime and spawning

`SetupInitialEarthlings` marks all forty records unused, retains level one for each
active player, then calls `AssignShipPieceLevels`. `EarthlingRetainLevelBank` returns
for level 26+, increments an existing matching bank's reference count, or chooses
an unused bank for the new level (`spawning_and_banks.c`).

For a new bank, it exchanges in the level seed, checks for an assigned ship piece,
configures mailbox mode, and chooses each record. The mailbox and ship piece are
injected before ordinary `LevelEarthlingChoices` entries. `EARTHLING_SHIP_PIECE` receives
the generated ship-anchor position; highest-level pieces queue a notice.
For ordinary choices the saved RNG state is restored when that path is first
entered, so the whole population pass is not simply one isolated seeded draw stream.

Level one exits its record loop immediately at record zero. Level zero uses fixed
positions from player `GetLevelZeroEarthlingPosition`. Other kinds use
`FindSpawnLocationForEarthling`. That helper chooses kind-specific rectangle sizes
and terrain masks, tries ten random metatiles, then scans the whole map cyclically
from a random origin. It returns zero on success and one on failure.

`CheckEarthlingOverlap` rejects elevator placement and occupied records closer than
the sum of both type radii scaled by four. `EarthlingIsSpecialRecordNearby` checks
the first two records for ship pieces/mail-order mailboxes within a much larger
340×200 exclusion rectangle; large item placement calls this predicate.

Spawn failure sets the record's bit in `gEarthlingSpawnFailureMasks[level]`, and
later bank construction excludes records already masked. The same mask also tracks
removed Earthlings (`EarthlingRemoveFromLevel`), so it is not only a diagnostic.

`EarthlingReleaseLevelBank` decrements every matching loaded bank; only the last
release sets its level -1, marks all records unused, and frees primary/secondary
objects (`bank_expiry.c`). Falls, rides, death, and final-level cleanup bracket
world transitions with these retain/release calls.

## State installation and visibility activation

`SetEarthlingState` updates the persistent state even without an object. With an
object it installs that kind/state's timed animation, restarts duration, preserves
or invalidates the old frame by scanning sequence control bytes, and frees a
secondary object when leaving state two (`spawning_and_banks.c`).

`LoadInEarthlings` handles only one slot parity per pass. It copies displayed
coordinates back to the persistent record, computes visibility relative to both
pending cameras, and sets camera bits. Bounds are X in (-160,480), Y in (-112,336);
the second camera is considered only in split mode. Objects are created if either
bit is set and released otherwise, while the record remains banked.

`EarthlingAllocateEntities` installs the type's movement process, dimensions,
resource counts, level, and record backlink. Definition `behavior_flags` plus cached
RNG can allocate a secondary animation and state two. `EARTHLING_WHIRLWIND` starts
in state three. Mail-order mailboxes use reduced resources and `ENTITY_SKIP_PHYSICS`.
Primary allocation failure requests eviction for a ship piece but relocates other kinds.

`EarthlingRelocateRecord` frees both objects, repeatedly requests a spawn outside
elevators, and changes the persistent position. `EarthlingRestoreSavedPosition`
frees objects and restores `saved_x_or_status`/`saved_y`. These two operations are not
interchangeable when `saved_x_or_status` instead holds terrain or a one-shot latch.

`EarthlingIsUpdateBlocked` permits a record when it is visible in at least one
camera whose player's menu is closed; a null Earthling pointer is unblocked.
Consequently menu pause is visibility-dependent, not an unconditional global freeze
(`terrain_movement.c`).

## Per-frame/phase flow

The main loop calls `ProcessEarthlings` on its Earthling/elevator/item quarter-phase
(`FurtherInitialisation`, `src/core/startup_and_main_loop.c`). The pass returns only
when both players' menu-open bytes are set, then performs:

1. `LoadInEarthlings` on each loaded bank for the selected parity.
2. `EarthlingDispatchAbilities` on occupied, allocated, awake, non-mail-order records,
   calling each type's ability pointer if its update gate permits.
3. `EarthlingDispatchPlayerContacts`, which selects a cached-random first player,
   then tests both players against type contact ranges, menu/elevator flags, and
   temporary player immunity. A contact can wake state two before the touch call.
4. `EarthlingApplyNearbySleep` for active boom boxes; the pending ship notice and
   finished mole/stolen-present effects are then processed.
5. Toggle parity so the other ten slots per bank are serviced next time.

Animation movement callbacks are a separate route: the entity's `process` pointer
comes from the type definition. `EarthlingMoveOnAllowedTerrain` and
`EarthlingMoveWithinMapBounds` multiply velocity by frame `motion_x/motion_y`,
then commit integer/fractional coordinates after collision/terrain checks.

The first callback restricts movement to kind-allowed terrain and sets the object's
sprite state zero on rejection; it uses narrow or menu-aware volume collision
according to kind and records decoy hits. The second enforces world bounds, keeps
terrain draw bits synchronized, and stores sampled terrain for `EARTHLING_ICE_CREAM_TRUCK`.
`EarthlingProbeDirectionalTerrain` offsets a probe in movement states 4–7.

## Think families and targeting

`AI_GenericEnemyThink` handles blocked-motion reset/delay, then on timer expiry
tries target acquisition, a type-defined turn, or random cardinal travel. Timer
ranges and probabilities come from the definition. States 4/5/6/7 establish
cardinal velocities when steering has not supplied one; timers are forced positive.
`EarthlingProcessSpecialThinkStates` intercepts mailbox, wizard delay, mole,
dentist-laugh, aimed-projectile, and other type-specific animation paths.

`EarthlingThinkTimedMovement` follows the same parameterized target/turn/travel
pattern but delegates Cupid bounce/attack and ice-cream-truck animation transitions.
`EarthlingSelectTerrainAnimationState`/`EarthlingUpdateTerrainAnimationState` change
`EARTHLING_ICE_CREAM_TRUCK`'s group from stored terrain, animation completion,
and player proximity; entering/leaving groups changes `ENTITY_COLLIDABLE`
(`terrain_animation.c`).

`EarthlingThinkHorizontalWander` reverses blocked horizontal movement, optionally
tracks a target, then chooses timed vertical drift. `EarthlingAdvanceHorizontalTurnAnimation`
steps/reverses facing states on completion; `EarthlingThinkHorizontalReversal`
adds menu-state checks; `EarthlingThinkTargetedResetCycle` uses target mode
`EARTHLING_WHIRLWIND`, phase countdown, and saved-position restoration (`animation_behaviours.c`).
`EarthlingSelectTurnState` returns a facing/turn choice, with an early decoy collision
path for moles (`movement_decision.c`).

`EarthlingSteerTowardNearestTarget` checks same-level decoys first; players are
considered only if neither decoy candidate survives. It compares squared distances
in quarter-world coordinates, applies a horizontal separation bias, excludes elevator
riders, and adds remap/mole-specific eligibility. Equal distances choose the second
candidate because selection uses a strict less-than comparison (`targeting.c`).

Modes dispatch to ordinary delta steering, vertical-dead-zone steering, or horizontal
drift; mode `EARTHLING_SHARK` may emit a one-shot status sound. Ordinary steering chooses
±256 velocities and suppresses an axis in narrow alignment regions. Horizontal
drift can retain prior X velocity or draw a direct random sign; the dead-zone helper
always moves horizontally and ignores vertical error within two quarter-units
(`steering.c`, `steering_helpers.c`).

## Friendly interactions, mailbox modes, and life requests

`EarthlingConfigureMailboxLevelBank` enables a mailbox bank on level two and above.
Levels four and above except twelve have a 50%-threshold chance of mode one;
other eligible levels reset mode/contact bytes. `EarthlingIsMailOrderMailbox`
identifies `EARTHLING_MAILBOX` with a matching bank whose mode is not one (`mailbox_earthling.c`).

`AI_MailboxTouch` offers mail order through a per-player choice/result lifecycle
in the non-one mode; mode one uses its approach, roar/bite, cooldown, and idle
animations with the shared hurt handler (`nice_earthling_touch.c`).

`PlayerCreateInteractionChoiceDisplay` attaches a small choice object to the player.
`CheckAButton` resolves action-edge acceptance/cancel, uses up/down to change choice,
consumes processed input while active, and latches action release afterward.
A positive service response is value two; it is not the same as the display's
zero/one selection index.

`AI_WizardTouch` charges one buck, animates healing one health unit at a time, and
restores the player's pose. `AI_WisemanTouch` opens the identification inventory
after agreement; the actual purchase is delegated to UI. `AI_OperaSingerTouch`
charges three bucks, can pose both shared-screen players, and repeatedly calls
`DoVikingAbility` against objects matching the customer's viewport draw mask.
These handlers serialize same-level partner interactions using per-player state.

`NiceEarthlingCheck` preserves a last hit point for specific singer, wizard, and
wiseman states. `DamageEarthling` otherwise decrements health, excludes special
immune kinds, removes dead records, and requests a timed effect. `DoVikingAbility`
also checks effect-slot capacity before a last hit; `EarthlingDamageOnHazardTerrain`
uses newly changed terrain while excluding kinds allowed to survive it.

`PlayerProcessLifeRequest` lets an absent player ask an eligible partner for a life
through a camera-relative display and choice. Agreement can decrement the donor's
lives, copies map state, restores the joining player's resources, forces split
mode, and delegates relocation. `PlayerCreateLifeRequestDisplay` verifies primary
viewport allocation before allowing that interaction (`nice_earthling_touch.c`).

## Sleep, Santa, and present-created Earthlings

`EarthlingApplyNearbySleep` places eligible nearby allocated kinds in state 99,
installs their sleep descriptor, stops horizontal velocity, and removes secondary
objects. It excludes ship pieces, mail-order mailboxes, several special kinds, and
some interaction states. `EarthlingWakeLevel` restores state zero for sleeping
records in that level; `EarthlingResetStateTwoByDrawMask` separately resets eligible
state-two animations (`spawning_and_banks.c`).

`SantaLogic` examines both players' ranges, sprites, burps, and animation frames to
choose alert/flight states. Flight uses acceleration and mixed cached/direct RNG;
once offscreen it relocates until neither viewport reports visibility. `DropPresentsAbility`
temporarily moves the player to Santa, saves/refreshes/restores the terrain cache,
and drops two or three selected presents before restoring coordinates (`mole_santa.c`).

`EarthlingSpawnMoleEffect` uses a six-handle effect pool;
`ReleaseFinishedMoleEffectsAndStolenPresents` frees finished effects and stolen
present displays that reach ground. Actual inventory theft is player-side.

`OpenEarthlingPresent` tries nearby offsets with compatible water classification,
looks for an unused or unallocated bank record, selects a terrain-appropriate kind
with special uniqueness restrictions, initializes `health_visibility` and saved/status words, creates a
transient reveal effect, and ensures entity capacity through allocation/eviction.
Its exhausted-slot paths contain the explicit bugs below.

## ORIGINAL BUG list

- `OpenEarthlingPresent`: after the first full-bank scan, the code reads one record
  past the bank before trying an inactive-record scan; exhaustion of that second
  scan is not checked before record writes either.
- `OpenEarthlingPresent` and `EarthlingDamageOnHazardTerrain`: their translation
  unit retains an `int` result view of `GetMapTerrainAtWorldPosition`, whose shared
  declaration returns unsigned char (`spawning_and_banks.c`).
- `EarthlingRetainLevelBank` and `OpenEarthlingPresent`: the same unit retains an
  `int` result view of the shared char-returning `EarthlingConfigureMailboxLevelBank`.
- `FindSpawnLocationForEarthling` and `OpenEarthlingPresent`: the same unit retains
  an `int` result view of the shared unsigned-short-returning `GetRandomNumber`.
- `EarthlingProbeDirectionalTerrain`, `EarthlingMoveOnAllowedTerrain`,
  `EntityUpdateTerrainDrawFlag`, and `EarthlingMoveWithinMapBounds`: their unit
  retains the `int` terrain-query result view (`terrain_movement.c`).
- `SantaLogic`: its unit retains an `int` result view of the shared short-returning
  `IsEntityVisibleInViewport` (`mole_santa.c`).

For these return-view comments, the sources report that ordinary shared declarations
and integral promotion do not match the original. Historical missing declarations
are explicitly [INFERENCE], not proven history or a separately observed gameplay
symptom. Preserve that qualification when deciding native bug compatibility.

## Open questions

- `include/earthling_types.h` now names all content kinds; this account does not
  enumerate every `EarthlingDefinitions` row or invent additional per-kind state enums.
- Some record words alternate between coordinates, terrain, and one-shot state;
  a native tagged behavior state should be designed from all consumers, not one AI.
- Spawn-bank selection assumes the two-bank occupancy protocol; unsupported extra
  levels or exhausted records are not defined expansion behavior.
- Friendly behavior, animation completion, type callback dispatch, and contact are
  distinct passes; changing their ordering can change which player interacts first.

## Port notes

- **Platform layer:** `PlayerProcessLifeRequest` manipulates VDP/blanking state;
  `PlayerCreateInteractionChoiceDisplay` allocates presentation entities. [INFERENCE]
  Expose those presentation effects as events and keep display-capacity failure
  separate from authoritative life/bank decisions.
- **Shared-screen coupling:** `EarthlingIsUpdateBlocked` combines visibility with
  `gPlayerInventoryOpen`/`gPlayer2InventoryOpen`; `ProcessEarthlings` returns when both menus are open.
  `AI_OperaSingerTouch` uses viewport masks for damage. [INFERENCE] Online menus need
  explicit interaction protection rather than view-owned enemy suspension.
- **Determinism:** `ProcessEarthlings` toggles `gEarthlingUpdateParity`, and
  `EarthlingDispatchPlayerContacts` randomizes the first player tested.
  `EarthlingRetainLevelBank` restores its seed before ordinary choices. [INFERENCE]
  Preserve those orders and timer widths; lockstep needs simulation RNG and
  activation regions independent of each client's render visibility.
- **Content tables:** `EarthlingAllocateEntities` consumes `EarthlingDefinitions`;
  `AI_GenericEnemyThink` consumes its probabilities/ranges, and `AI_WizardTouch`
  contains a service cost. [INFERENCE] Externalize these inputs with stable
  behavior IDs and per-kind tagged state rather than raw RAM aliases.
- **Camera/visibility:** `LoadInEarthlings` uses fixed activation bounds and
  `gSplitScreenEnabled`, while `SantaLogic` retries relocation until neither view sees it.
  [INFERENCE] Define activation independently of display width, or deliberately
  version the changed behavior so widescreen does not silently alter co-op simulation.
