# Players, movement, falls, and abilities

## Purpose and source boundary

Player behavior is distributed across a shared entity record, named sprite states,
an animation-family mask derived by `SetSprite`, and parallel per-player RAM arrays.
`ProcessPlayers` does not switch on one master enum: it runs an ordered chain of
handlers, each checking its own state/group conditions. Movement integration and animation callbacks
are separate passes (`ProcessPlayers`, `ProcessMovement`, `SetupPlayer`).

Source paths describe the repository layout. Player C files live in `src/player`;
bare C filenames below abbreviate that directory. `src/player/edge_hop_and_clap.c`
owns edge hops and health sharing. Targeting/steering live in `src/earthlings`,
and camera positioning/split-view animation live in `src/camera`; those are not
counted as player implementation here. Identifiers match the repository headers.

Several important interfaces have declarations/callers but no canonical C body in
the inspected `src`: `SpriteChecks`, `PlayerProcessTerrainMotion`,
`CheckPlayerFalling`, `Player_Youch`, and `MapClearTilePriorityNearPlayer`.
Their exact missing internals remain open rather than being reconstructed from names.

## Files and roles

- `setup_player.c`: player objects, hardware sprite links, inventory-object chains.
- `sprite_movement.c`: directional speed/animation selection and phase preservation.
- `entity_motion_terrain.c`: proposed-position terrain resolution, `StopPlayer`,
  and the two installed player terrain callbacks.
- `process_movement.c`: fixed-point integration of all linked active entities,
  with additional player bounds and collision handling.
- `level_fall_sinking_and_respawn.c`: ordered player updates, sinking, jump/landing,
  spring shoes, level falls, relocation, death, and last-life removal.
- `start_falling.c`: falling initiation, VBlank terrain priority handoff, teetering.
- `jump_motion.c`, `start_motion_mode.c`: phased jump input/state and initialization.
- `rocket_motion.c`, `open_rocket_skates.c`: skates input, acceleration, turns,
  terrain-dependent sprites, and opening setup.
- `active_present_sprite.c`: the active powerup/terrain-dependent idle selection.
- `projectiles.c`: player firing poses and launch into the shared projectile pool.
- `sleep.c`: schoolbook, idle sleep, wake effects, and wake-input checks.
- `edge_hop_and_clap.c`: automatic terrain-edge hop and two-player health sharing.
- `entity_motion_proximity.c`: same-level proximity/range predicates.
- `earthling_player_eligibility.c`: common allowed-player-state predicate.
- `contact_cupid_and_touch.c`: squish, Hula, Cupid, chicken/boogie, whirlwind,
  bouncing damage, and decoy attack-range behavior.
- `mole.c`: victim capture, inventory theft, and stolen-present animation setup.
- `hottub_ability.c`: entering, bathing/healing, and leaving the hot tub.
- `ability_helpers.c`: level-zero fixed items/positions and lemonade reward flow.
- `effect_slots.c`: allocation/retirement of the two generic timed effects.

## Data structures and RAM

`include/entities.h` defines the 128-byte `object_state` used by players and other
entities. It contains world X/Y/height, signed-short velocities/accelerations,
byte fractions, `animation_motion_axes`, process callback, animation sequence/frame
pointers, frame counters, sprite resources, level, `draw_flags`, `behavior_flags`,
Earthling pointer, `owner`/attachment fields, and linked-list membership. Height is
world elevation; `height_pixels` is the sprite extent, not a duplicate coordinate.

`include/ram_player.h` declares `gPlayerInfo[40]`; `PLAYER_TOEJAM`/`PLAYER_EARL`
in `include/entities.h` identify the two players. `gEarlPositionX` aliases Earl's record,
and `gToejamLevel`/`gEarlLevel` name existing
level bytes. Records 26–39 also host inventory displays: two selector-plus-six-item
chains initialized in `SetupPlayer`, not fourteen extra player actors.

