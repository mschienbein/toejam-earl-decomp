# UI: HUD, inventory, text, windows, menus, and pause

## Purpose and evidence

UI combines presentation with substantial gameplay policy: inventory mutation,
mail-order purchases, present identification, health/life checks, and promotion.
Menus use the same tilemaps, object pool, sprite chains, RNG, and VBlank services
as gameplay rather than an isolated overlay system.

Evidence is `src/ui/*.c` and the referenced headers. Source paths describe the
current repository-root layout; `src/ui/window_panels.c` owns the window and
transient-cleanup primitives. Identifiers refer to source functions/shared state.

## Files and roles

- `inventory.c`: two-player inventory input, redraw service, item transactions,
  font banks, tile text, selectors, six visible presents, and banners.
- `main_menu.c`: character/game/controller menus, cursor/header entities,
  controller-presence test, game-over timeout/reset.
- `pause.c`: global pause display, input sequence, and final-level reset choice.
- `hud.c`: level digits, action/life icons, maximum-health and health-bar
  initialization/interpolation, full HUD, ranks, promotion, resources, and death/swim checks.
- `window_panels.c`: tile borders, nested panels, pause choices, and transient cleanup.
- `include/ram_ui.h`: menu state, aliases, inventory objects, choice/life-request state.
- `include/assets_ui.h`: maps, labels, fonts, icon and menu-table resources.
- `include/ui.h`: eight-byte `menu_item`, sixteen-byte elevator `ride_speech`,
  and the shared `dialogue_descriptor` used by speech.
- `include/ui_calls.h` and `menu_calls.h`: shared UI/menu interfaces.

## Key state and storage aliases

`gPlayerInventoryOpen[player]` is not just boolean: observed values are ordinary inventory
`1`, mail order `2`, and identification `3`; zero is closed.
Its second byte also has the scalar `gPlayer2InventoryOpen` name (`ram_ui.h`).
Separate arrays track scroll row, selected row/column, and action-banner mode.
`gInventoryRedrawPlayer` plus `gInventoryRedrawPhase` coordinate one shared redraw service.
`gInventorySavedCamera*` hold temporary camera suspension state.
`gInventoryLabelsSuppressed` supports the randomizer animation.

The two selectors alias entity records 26 and 33; six present entities follow
each at 27..32 and 34..39. Their scalar flag names alias `behavior_flags`, not
new allocations. This shares storage with the expanded level-26 entity pool.
`gTextFontBuffers[][1888]` stores two writable font banks.
`gTextFontBank`, `gTextFontAnimationTick`, and `gTextFontAnimationEnabled` control font alternation.
Mailbox items/prices have three entries per player.
`gMenuCursorEntity` and `gMenuHeaderEntity` are dynamic object handles.

HUD uses fixed hardware sprite records: level digits 11..14, health bars 15..18,
lives 19..20, and action icons 21..22 (`hud.c`).
`gHudDisplayedHealth` is distinct from actual health, and death checks read both.
`gPlayerSelection`, rank, points, lives, bucks, powerups, and restore-health state
come from player/effect RAM interfaces, not a self-contained UI model.

## Inventory open/redraw/close state machine

`DrawInventory` clears the player's viewport sprites in split mode, suppresses
the player sprite, and stops its animation countdown. It selects the banner,
creates mailbox offers when appropriate, resets the REV 02 idle counter,
initializes seven display entities, and requests redraw phase four.
The loop waits until interrupt-time service reaches zero; it publishes the
menu-open value once background phases have progressed far enough.
In shared mode it repurposes the secondary camera origin as `(0,112)`.

`InventoryServiceRedrawPhase` decrements `gInventoryRedrawPhase` after each service:

- Phase 4 and 3 upload seven rows each to the requested half of the window map.
- Phase 2 forces black text background, draws stats/banner, then HUD.
- Phase 1 draws mail-order offers or ordinary contents; identification also draws bucks.

Byte-map flags become VDP horizontal/vertical flip attributes during upload.
`InventoryRedrawSynchronously` can service all phases directly;
`InventoryRefreshContentsAndWait` waits for any outstanding operation and then
requests only contents phase one. These are different synchronization paths.
`InventorySetCameraSuspension` saves/replaces only open split-view cameras and
restores them later, using Y 0 or 112 for menu space.

`ToggleInventory` releases display entities and player skip-physics state.
Closing split mode can trigger a forced merge when the partner has no menu and
the players no longer require separate views; otherwise it requests HUD redraw.
It restores suppression/animation/view residency. Closing shared mode restores
secondary camera coordinates and redraws HUD with graphics service gated.
Opening is inhibited during the relevant elevator-departure condition.

