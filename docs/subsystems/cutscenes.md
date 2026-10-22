# Cutscenes: Sega, title, intro, ending, and Funkatron presentation

## Purpose and evidence

Cutscenes uses the gameplay entity/tile renderer to stage logos, multi-part
characters, captions, rocketship sequences, credits, and the interactive final
planet. Script state/countdowns live in global RAM, while each part runs a
blocking C loop with its own rendering, input, and sound cadence.

Evidence is the C/header implementation at the current repository-root paths
below, using named fields/constants. `src/cutscenes/funkotron_scene.c` retains
the function/resource spelling `Funkatron`. No dialogue transcript or asset
bytes are reproduced here.

## Files and roles

All C paths in this list have prefix `src/cutscenes/`:

- `sega_logo.c`: logo planes, palette cycle, one-time rocketship pass/trademark.
- `sega_logo_vblank.c`: palette/sprite/audio service and frame-counter ownership.
- `title_and_transition.c`: animated backgrounds, logos, borders, metatile panels,
  attract sequence, common frame helper, and flagged-entity cleanup.
- `intro.c`: dispatcher and `IntroPart1` through `IntroPart10`.
- `intro_part11.c`: final intro dialogue and ship-piece objective explanation.
- `ending.c`: ending dispatcher and five staged ending sequences.
- `cutscene_text.c`: centered 32-glyph runtime text, palette remapping, two handles.
- `large_player_parts.c`: eight large-player parts, three driving parts, and asteroid.
- `intro_frame_entities.c`, `intro_frame_reveal.c`, `intro_frame_release.c`:
  ten hidden rocketship components and their reveal/release.
- `drifting_entities.c`: eight drifting background entities and flying-slot lifecycle.
- `flying_entities.c`: rightward flight and four-object driving-scene motion.
- `collision_scene_entities.c`: collision parent/shared copies and release.
- `rocketship_allocate.c`: ship/exhaust object pair construction.
- `rocketship_pose.c`: pose/sequence replacement and viewport reallocation.
- `rocketship_foreground.c`: priority foreground ship without resident exhaust.
- `rocketship_move.c`: leftward movement and wrapping.
- `rocketship_release.c`: exhaust-before-ship release and handle clearing.
- `composite_entities.c`: pose/six-piece/right-offset/elevated ending composites,
  falling ship, and shared pair positioning.
- `impact_and_planet_entities.c`: impact, planet, stationary ending composite.
- `ending_player_entities.c`: solo/paired animated ending players.
- `ending_player_release.c`: release and first-player-frame query.
- `funkotron_scene.c`: data-driven level-26 placement visibility and interactions.

Headers are `ram_cutscene.h`, `ram_intro.h`, `assets_cutscene.h`, `assets_intro.h`,
`assets_scenes.h`, `cutscene_calls.h`, `cutscene_text_calls.h`, `title_calls.h`,
`intro_calls.h`, `intro_object_calls.h`, and `reset_calls.h`.
Shared entity/frame definitions remain in `entities.h` and `sprites.h`.

## RAM state and resources

`gCutsceneScriptState` is a global short; `-1` marks completed intro/ending parts.
`gCutsceneScriptCountdown` advances each part's timed states.
`gCutsceneAdvanceButtonMask` holds face-button input while waiting for release.
`gCutsceneMusicActorAnimationEnabled` gates a music-event actor pulse.
These are reused between parts, not an array of independently running scripts.

`gSceneBackgroundPairIndex`, frame phase/timer, background-advance counter,
scene-update counter, and last accepted music-event time drive shared backgrounds.
`gTitleLogoSelector` selects one of two logos.
`SEGA_PAL_LOAD_FLAG` plus `SEGA_PAL_TO_LOAD` publish a pending palette to VBlank.
Sega state also remembers whether sprite updates are enabled, trademark handle,
and whether the once-only rocketship pass has already been shown.

Object handle groups include eight large-player parts, three driving parts,
ten intro components, eight drifters, four flying slots, ship/exhaust,
collision parent/two three-slot arrays, asteroid, impact, and planet.
Ending has two player handles and several composite handles.
`gCutsceneFirstTextEntity` and `gCutsceneSecondTextEntity` are ordinary pool handles.