The same header owns lives, bucks, points, rank, health, health restoration, and HUD
shadow values. Player-two scalar names alias element one of the corresponding
arrays. Incomplete array bounds in the header are deliberately not promoted here
to guessed independent allocations.

`LAND_NEAR_TOEJAM` and `LAND_NEAR_EARL` are five-by-five tile-property neighborhoods;
the `LAND_UNDER_TOEJAM`/`LAND_UNDER_EARL` names alias center [2][2]. Player handlers
commonly mask that property with `TERRAIN_CLASS_MASK` (`include/terrain.h`), while
destination/edge logic can inspect full property bytes (`PlayerApplyTerrainDestination`).

Parallel arrays in `ram_player.h` track fall states, jump counters, sinking,
landing-counter seeds, pending hurt damage, pre-death sprite state, phased-jump
phase/ticks/arming/speech, rocket turn delay, book delay, wake effects, idle counters,
and Hula timing. These are not a recovered monolithic historical player struct.

`include/ram_effects.h` owns powerup IDs/timers, shared projectile handles/owners,
whirlwind timers, remap effects, and generic effect slots. `ram_elevator.h` owns
attachment IDs and approach gates; `ram_ui.h` owns menu/choice state;
`ram_camera.h` owns split/view coordinates; `ram_system.h` owns input and time/RNG.

`include/sprites.h` supplies frame/animation layouts and `include/assets_sprites.h`
player animation tables. `SetSprite` derives `animation_group_mask` as
`1 << (sprite_state >> 2)`: these are four-state-family membership bits, not free
ability flags. `include/player_animation.h` and `include/player_animation_groups.h`
name the states and masks; `include/present_types.h` names active power identities.
`entity_motion_terrain.c` additionally defines a local 12-byte `movement_destination`
with position, tile indices, and fractional bytes; it is not a RAM overlay type.

## Initialization and update schedule

`SetupPlayer` installs `Player1ProcessTerrainMotion`/`Player2ProcessTerrainMotion`,
player-specific initial animation descriptors, and different sprite extents/counts.
Both callbacks forward to `PlayerProcessTerrainMotion` with an explicit player index.
It also initializes hardware sprite links and the inventory draw-list head.

The main loop alternates view/land/sprite work with an entity-processing phase.
Within the latter, player terrain/input/state work and Earthling/elevator/item work
alternate, while `ProcessMovement`, viewport updates, animation motion, and speech
run on both entity phases (`FurtherInitialisation`, `src/core/startup_and_main_loop.c`).
Thus a handler call, a simulation step, and one VBlank are not interchangeable.

`ProcessInput` obtains pad input, resolves two-choice interactions through
`CheckAButton`, handles pause/map selection, then calls `SpriteChecks` for active
players whose menus are closed. Its map entry is a blocking call (`startup_and_main_loop.c`).

For each active, menu-closed player, `ProcessPlayers` runs this exact ordered chain:
teetering; hurt/idle; spring shoes; descending terrain; phased jump; skates;
jump/landing animation; present completion/terrain pose; sinking; death; shooting;
sleep; whirlwind/recovery; health/lives checks. It then maintains an interaction/
elevator collision flag. Promotion, control-remap, and terrain-effect updates follow
for non-skipped records; absent players instead enter the life-request path.

After both players, `ProcessPlayers` calls `CheckPlayerFalling` and, when
`PLAYER_INDICATOR == 2`, health-sharing clap processing. A menu-open active player
hits `continue`, skipping even the common tail for that player.

## Ordinary movement and terrain response

`PlayerChooseDirectionalMovement` starts with fixed-point speeds 320/240, raises
them on `TERRAIN_FAST_GROUND`, reduces them with negative sinking height, honors
elevator approach input gates, and chooses horizontal/vertical/diagonal velocities.
`PLAYER_GROUP_ICARUS_FLY` selects acceleration, while `PLAYER_GROUP_HITOPS` triples speed.

`PlayerApplyDirectionalMovementAnimation` applies those values, chooses the
appropriate directional family for active abilities/terrain/action input, and
preserves animation frame/duration when changing within the same family. Sequence
control bytes can invalidate a preserved frame. Directional animation and movement
mode therefore jointly control motion (`sprite_movement.c`).