## Inventory input and transactions

`ProcessInventory` handles logical-player button edges, not held-button repeat.
It skips absent players and the two tested unavailable sprite states.
B toggles inventory; the map/action button toggles open/drop banner in mode one;
the present-action button purchases, identifies, drops, randomizes, or opens.
A fresh direction edge changes one of three visible rows, scrolls up to row five,
or toggles columns outside mail-order mode.
`InventoryUpdateSelectorEntity` retreats over empty slots and hides an exhausted selector.

`CheckBButton` calls `ProcessInventory` and, in a shared screen, enters a blocking
loop until both menus close. It alternates sprite service and VBlank waits,
sampling input and servicing inventories, not ordinary partner/world simulation.
In split mode it returns to the regular game loop instead.
`InventoryOpenMailOrderMenu` and `InventoryOpenIdentificationMenu` pre-latch the
action button in both current/previous input so entry does not immediately act.

Selection indexes `(scroll + row) * 2 + column` in a 16-item inventory.
`OpenPresent` resolves mystery entries, applies the effect, marks identification
except for the special original slot value, compacts the inventory, and returns
`present+1`; zero means no successful opening.
`InventoryDropSelectedPresent` rejects the elevator case, otherwise compacts then
hands the removed item to `DropPresent`.
`InventoryBuySelectedMailOrderPresent` requires a free slot and sufficient bucks,
charges the selected price, and produces speech/error feedback.
`InventoryIdentifySelectedPresent` costs two bucks only when resolving mystery
or changing identified state; already known/empty/poor cases do not charge.

`CheckIfOpenForBoth` may apply one player's opened present to the living partner
when `PlayersRequireSeparateViews` is false. Doorway, decoy, togetherness,
telephone, earthling, and boombox are excluded from that replication path.
The decision is based on view-separation policy, not only the current split flag.
A pending un-fall relocation can close the other inventory before moving a player.

## Randomization and seeded offers

`SetMailOrderPresents` saves RNG state, seeds from the player's level seed,
chooses three distinct gifts excluding extra life/promotion/jackpot, sets prices
in `3..5`, then restores RNG state. This is explicit local reseeding.
`RandomizeInventory` rerolls occupied slots except value 27.
`RandomizePresents` resets identities, may open the partner's inventory, hides
selectors/labels, then performs sixty rerolls with progressively longer waits.
It refreshes both contents, queues sprites, plays sound, clears world readiness,
and closes an inventory opened only for the effect.
This is a gameplay mutation presented through a blocking animation.

## Text and window rendering

`TextCopyFontBanksToRam` copies two 1888-byte source banks into consecutive RAM.
`TextAnimateFontBanks(rate)` swaps and DMA-uploads a 944-word bank at the selected
rate. Its nonzero return consumes core's ordinary graphics-service slot.
`DisableTextFontAnimationIfMenusClosed` leaves animation enabled while either menu is open.
`ForceTextBackgroundColorToBlack` selects black or dark blue in one CRAM entry.

`TextMapCharacterToGlyph` maps case-insensitive letters, digits, punctuation,
and six embedded control values. Some controls emit extra tile words before
returning the last glyph, so one input byte need not equal one tile.
A question mark advances the shared 59-word pre-generated-random cache and
chooses one of four glyphs. Unknown characters map to blank.
`TextWriteStringTiles` adds the font tile base and writes zero-terminated text.
`TextWriteDecimalTiles` suppresses leading zeros while retaining a final digit.

`LoadPresentsIntoInventory` shows a 3-by-2 item grid, authored present appearance,
and either known labels or randomized unknown glyphs. Togetherness is labeled
un-fall when the partner is absent. Mail-order uses three rows with prices.
`InventoryDrawActionBanner` selects one of four compact tilemaps and records it.
`DrawMenuWindowFrame` randomizes outer border tiles/flips over a 40-by-28 window.
`DrawNestedWindowFrame`, `DrawPresentDisplayPanel`, and `DrawPauseChoicePanel`
write fixed rectangle resources, with interrupt graphics work gated as needed.

## HUD and gameplay consequences

`DrawHUD` skips level 26; otherwise it draws a combined five-row lower window
when unsplit with no inventories, or repositions the alternate arrangement.
It updates all HUD sprite positions and uploads the relevant table segment.
Level digits are uploaded separately and omit a zero tens digit.
Action icons select a default character icon or a powerup-specific piece.
Life icons change image scheme above four lives (`HudLoadLivesSpriteTiles`).
Maximum health is 23 for ToeJam, 31 for Earl, plus four per rank.
`HudAnimateHealthBar` clamps actual/restore health, advances displayed health one
unit toward actual health, and rewrites affected packed-pixel rows.
It also updates changed life icons and skips absent/ghost/final-level players.