`held_frame2` is frame byte plus FE hold command. Frame tables remain variable
counted metasprites, not fixed arrays inside `render_frame` (`sprites.h`).
Funkatron resources contain 30 descriptors and 277 placement records; descriptors
supply frames/sequences, period, bounds, and reservations, while placements contain
X/Y and interaction ID. Zero X separates descriptor groups (`assets_scenes.h`).

## Attract sequence and Sega logo

`SEGAAndIntroVideoRoutine` enables the supporting VBlank service flag and loops
Sega, title, and intro until title Start or intro interruption. If title Start
ended the loop, it still runs the intro once before restoring game palettes.
It then clears that service flag and restores the gameplay video mode.

`SEGAVideo` installs `SegaLogoVBlank`, clears/prepares planes, uploads logo
resource/map, queues its palette, and enables display. REV 02 resets the frame
counter here. It cycles 20 palette frames at three-tick intervals for 120 ticks.
On the first pass only, it creates trademark and ship/exhaust objects, moves
the ship from X 280 past -50, alters Y in two short X ranges, and emits sneeze/
tone sound cues at exact positions. It frees these objects and remembers the pass.
It waits one final counter change before clearing the VBlank override.

`SegaLogoVBlank` consumes the queued palette into slot three, optionally loads
sprites/selects viewport zero, services `MassiveRoutine`, and increments
`TIME_COUNTER`. Core's ordinary VBlank counter increment is bypassed by override.
`SegaLogoCreateTrademarkEntity` reserves ten pieces/108 tiles and uses a held
frame; its release clears the stored handle.

## Title and shared background cadence

`TitleScreenVideo` disables display, installs title palettes/video state,
initializes transition tiles, draws the selected logo, chooses music event
channel two, and enables display. In REV 02 it resets both time and last event
time; REV 00 instead measures elapsed time from a saved start counter.
The title runs until 1200 ticks or Start, waiting for Start release before
returning a positive result. Each iteration also advances RNG.
An accepted cadence completion toggles `gTitleLogoSelector` and redraws its map.

`AnimateTitleTransitionBackground` selects two frames per background pair on an
eight-call timer, decodes exactly four 16-tile pieces, queues a 64-tile upload,
waits VBlank, and loads tiles only. Its no-change path also waits one VBlank.
`InitializeTitleTransitionTilemap` repeats an eight-by-eight pattern across a
64-by-32 map, incorporating the global camera priority attribute.
`TitleDrawSelectedLogo` uploads either logo and its 40-column mapped rows.

`CutsceneStepFrameAndMusicCadence` animates that background, queues entities,
waits/loads sprites, selects viewport zero, and increments scene updates.
It clears observed music events, accepts them only ten counter ticks apart,
optionally pulses a large-character sequence, and returns zero when accepted
event processing finds scene updates above 51.
With an advance period it also cycles the background pair modulo 24.
This mixes update counts, VBlank waits, and music events; it is not one uniform
cutscene tick equal to one display frame.

## Intro dispatcher and state machines

`IntroVideo` zeroes scroll/camera origins, loads font/palettes/background/border,
enables display, and short-circuits through `IntroPart1..11` in order.
Each part returns nonzero when interrupted, not when completed normally.
It finally disables display, releases text, clears both maps, invokes flagged
cleanup, and restores scroll origins before returning interruption status.

The main part families are:

| Part | Observed construction and progression |
| --- | --- |
| 1 | Eight-part large characters; caption/pose conversation, timed or released face-button advance. |
| 2 | Ten hidden frame components reveal the ship; captions, then transition music/background advancement. |
| 3 | Return to large characters for the problem announcement and reaction. |
| 4 | Ship/exhaust plus eight drifters; parallax scroll, alternating ship movement, captions. |
| 5 | Three driving parts and four flyers; timed pose changes and reversible background motion. |
| 6 | Dodging ship poses and rightward flying object; timed tone cues and scrolling. |
| 7 | Driving parts/flyers, alternating five-pixel camera shake, asteroid creation/sound. |
| 8 | Crash tiles/shared-copy mode; create collision objects, wait, release, restore background. |
| 9 | Falling ship inside bordered scene; second-plane vertical scrolling across timed states. |
| 10 | Ship descends toward planet, replaced by impact, followed by driving-warning caption. |
| 11 | Large-character conversation about Earth and recovering ten ship pieces. |