`PlayerApplyTerrainDestination` reads the proposed tile in the local cache. Ground
properties through `TERRAIN_WALKABLE_MAX` or positive elevation allow the proposed
position; high properties adjust outer cache-edge positions and choose a collision pose.
Certain movement families are treated as water/air motion and restore another pose
when they leave `TERRAIN_WATER`. On final level 26, a proposed move that would require
separate player views is rejected, directly coupling movement to camera policy.

`StopPlayer` clears all three velocity and acceleration components without changing
position. It does not clear fractional position bytes (`entity_motion_terrain.c`).

`ProcessMovement` traverses the active entity chain, checks draw/update gates, adds
acceleration, then integrates position as integer world coordinate plus an 8-bit
fraction and signed velocity. `ANIMATION_MOTION_X`/`ANIMATION_MOTION_Y` delegate
selected axes to frame callbacks instead of this integrator. Player records get
fixed bounds, X/Y/descending collisions (`ENTITY_BLOCKED_X`,
`ENTITY_BLOCKED_Y_OR_HEIGHT`), and elevator crossing. Earthling blocking also participates.

`PlayerCheckTerrainEdgeHop` samples the proposed point in the local terrain cache.
High-bit terrain launches the edge-hop sprite/vertical motion; otherwise it may
play the alternating randomized meep sound (`edge_hop_and_clap.c`).

## Jump, powerup motion, and shooting

`GetActivePresentSprite` selects a resting pose from powerup ID, terrain, height,
and elevator state. Rocket skates delegate to `OpenRocketSkates` and return -1;
other branches may also return -1 to leave an ongoing airborne pose unchanged.
The helper can reset horizontal velocity and animation frame, so it is not pure.

`PlayerInitializePhasedJumpMotion` initializes phase/ticks/arming/speech bytes.
`PlayerUpdatePhasedJumpMotion` runs only for `PRESENT_ICARUS_WINGS`, suitable states,
no fall/elevator, and limited sinking. It alternates phases from action press/release,
handles landing/water/void, and delegates to `PlayerApplyPhasedJumpInput`.
The latter boosts or cancels horizontal velocity at phase changes, decelerates
released directions, clamps axes, applies remapped controls, and speaks once after
rising. The power and animation names come from the integration headers, not new enum storage.

`PlayerUpdateSpringShoeJump` runs for `PRESENT_SPRING_SHOES` outside falls/elevators. Action
input starts a short preparation, then launch animation completion doubles horizontal
velocity and adds vertical motion. `PlayerResolveDescendingTerrain` limits ascent
when action is released/height exceeds a threshold, then chooses ground landing,
water splash, void fall, or running-powerup recovery.

`OpenRocketSkates` stops motion, raises submerged players when necessary, chooses a
terrain/height-dependent direction family, and clears previous directional input.
`PlayerUpdateRocketSkatesMotion` applies input-change/turn-delay gates and cached
random alternatives after collisions. `PlayerApplyRocketSkatesInput` converts
reversals into turns, accelerates on one axis, and clamps speed;
`PlayerUpdateRocketSkatesSprite` handles charged jump/release, water, and landing.

`PlayerUpdatePresentShooting` launches on specific animation frames and then restores
the active pose. Held direction+action outside elevators selects firing sprites for
`PRESENT_TOMATOES`/`PRESENT_SLING_SHOT`. `PlayerLaunchPresentProjectile` uses one of the first four
shared slots, player/facing-dependent placement triples, two launch-speed/gravity
profiles, and a three-quarter diagonal adjustment. Impact/update ownership belongs
to `src/presents/projectile_effects.c`.

## Falls, sinking, relocation, and death

`PlayerStartFalling` sets fall state one, selects the facing fall pose, nudges away
from a supported neighbor, sets downward acceleration, clears tile priority, and
waits for a VBlank terrain-priority operation before speech/sound. Teetering counts
past thirty updates, but departure-in-progress delays its short launch
(`PlayerUpdateTeetering`, `PlayerStartTeeteringFall`).

