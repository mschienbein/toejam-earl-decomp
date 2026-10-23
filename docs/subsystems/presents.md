# Presents, world items, telephones, and projectile effects

## Purpose and boundaries

This subsystem assigns present appearances/identification, populates item banks,
collects ground items, opens effects, maintains timed powers/world effects, runs
telephone reveals, and processes shared tomatoes and control-remap projectiles.
Movement abilities activated by presents live in player; inventory selection and
consumption live in UI (`SetupPresentIdentities`, `MapUpdateItemsAndPresents`,
`OpenSelectedPresent`, `ProcessPresents`, `ProcessTomatoRain`).

Source paths describe the repository layout. Present C files live in `src/presents`;
bare C filenames below abbreviate that directory. Telephone helpers are
`src/presents/telephone_clear.c`/`src/presents/telephone_open.c`, and the empty
callback is in `src/presents/noop.c`. Canonical identities are in `config/rev02/names.json`.
`DropPresent` has a matched body in `src/presents/level_item_setup.c`; it searches
nearby terrain and inserts into the circular dropped-present bank.

Content IDs come from `include/present_types.h`, terrain from `include/terrain.h`,
and entity policy/state names from `include/entities.h` and the player-animation headers.

## Files and roles

- `src/presents/level_item_setup.c`: identity shuffle, initial inventories, item-bank
  reference lifetime, placement constraints/search, weights, level population, and
  dropped-present insertion.
- `item_updates.c`: item display entities/visibility, food/pickup/hazard effects,
  powerup expiration/HUD blink, and the item/present update entry point.
- `present_effects.c`: transient reveal effects, decoys, doorways, promotions,
  burps, rain clouds and boom boxes.
- `present_dispatch.c`: the per-frame effect dispatch (`ProcessPresents`), opening the
  selected present (`OpenSelectedPresent`) and active-power setup (`SetupSelectedPresent`);
  split from `present_effects.c` so that each unit stays inside MCC68K's DOS heap.
- `projectile_effects.c`: Earthling aiming, twelve shared tomato slots, tomato rain,
  four control-remap projectile slots, remapped input, terrain/rosebush effects,
  and animation-frame-triggered Earthling firing.
- `telephone.c`: display allocation, ringing, visibility, answering, and map reveal.
- `telephone_open.c`: ambient distant placement and present-created phone placement.
- `telephone_clear.c`: removal/reset of both telephone records.
- `noop.c`: the explicitly empty `NoOp` callback; not an unfinished implementation.
- UI boundary: `src/ui/inventory.c` owns `OpenPresent`, `RandomizePresents`, shared
  opening, identification, purchase, and drop selection; these are cited where
  necessary to explain end-to-end present behavior, not reassigned to this folder.

## Data structures and RAM

`include/presents.h` defines two-byte `present_info` (`appearance` wrapping index
plus `identified` byte), eight-byte `level_item` (`kind`, `level`, `object`, `unused`,
X/Y), four-byte `item_position`, and eight-byte `telephone_state` (including its
unread `unused_kind`/`unused` bytes). `object` is a `gPlayerInfo` handle, not a pointer.

`include/ram_presents.h` owns `gPresentInfo[28]`, `gPlayerInventory[2][16]`, two
32-record `gObjectTable` banks, their player-reference counts and loaded-level
identities, `gDroppedPresentTable[32]`, its index, and per-level collected-item masks.
The `gItemBank2Records`, `gItemBank2Users`, and `gItemBank2Level` names alias the
second bank (index one).
`ITEM_UNUSED`/`PRESENT_NONE` mark empty item/inventory entries. `ITEM_GROUND_BASE`
separates presents from ground items; `ITEM_BUCK` is money, `ITEM_LARGE_BASE`
starts large objects, and `ITEM_HURT_HAZARD` selects the large hurt source (`item_updates.c`).

The same header declares two telephone records and separate activated/rings-left/
ring-delay arrays. Telephone record index is not necessarily the player who created
or answers it (`OpenTelephone`, `CheckForTelephone`).

`include/ram_effects.h` supplies the independent parallel arrays for active powerup
IDs/timers, transient effects, decoys/hit counts, doorways/contact flags, promotions,
burps, clouds, boom boxes, projectiles/owners, control-remap effects, and terrain
effects. Several scalar names alias player-two elements.
`gPlayerRainCloudStates[player]` holds a cloud entity handle in these consumers.

The shared tomato pool has twelve handles/owners: player launches search slots 0–3;
Earthling aiming and tomato rain search 4–11. Remap projectiles use a separate four
handles (`src/player/projectiles.c`, `src/presents/projectile_effects.c`).