Start interrupts the tested loops. Face-button release advancement appears in
caption-centric parts, but should not be generalized to every timed driving part.
Most parts explicitly release the objects they own, reset special scroll state,
and perform one final shared-frame service. Ending a dispatcher does not itself
prove every helper handle is cleared; some release helpers intentionally retain it.

## Scene object construction

`IntroAllocateLargePlayerParts` and `IntroAllocateDrivingParts` sum each selected
frame's piece width times height to reserve tile area. Position helpers install
held-frame sequences and primary sprites; selected driving parts use Y/elevation
biases 100/101 to establish layering without a new coordinate record type.
Eight large parts exist, but `IntroPositionLargePlayerParts` shows seven initially.
The asteroid reserves twelve pieces/160 tiles.

`IntroCreateHiddenFrameEntities` uses only slot one of
`AnimationPointerWords_AF040` as a proved frame table, constructs ten hidden
objects, and sums tile area. Slot zero is not evidence of another typed animation
descriptor. Reveal clears hidden; release clears handles and refreshes sprites.

`CutsceneAllocateRocketshipPair` reserves 101 tiles/eight pieces for ship,
15 tiles/two pieces for exhaust. Pose setters release/reallocate resident
primary sprites and install corresponding ship/exhaust sequence variants.
Foreground pose sets priority and releases exhaust sprites without deleting it.
`CutsceneMoveRocketshipPairLeft(distance)` subtracts the requested distance from
the ship but always one pixel from exhaust; each wraps below -60 to 320.

Drifters randomize position/speed, exclude the open Y interval 150..170, and
advance X on a fraction-byte timer, wrapping past 370 to a random negative X.
`IntroUpdateRightwardFlyingEntity` moves the first slot ten pixels right and
releases sprites after X 232. `IntroUpdateFlyingEntityMotion` selects scene-specific
spawn/drift, respawns outside its X/elevation bounds, and delays sprite allocation
until its signed countdown reaches zero. Its random mask only produces variants
zero/one even though comparisons for two/three remain in the source.

Collision construction builds one parent and three shared-tile copies; release
also clears the separate auxiliary three-slot list. Composite helpers select
distinct frame/sequence resources, fixed reservations, priority, elevation,
and relative-position offsets. Several impact/planet/stationary release helpers
free without zeroing the handle; this is stated behavior, not assumed idempotent
ownership for arbitrary interleaved scene lifetimes.

## Runtime caption rendering

`CutsceneTextLoad` releases both existing text handles, clamps a line to 32
characters, allocates eight pieces/32 tiles, and uses the remapped text frame.
It stores Y+100 and height 100, yielding the intended on-screen Y through the
normal elevation renderer. `CutsceneLoadSecondTextLine` adds the second handle
without discarding the first. Original-palette loading uses a separate frame
with zero elevation.

Both queue functions pad a line to 32 tiles and enqueue destination/count words.
The remapping helper translates both packed 4bpp nibbles: original font values
11 to 1, 10/9 to 2, 8 to 15, otherwise zero (`cutscene_text.c`).
Character mapping is shared with UI; glyph 27 is incremented to avoid that variant.
Caption text therefore still uses the shared random-cache behavior for question marks.

## Ending flow

`PlayEndingSequence` resets scroll/display service, loads font/level resources,
sets palettes/starfield, and calls five parts in a fixed order:

1. `EndingPlayDialogueAndDeparture`: nine-metatile background, ship, pose and
   active-player composites. Solo/paired celebration text differs. A specific
   animation frame triggers clap, composites change, transition music drives a
   mouth-like display, then increasing speed sends the ship left.
2. `EndingPlayUpwardRocketshipDeparture`: planet scene; a halved move period
   accelerates upward/left movement and paired-player driving dialogue.
3. `EndingPlayScrollingCredits`: drifters and ship over parallax starfield;
   caption pages alternate with short blank intervals while ship moves every
   third local animation phase. It does not use the intro's input-skip condition.
4. `EndingPlayDescendingRocketshipScene`: stationary composite, leftward movement,
   then progressively tighter vertical steps and a downward exit.
5. `EndingPlayBouncingRocketshipScene`: repeated metatile background, plane scroll
   by five, alternating X movement, vertical velocity/acceleration and companion
   swaps around height thresholds; it ends after the ship passes X -30.