`MakePlayerFall` waits until the relevant falling players are ready. It releases
old Earthling/item banks, decrements level (wrapping below zero to the highest
level), assigns height 600 and downward motion, loads/repopulates destination banks,
and positions falling players together or beside a supported partner. It avoids
using an unsupported/dead partner and refreshes split mode, level HUD, and video
state. `CheckPlayerFalling` decides when to call this flow but its body is absent.

`PlayerUpdateJumpLandingAnimation` advances preparation/launch/landing flags and
chooses splash or the fall-state landing path. `PlayerUpdatePresentCompletionAndTerrainPose`
returns finished present poses to the active state and searches nearby support when
leaving the resting water pose (`level_fall_sinking_and_respawn.c`).

`PlayerUpdateTerrainSinking` lowers eligible grounded players on `TERRAIN_SINKING`,
using time-counter masks, per-character depth thresholds, cached speech chance,
and partner overlap to reset shallow depth. Leaving sinking restores height/flags.
The source's byte store of literal 256 is a matching qualification explicitly
not asserted to be an original gameplay bug.

`PlayerUpdateHurtRecoveryAndIdle` applies pending damage after the hurt animation,
restores skates/air/terrain motion, nudges toward support, and can start a void fall.
Its Earl-only idle variation consumes direct RNG (`level_fall_sinking_and_respawn.c`).

`PlayerRelocateToPartnerOrNextLevel` returns to the live partner, reloads banks if
levels differ in split mode, or advances toward the highest level when alone.
During an elevator departure it instead copies the partner's attachment and hides
the returning player; otherwise it starts a descending reappearance effect.
`PlayerAdvanceToNextLevel` brackets its own level increment with effect waits,
bank release/retain, map/item loading, and arrival-anchor positioning.

`KillThePlayer` refuses menu-open players, stops motion, cancels skates if active,
saves the prior sprite, and starts `PLAYER_ANIM_GHOST`. `PlayerUpdateDeathAnimation`
uses camera-relative exit position and `ENTITY_LAST_LIFE_EXIT` to select return or final
removal. `PlayerFinalizeLastLifeExit` can invoke game over, otherwise releases banks,
marks the player absent on level 99, clears split mode, and restores the survivor's
map/HUD. It waits for the partner's menu to close before final removal.

## Sleep, contacts, and cooperative actions

`OpenSchoolBook` enters `PLAYER_ANIM_READ_BOOK` and a sixty-update reading delay.
`ProcessSleep` enters `PLAYER_ANIM_SLEEP` after that delay or over 300 eligible idle updates, chooses 4–10 wake
effects, and probabilistically responds to changed raw pad input with whisper/shout
objects. Final effect completion restores the active pose. REV 02 excludes active
two-choice interactions from idle sleep; REV 00 does not (`sleep.c`).

`PlayerCanReceiveEarthlingEffect` combines the per-player interaction exclusion byte
with permitted sprite/flag masks. `Player_Squish` applies direct type-defined damage;
Hula induces nearby dance; Cupid chooses horizontal/falling attacks; chicken and
boogie helpers use the shared hurt interface and state selection (`contact_cupid_and_touch.c`).
`EarthlingStartHorizontalAttack` and `EarthlingCheckDecoyAttackRange` also target decoys.

`WhirlWindAbility` consumes the source Earthling and captures the player for 225
updates in `PLAYER_ANIM_WHIRLWIND`; `PlayerUpdateWhirlwindAndRecovery` performs
cached-random wandering, then 48 updates in `PLAYER_ANIM_WHIRLWIND_RECOVERY` and
terrain/powerup restoration. The bouncing damage helper is not ordinary player physics.

`MoleAbility` aligns mole/victim, captures `PLAYER_ANIM_ITEM_REACTION`, and steals up to three presents
under its state/timer/chance rules. `DoMoleAbility` removes an occupied inventory
entry by replacing it with the last entry, then optionally creates a thrown display
object; inventory loss precedes allocation, so allocation failure does not undo it.