`include/assets_presents.h` declares 28 appearance frames, 27 selection weights,
12 fixed level-one positions, signed food-health lookup values, projectile placement
triples, ground-item frames, and a shared one-frame animation program.
`include/assets_map.h` supplies terrain exclusions and control-remap direction rows;
`assets_sprites.h` supplies effect animations. Structures for frame/animation data
come from `include/sprites.h`; all displayed effects reuse `object_state`.

## Identities and initial inventories

`SetupPresentIdentities` clears `appearance`/`identified` through `PRESENT_MYSTERY`,
fixes the mystery wrapping, fixes/identifies `PRESENT_BONUS_HITOPS`, then assigns
the other wrapping appearances to random unused identities by cyclic collision
resolution. Inventory bytes remain effect IDs; `gPresentInfo[present_number].appearance`
supplies wrapping. Identification is global per ID, not private per inventory.

`SetupInitialInventory` empties both inventories and all three world-item banks,
invalidates loaded-level views, shuffles identities, and retains/populates level one
for each active player. Each gets two `PRESENT_BONUS_HITOPS`, or four when alone.
`OpenSelectedPresent` maps that bonus identity to `PRESENT_SUPER_HITOPS` rather than
treating it as an independent timed ability (`level_item_setup.c`, `present_dispatch.c`).

`SelectRandomPresent` sums all 27 byte weights, draws modulo total+1 until nonzero,
then chooses the first cumulative bucket containing the draw. It does not draw
uniformly from the IDs; mystery resolution and world population reuse this selector.

## Level item population and placement

`LoadLevelItemBank` returns for level 26+, increments references for an existing
bank, or chooses an unused bank and exchanges in the level seed. It creates 12
ordinary slots when either player is absent, otherwise 16, adds level-2 above
level two, and caps the result at 28. It restores the previous RNG state at exit.

Level one uses twelve fixed coordinates and present-only choices; its first four
slots reject selected risky IDs. Level zero skips ordinary slots and initializes
three fixed objects at slots 28–30 through `InitializeLevelZeroFixedItems`.
Ordinary higher levels choose roughly half presents, otherwise weighted food/money,
and omit slots whose collected bit is already set (`LoadLevelItemBank`).

`MapFindItemPlacementPosition` chooses the active map matching the level, tests
small or large rectangle footprints, rejects marked/excluded metatiles, calls
`TerrainValidation`, and avoids elevator regions. It tries ten eligible random
candidates before a cyclic full-map scan. `MapIsLargePlacementTerrainExcluded`
uses the low seven metatile bits and a 23-entry lookup.

`MapPlaceLargeLevelItems` attempts four slots 28–31, spacing them away from each
other, ordinary items, elevators, and ship/mail-order special records. After ten
failed attempts it leaves a slot empty; otherwise `TERRAIN_SINKING` chooses between
unnamed kind 82 and `ITEM_HURT_HAZARD`; other terrain chooses `ITEM_LARGE_BASE`.
`ElevatorAllowsWorldPlacement` always allows level one and rejects other elevator neighborhoods.

`MapReleaseLevelItemBank` only frees/invalidates records when the last player's
reference leaves a loaded level. Collected masks persist separately so deterministic
regeneration need not resurrect collected ordinary items (`level_item_setup.c`).

`DropPresent` rejects swimming poses, selects the current circular-bank slot, and
frees its existing display object before searching. One cached random value
changes the orientation of a five-by-five terrain-perimeter scan. Both passes
reject water and void; the first also rejects positions occupied by another
dropped present on the same level, while the second permits that overlap. A
success writes the present, level, and position and advances
`gDroppedPresentTableIdx` modulo 32. If no position qualifies, the function returns
without advancing the index; the earlier display-object release has already happened.

## Item visibility, pickup, hazards, and update order

`EntityCreateLevelItem` chooses frames from the shuffled present appearance,
ground-item table, or large-item table; it assigns fixed sprite sizes/resources and
copies saved world position. Allocation failure for small items requests relocation
through the placement helper, including the missing-argument bug below.

`EntityUpdateLevelItemVisibility` allocates/releases item display objects according
to matching pending-camera levels. Original X bounds are (-20,340); first-camera Y
limits are 244/288 for small/large items, while split second-camera limits are
132/176. These differ from the much larger Earthling activation regions.