Each shown ending loop runs to its state/motion condition rather than checking
Start. `EndingCreateActivePlayerEntities` selects solo/paired streams according
to living players. `EndingGetFirstPlayerAnimationFrame` tests either handle but
indexes the first handle in both branches; this is observed, not corrected here.
The dispatcher finally releases captions, clears the map, and invokes flagged cleanup.

## Interactive Funkatron scene

`FunkatronUpdateSceneEntities` scans placement groups but services only one
quarter of descriptor groups per call using a rotating two-bit phase.
It restricts the final two groups according to solo-player indicator.
Camera-relative bounds use width 320 and height 224 plus descriptor margins;
visible placements allocate level-26 entities, invisible placements free them.
Thus visibility controls actual entity existence here, not only draw submission.

For descriptor groups 15 and above, proximity 80-by-50 selects interaction versus
idle animation. Tighter 30-by-20 proximity can start placement dialogue, with
character-specific exclusions, and latch successful dialogue so it does not repeat.
Selected interaction IDs also draw the present display panel.
Descriptor six draws late, descriptor thirteen is noncollidable, and most other
created objects are collidable (`FunkatronCreateSceneEntity`).

## Interactions and cleanup caveats

- Core/audio continue through VBlank while scene loops replace ordinary world work.
- Sprites supplies all metasprite allocation, animation, tile staging, and priority.
- Jam Out reuses transition/text/crash backgrounds; intro crash reuses Jam tile blocks.
- Funkatron uses player proximity, speech, and UI present panels despite this folder.
- `CutsceneReleaseFlaggedEntityRange` advances a pointer from record zero while
  passing deletion indices six through 39. That exact offset mismatch is visible;
  it must not be described as simply testing/deleting the same record.

## ORIGINAL BUG list

- `IntroCreateCollisionSceneEntities`: parent pointer is uninitialized if parent
  allocation fails but a later shared-copy allocation succeeds.
- `EndingCreateActivePlayerEntities`: local `AllocateEntity` declaration retains
  an observed int result view rather than the shared short result; a historical
  missing declaration is explicitly [INFERENCE] in the source comment.
- `EndingCreatePoseComposite`, `EndingCreateSixPieceComposite`,
  `EndingCreateRightOffsetComposite`, `IntroCreateFallingRocketship`, and
  `EndingCreateElevatedThreePieceComposite`: the same marked int-view allocator
  declaration applies to these consumers in `composite_entities.c`.
- `EndingCreateStationaryComposite`: `current_frame` receives the frame-table
  address, not its first frame.
- `EndingPlayDialogueAndDeparture`: its `EndingGetFirstPlayerAnimationFrame`
  consumer retains the marked int view of a short-returning function.
- `EndingPlayBouncingRocketshipScene`: `movement_frame` increments uninitialized,
  making the initial nine-step acceleration phase indeterminate.

## Open questions

- Allocation-failure assumptions and stale-handle lifetimes are not consistently
  guarded; no extra original-bug labels are invented beyond source comments.
- Some whole-renderer queue/preparation bodies are absent from the integration
  tree, limiting proof of shared-copy and animation advancement internals.
- Several composite resources retain address-based names; their exact depicted
  identities are not established by the constructor's frame pointer alone.
- The flagged cleanup offset mismatch and first-player frame fallback are
  observed oddities without `ORIGINAL BUG:` classification in those functions.

## Port notes

- Platform layer: replace `gVBlankFunc` overrides, VDP planes/scroll/palettes,
  sprite uploads, and controller busy loops with an explicit scene player.
- Shared-screen coupling: define session-wide versus local scene ownership;
  `IntroVideo` ORs pad inputs, while online skip/advance needs ordered agreement.
  Per-player inventory must not inherit globally blocking scene loops.
- Determinism: define `CutsceneStepFrameAndMusicCadence` script ticks and music
  events independently of display/audio callback rate, version drifter/border RNG,
  and choose defined initial movement phases rather than host residue.
- Content tables: externalize caption/state timing, poses, music cues, actor
  placements, descriptors, borders, credits, and resource identities.
- Camera/visibility: preserve widescreen composition and separate shake from
  world origins; `FunkatronUpdateSceneEntities` camera-driven creation needs
  explicit gameplay policy before introducing independent online viewports.