`HottubAbility` advances entry jumps, bathing/healing, randomized chat, directional
exit, and landing. `LemonadeAbility` completes drinking/burp animations and opens
`PRESENT_EXTRA_LIFE` after audio cooldown. Level-zero item/Earthling coordinates are fixed
helpers in `ability_helpers.c`, not generated map content.

`PlayerUpdateHealthSharingClap` requires the same level, aligns complementary
facing players into `PLAYER_ANIM_HIGH_FIVE_LEFT`/`PLAYER_ANIM_HIGH_FIVE_RIGHT`, plays the clap, and redistributes health
when Earl's animation completes. `EntityIsNearPosition`/`EntityIsWithinRange` require
matching levels and exclusive axis limits; timed-effect helpers manage two slots
and free their objects after animation completion.

## ORIGINAL BUG list

- `PlayerApplyRocketSkatesInput`: an excessive X speed takes the first clamp branch
  and skips the Y clamp because it is `else if`, not a second independent test.
- `PlayerUpdateHealthSharingClap`: when the averaged health exceeds a maximum-health
  helper result, the compared player's health is assigned the average rather than
  capped to that result; the other assignment uses the intended limit in subtraction.
- `EarthlingUpdateBouncingDamageState`: the map index is uninitialized if neither
  active map level matches the object's level before the terrain query.

These are the `ORIGINAL BUG:` comments in the inspected player source files.
Matching qualifications and uncertain historical spellings are not extra bug claims.

## Open questions

- Missing `SpriteChecks` and `PlayerProcessTerrainMotion` bodies leave the complete
  input-to-destination dispatch path open; the shown directional and terrain helpers
  establish important parts, not the entire missing controller.
- Missing `CheckPlayerFalling` and `Player_Youch` bodies leave exact fall readiness
  and common hurt eligibility/damage initiation outside this source-only account.
- `animation_group_mask` is derived from `sprite_state`; named group constants
  describe individual families, but composite eligibility masks still require all consumers.
- `PlayerAdvanceToNextLevel` and relocation use some explicit map-zero queries even
  with a player argument. The visible call is reported, not silently corrected.

## Port notes

- **Platform layer:** `PlayerStartFalling` waits on `WAIT_FOR_VBLANK_FLAG` after its
  terrain-priority request; `PlayerRelocateToPartnerOrNextLevel` changes VDP/blanking
  state. [INFERENCE] Split logical state changes from renderer/audio notifications
  and eliminate these blocking waits inside native simulation.
- **Shared-screen coupling:** `ProcessPlayers` skips the owner of `P1_MENU_OPEN`;
  `PlayerApplyTerrainDestination` rejects final-level movement that requires separate
  views. Death/fall relocation also coordinates the partner. [INFERENCE] Online
  co-op needs explicit per-player suspension and shared transition policies rather
  than freezing the partner whenever one inventory is displayed.
- **Determinism:** `ProcessMovement` integrates fixed-point shorts/fractions;
  `ProcessPlayers` fixes handler order; `ProcessSleep` consumes direct/cached RNG.
  [INFERENCE] Use fixed simulation ticks, stable input histories, explicit-width
  arithmetic, and replicated state rather than native render time.
- **Content tables:** `PlayerChooseDirectionalMovement` embeds movement parameters;
  `PlayerLaunchPresentProjectile` reads placement triples; `InitializeLevelZeroFixedItems`
  embeds placements. [INFERENCE] Externalize these inputs and sprite-family bindings;
  data-driven presents need behavior IDs connected to the player handlers.
- **Camera/visibility:** `PlayerUpdateDeathAnimation` uses camera-relative exit
  position, and `PlayerApplyTerrainDestination` asks whether views would separate.
  [INFERENCE] Widescreen must define logical death/interaction timing independently
  of a larger viewport while preserving world collision dimensions.