`PlayerCollectNearbyItems` requires a live, low-height, eligible player and an
allocated item within 16 X/8 Y. Presents enter the first empty inventory slot and
reset mole contact flags; a full inventory gives dialogue. Money adds a buck;
food applies the signed lookup delta and speech/sound. Successful ordinary pickups
set the high-bit-first per-level mask and mark the record empty.

`EatYummyFood`, `EatYuckyFood`, and `EatFudgeSundae` are immediate present-triggered
health helpers (+8, -8, +12 respectively). `PlayerCheckLevelItemHazards` routes
nearby `ITEM_HURT_HAZARD` objects through `Player_Youch` using a small Earthling-shaped
source record. Its initialization is limited to fields that handler consumes.

`MapUpdateItemsAndPresents` returns when both menus are open, otherwise updates
loaded/dropped item visibility, processes each live menu-closed player's pickups and
hazards, reduces active power timers, then calls `ProcessPresents`. Final level 26
also updates Funkatron scene entities. The main loop calls this on its Earthling/
elevator/item quarter-phase and calls `ProcessTomatoRain` afterward.

`ProcessPresents` updates both players' decoy, boom box, doorway, burp, cloud,
transient effect, timed effect, telephone, and tomato-rain scheduler in that order.
Not every effect has the same per-player pause policy: its own handler decides.

## Opening and consuming a present

UI `OpenPresent` resolves `PRESENT_MYSTERY` repeatedly with `SelectRandomPresent`, then calls
`OpenSelectedPresent`. Rejection preserves inventory and reports failure; success
marks a directly selected non-mystery ID identified, closes the inventory gap, and
returns effect ID+1 (`src/ui/inventory.c`).

UI `CheckIfOpenForBoth` can apply that effect to a nearby live partner according to
`PlayersRequireSeparateViews`; it closes the partner's menu and excludes doorway,
decoy, relocation, telephone, Earthling, and boom-box IDs from duplication.
This shared-opening policy is not inside `OpenSelectedPresent` itself.

`OpenSelectedPresent` rejects elevators and death sprites. Observed dispatch is:

| Identity (`include/present_types.h`) | Operation before common setup |
| --- | --- |
| `PRESENT_ICARUS_WINGS`, `PRESENT_SPRING_SHOES`, `PRESENT_INNERTUBE`, `PRESENT_TOMATOES`, `PRESENT_SLING_SHOT`, `PRESENT_ROCKET_SKATES`, `PRESENT_ROSE_BUSHES`, `PRESENT_SUPER_HITOPS` | Timed active powers; spring shoes reject level zero. |
| `PRESENT_DOORWAY` | Create doorway. |
| `PRESENT_FOOD` | 70%-threshold yummy versus yucky food. |
| `PRESENT_ROOTBEER` | Queue 12–18 burps and add four health. |
| `PRESENT_PROMOTION` | Set points just below the next promotion threshold. |
| `PRESENT_TOGETHERNESS` | Queue global relocation/unfall; reject if one is queued. |
| `PRESENT_RAIN_CLOUD`, `PRESENT_FUDGE_SUNDAE`, `PRESENT_DECOY`, `PRESENT_TOTAL_BUMMER`, `PRESENT_EXTRA_LIFE` | Cloud, sundae, decoy, health-drain display, extra-life display. |
| `PRESENT_RANDOMIZER` | No dedicated branch here; direct selection is intercepted by UI. |
| `PRESENT_TELEPHONE` | Telephone; reject high-bit terrain. |
| `PRESENT_EXTRA_BUCK`/`PRESENT_JACKPOT` | Add one/five bucks with dialogue. |
| `PRESENT_TOMATO_RAIN` | Schedule rain and optionally an opening pose. |
| `PRESENT_EARTHLING` | Create an Earthling through `OpenEarthlingPresent`. |
| `PRESENT_SCHOOL_BOOK` | Schoolbook; reject high-bit terrain or positive height. |
| `PRESENT_BOOMBOX` | Boom box; reject high-bit terrain. |
| `PRESENT_MYSTERY` | Normally resolved by UI before this entry point. |
| `PRESENT_BONUS_HITOPS` | Remap to `PRESENT_SUPER_HITOPS`. |

Decoy also rejects high-bit terrain. Every accepted call reaches
`SetupSelectedPresent`, which adds two points; IDs from `PRESENT_DOORWAY` upward
return after that. Earlier active-power IDs install the action icon and a base
timer of 1000: `PRESENT_ICARUS_WINGS` halves it, `PRESENT_ROCKET_SKATES` quarters it,
and `PRESENT_SUPER_HITOPS` uses 750. Setup initializes ability motion/dialogue and eligible poses.
REV 02 adds a fall-state guard when opening skates and broadens one eligible sprite
flag mask; the revision branches remain explicit (`present_dispatch.c`).