`GetScoreForPromo` computes 40 plus accumulated `(prior_rank+3)*20`, returning
zero for rank eight and above. `CheckPromotionScore` explicitly stops at eight,
refreshes health graphics, and requests combined HUD redraw when appropriate.
`SetupTheMainGame` gives active players level one, three lives, three bucks,
base health, no powerup, and fresh HUD resources.
`PlayerResetRankAndResources` has separate solo/two-player reset resource rules.
`PlayerUpdateHealthAndLives` waits for displayed as well as actual health to reach zero
before death, decrements lives subject to its observed last-life branch, handles
saved health during swimming, drains health according to `gFrameCounter & 12`,
and invokes promotion. Presentation state therefore affects gameplay timing.
`HudInitializeHealthBar` resets displayed health, installs the two bar sprites,
clears the `HUD_HEALTH_BAR_TILES` tile span, and paints the maximum-capacity
bar pattern from character base health plus the rank bonus for active players.
Its immediate path DMA-uploads the pixels and animates displayed health toward
actual health; its queued path appends the tile run, waits for the upload request
to clear, and leaves displayed health at zero.

## Main menus and pause

`NewGameMainMenu` selects both/ToeJam/Earl, sets absent-player sentinel resources,
then loops over game choices: gameplay, Jam Out, intro replay, controller options,
or alternate world type. Physical input can swap ports for solo selection.
`GeneralMenuHandler` redraws the scene, waits for button release, creates a
cursor, and advances on fresh input. Negative-selection entries are nonselectable.
It refuses two-player selection when `AreBothControllersConnected` fails.
That test directly changes controller direction/data registers while holding
Z80 bus request; it is a hardware-specific presence heuristic.
Menu servicing advances RNG once per iteration.
`ShowGameOverAndReset` waits for input or counter greater than 600, clears display
resources, stops sound requests, and performs hardware-style restart.

`GamePaused` creates an eviction-backed pause entity and runs a blocking input
loop until either player's new Start press. It silences/releases sound channels
and invalidates music selection on exit. It does not run world simulation.
Its four input combinations can rewrite ship-piece level assignments, preserving
only the earliest remaining piece and moving it to the next level if needed.
`RunFinalLevelPauseMenu` instead presents resume/reset selection for that player.

The pause allocator is implemented in `src/map/land_processing.c`.
`AllocateEntityWithEviction` first tries an unused slot unless release-first
is requested, then reclaims mole effects, speech, shared tomatoes, item objects,
and, per loaded earthling bank, secondary then non-ship primary objects.
`EntityReleaseOneByPriority` uses that same order to make a slot available.

`ClearTransientEntities` is housed in the window file but frees projectiles,
speech, effects, decoys, choices, boomboxes, rain state, dropped presents, and
telephones. Its cross-subsystem cleanup role must not be mistaken for window-only work.

## ORIGINAL BUG list

- `NewGameMainMenu`: reads an incoming short argument although
  `FurtherInitialisation` calls it without one, before the first menu overwrites it.
- `CheckBButton`: uninitialized modulo-three sprite-service counter.
- `TextWriteDecimalTiles`: negating `-32768` narrows back to a negative short.
- `PlayerResetRankAndResources`: lacks the rank-eight bound; the zero threshold
  makes the recomputation loop endless at points at least 880.

## Open questions

- The exact historical declarations are not recovered by a matching qualification;
  compiler-shape comments should not be presented as original design intent.
- Input/action numeric states remain in several places despite named constants elsewhere.
- Local menu behavior in split mode is not proof that all menu-triggered effects
  can run concurrently; randomization and redraw ownership remain global.

## Port notes

- Platform layer: replace `InventoryServiceRedrawPhase` tilemap/CRAM work,
  sprite DMA, controller presence probes, and busy waits with UI services.
- Shared-screen coupling: replace the blocking `CheckBButton` and `GamePaused`
  loops with per-player UI state machines. Preserve `gPlayerInventoryOpen` and
  `gSplitScreenEnabled` action policy explicitly while partner simulation continues.
- Determinism: separate `DrawMenuWindowFrame`/glyph cosmetic RNG from simulation,
  preserve `SetMailOrderPresents` reseeding, and move death rules out of HUD animation.
- Content tables: externalize menu entries, labels, prices/exclusions, glyph maps,
  ranks, HUD layouts, banner maps, and animations; support localization separately.
- Camera/visibility: widescreen/per-player menus should use explicit UI rectangles,
  not fake secondary-camera coordinates, HBlank aliases, or scarce world-object slots.