`ReducePresentTimer` skips absent/dead/menu-open players and does not decrease the
timer in an elevator. Near expiry it blinks the action icon with timer masks.
At zero it repeatedly tries `PresentTryEndActivePowerup` until height/state permits
clearing the ID and restoring a terrain-appropriate pose (`item_updates.c`).

## Persistent world and attached effects

`PresentStartTransientEffect` allocates or restarts one reveal effect per player;
`PresentUpdateTransientEffect` reports not-ready during its first frames and frees
completed animation. Decoys/doorways use that result to reveal themselves.

`OpenDecoy` places an object behind the player, shifting X out of elevator footprints.
`PresentRegisterDecoyHit` increments its owner's hit count. `PresentUpdateDecoy`
removes it on level change or three hits and can create a destruction effect.
Earthling steering and projectile collision explicitly recognize decoys.

`PresentIsDoorway` aligns a left/right-offset doorway to the tile grid. Its three-row,
four-column terrain footprint overrides terrain-cache properties through
`PresentFindDoorwayOwnerAtTile`. `PresentUpdateDoorway` tracks aligned player contact,
animates open/closed states, and teleports a player crossing behind it.
`PlayerTeleportThroughDoorway` chooses collision-free terrain placement and forces
a view redraw when the destination is not safely inside the original shared camera.

`PerformPromotion` creates an attached rank/health-drain/life display, or applies the
immediate result when unavailable. `BummerLifePromo` applies ongoing drain/sound and
completion effects, restores the player pose, frees the display, and can chain a
rank promotion into an extra life. Lives are capped at eight on these paths.

`PresentUpdateBurps` resets some state-two Earthlings visible in the player's draw
mask, emits speech/sound, and selects the next random delay. Cloud creation starts
a 400–500 update timer; `PresentUpdateRainCloud` randomly starts strikes, applies
hurt or direct damage according to player state, and handles menu/elevator cases.

`OpenBoomBox` places a 300-update world object and selects `MUSIC_BOOMBOX`.
`EarthlingApplyNearbySleep` owns its sleep effect; expiry calls `EarthlingWakeLevel`
and restores ordinary music only when neither player has a boom box.

`PresentUpdateTerrainEffects` responds to fresh action for `PRESENT_ROSE_BUSHES`: some
terrain creates a persistent object, other valid terrain a temporary effect.
`PresentRetireTerrainEffect` switches the active object to its final animation;
completed persistent effects can hurt either nearby player through `Player_Youch`.
These are real world hazards, not renderer-only particles (`projectile_effects.c`).

## Tomatoes and control remapping

`EarthlingLaunchAimedProjectile` uses target velocity prediction, quantized angle
threshold/vector tables, random spread, and distance-dependent vertical acceleration.
Player-fired launch code lives in player; both feed the twelve-handle shared pool.

`ProcessTomatoRain` actually updates all shared tomatoes. It handles offscreen low
objects, terrain-dependent landing/impact animations, owner-excluding target tests,
player health damage, decoy hits, Earthling damage, and completed-object release.
Player-owned shots cause four health damage and other owners six on its eligible
player path. `TERRAIN_VOID` takes a distinct priority/state path rather than an
ordinary ground splash. It finally updates remap projectiles.

`SetTomatoRainTomatoes` chooses 50–80 pending drops. `CheckTomatoRainTimer` uses
4–8 update delays, random positions around the player, height 224, and slots 4–11.
It decrements pending count only after successful entity allocation.
`PresentReleaseFirstSharedTomato` frees the first slot in that range for eviction.

`EarthlingLaunchControlRemapProjectile` fires left/right into a separate four-slot
pool. `EntityFindProjectileTargetHit` tests players before decoys.
`EarthlingUpdateControlRemapProjectiles` applies a 240-update remap effect or decoy
hit, and handles landed/offscreen cleanup. `PlayerRemapDirectionalInput` keeps
upper action bits while mapping the nonzero direction nibble through one of four
rows; player movement handlers opt into that accessor.

`EarthlingUpdateControlRemapAttack` shoots on frame two and restores facing state;
`EarthlingUpdateAimedProjectileAttack` chooses a player/visible decoy during early
firing frames. Thus animation timing is part of attack simulation, not only visuals.

## Telephones and inventory-owned randomization

`TelephonePlaceOutsidePlayerViews` avoids an existing phone serving the level and
retries terrain-valid positions distant from either player. `OpenTelephone` instead
chooses/reuses one of two records, places behind the player, avoids immediate partner
pickup, resets activation, and starts a reveal effect (`telephone_open.c`).

`CheckForTelephone` allocates by camera visibility, starts six rings when a live
player enters its 320×224 neighborhood and sound cooldown permits, and counts
ring delays. Reaching its 16×8 pickup region frees the object, sets `gMapRevealMode`,
blocks in `DrawGameMap`, then clears reveal mode and the phone. Expiry also removes
it; `TelephoneStartRing` suppresses ring sound during elevator departure.
`TelephoneClearAll` frees/reset active records and ring arrays.

UI `ProcessInventory` intercepts directly selected `PRESENT_RANDOMIZER` and calls
`RandomizePresents`: reshuffle appearances, show/refresh both inventories, reroll
occupied slots except `PRESENT_BONUS_HITOPS` sixty times, then invalidate world-present readiness.
`InventoryIdentifySelectedPresent` resolves mystery IDs and charges two bucks only
when identification or resolution changes something (`src/ui/inventory.c`).

## ORIGINAL BUG list

- `EntityCreateLevelItem`: allocation failure calls the four-argument
  `MapFindItemPlacementPosition` with only three arguments; `large` is missing.
- `PlayerCollectNearbyItems`: full-health positive food still disappears because
  the collected flag remains set on the "stuffed" path.
- `MapFindItemPlacementPosition`: rejected marked/excluded tiles do not consume
  random attempts, so an entirely rejected map need not reach the fallback scan.
- `MapPlaceLargeLevelItems`: its unit retains an `int` result view of the shared
  unsigned-char-returning `GetMapTerrainAtWorldPosition` (`level_item_setup.c`).
- `PerformPromotion`: the no-display rank path passes an object pointer to
  `GenerateSpeech` where the interface expects a speaker/player number.
- `ProcessTomatoRain` and `EarthlingUpdateControlRemapProjectiles`: their unit
  retains the `int` terrain-query result view (`projectile_effects.c`).
- `ProcessTomatoRain` and `EarthlingUpdateControlRemapProjectiles`: their unit also
  retains an `int` view of the shared short-returning `PresentRegisterDecoyHit`.

Return-view comments preserve observed matching ABI behavior; historical missing
declarations are [INFERENCE], not a proven explanation or an observed gameplay
symptom. UI-owned findings are documented only in the
[UI subsystem](ui.md).

## Open questions

- Direct `PRESENT_RANDOMIZER` selection is special; `OpenSelectedPresent` has no
  equivalent branch. Mystery resolution and direct calls are not assumed identical.
- Named content identities now include `PRESENT_INNERTUBE`; animation group names
  describe sprite families, not independently maintained powerup flags.
- Frame completion, allocation failure, visibility, and per-effect menu policies
  are observable behavior boundaries that a data-driven rewrite must specify.

## Port notes

- **Platform layer:** `PlayerTeleportThroughDoorway` manipulates VDP/blanking state
  during redraw; `PresentStartTransientEffect` allocates shared entity resources.
  [INFERENCE] Present creation should emit simulation events with independently
  renderable effects rather than blocking on presentation work.
- **Shared-screen coupling:** `MapUpdateItemsAndPresents` gates work with
  `gPlayerInventoryOpen`; `CheckIfOpenForBoth` can close the partner's menu, and
  `CheckForTelephone` blocks in the map screen. [INFERENCE] Online co-op needs
  authoritative effect ownership and per-player nonblocking menus; retain global
  identification only as a deliberate gameplay rule.
- **Determinism:** `ReducePresentTimer` applies player/elevator/menu gates,
  `ProcessPresents` fixes effect update order, and `LoadLevelItemBank` exchanges and
  restores the level seed. [INFERENCE] Use fixed ticks, explicit RNG streams, and
  replicated item/effect state; do not derive outcomes from render allocation.
- **Content tables:** `SetupSelectedPresent` embeds timers/ability setup;
  `SelectRandomPresent` reads weights and `MapPlaceLargeLevelItems` defines placement.
  [INFERENCE] Externalize IDs, restrictions, values, effects, and bindings while
  preserving mystery, reroll, timed-power, and world-spawn distinctions.
- **Camera/visibility:** `EntityUpdateLevelItemVisibility`/`CheckForTelephone` use
  fixed view bounds; `ProcessTomatoRain` uses visibility during projectile cleanup.
  [INFERENCE] Widescreen needs new presentation bounds and a defined activation
  policy so wider views do not silently change spawns, hits, or effect expiry.
