# Jam Out: interactive music and replicated dancers

## Purpose and evidence

Jam Out is a dedicated front-end mode with its own VBlank callback, controller
routing, sampled/FM instruments, music-driven backgrounds, and repeated dancing
character objects. It is entered from the new-game menu, not the gameplay loop.

Evidence is `src/jamout/jamout.c`, `include/{ram_jam,jam_calls,assets_jam}.h`,
and the shared scene/audio helpers at repository-root paths.
See `docs/subsystems/audio.md` for soundtrack implementation and
`src/cutscenes/title_and_transition.c` for `CutsceneStepFrameAndMusicCadence`.

## Files and roles

- `src/jamout/jamout.c`: mode lifecycle, two VBlank handlers, animation selection,
  copy allocation/synchronization, layouts, and scroll.
- `include/ram_jam.h`: state counters, nine handles per character, input routes,
  sequence/lock state, sample selections, and alternate music-bank flag.
- `include/assets_jam.h`: character frames/sequences and two crash-scene maps.
- `include/jam_calls.h`: call boundary used by main menu and scene machinery.
- `src/ui/main_menu.c`: menu selection and return to player selection afterward.
- `src/cutscenes/title_and_transition.c`: shared frame/upload/music cadence helper.
- `src/asm/sound*.s` and `src/z80/sound_driver.s`: instrument requests/playback.

## Key state

- `gJamSelectedSong`: requested menu selection, retained even as local song is adjusted.
- `gJamLayoutPhase`, `gJamLayoutCountdown`: six presentation phases.
- `gJamHorizontalScroll`, `gJamVerticalScroll`, `gJamScrollStep`: plane scrolling.
- `gJamToejamEntities[9]`, `gJamEarlEntities[9]`: dynamic-object handles.
- Lead sequence/frame state is cached separately for ToeJam and Earl.
- `gJamToejamAnimationLocked` and Earl equivalent protect selected actions.
- Per-player previous input and pending animation input are distinct from the
  unsigned audio-route-one/two pending input words.
- `gJamSongCycleIndex` and `gJamTemplateBankCycleIndex`: random-mode Start actions.
- Six sample-index words select A/B/C PCM resources for the two audio routes.
- `gMusicAlternateTemplateBankEnabled` is shared with the music driver.
- `gSharedTileSpriteCopiesEnabled` is renderer-global, not Jam-private allocation.
- `gVBlankFunc` replaces ordinary core VBlank while this mode is active.
- `gMusicAnimationEvent` is the music event flag consumed by shared scene cadence.

## Entry and exit flow

`StartJamOut(song)` first waits until both controllers report no input.
It disables visible display, requests `Sfx_Jammin`, records selection, installs
`JamOutVBlank`, and enables alternate music templates.
It uploads crash-screen graphics and palettes, enables shared-tile copies,
allocates character copies, configures video mode, clears camera tile attributes,
and resets the layout/scroll state.

Selection seven chooses random samples: it picks six indices modulo 42 and
replaces the callback with `JamOutRandomSampleVBlank`.
For nonzero selection it decrements the local song number, waits a VBlank,
and forces a music-bank comparison mismatch to start that selection.
Selection zero requests release/track reset instead of a backing song.
It then enables display and creates the exit instruction text.

The mode loop calls the shared frame helper with advance period three,
updates layout/scroll, updates lead animations, checks Start hold, and consumes
each player's pending animation input.
Exit requires 51 consecutive loop updates with either controller's Start bit
held; releasing Start resets the hold count to zero.
That is an update count, not an explicit seconds value.

Exit disables display, clears the VBlank override/alternate bank, releases text,
clears upper/lower tilemaps around a layout reset, requests normal song selection,
releases copies, disables shared-copy mode, and releases flagged scene entities.
`NewGameMainMenu` then returns to player selection rather than resuming gameplay.

## Standard VBlank controller and audio flow

`JamOutVBlank` changes song templates when the requested/loaded selections differ;
otherwise it services `MusicUpdateSequencer`. It increments `gFrameCounter` itself.
It alternately polls one logical player per frame-counter parity, respecting
physical-port swap. Any change of the complete input byte copies the whole
current input into audio and animation pending words.
This is not a per-button rising-edge mask: a changed combination can include
other still-held buttons.

When `gPlayerSelection == 1`, it swaps audio route words and temporarily increments
the frame counter again for routing parity, then decrements it before return.
The final net counter increment remains one.
Animation words stay associated with player/character, not swapped audio route.

Route one requests PCM:

- A (`0x40`): bass drum, plus event flag when no selected song.
- B (`0x10`): snare drum.
- C (`0x20`): clap.
- Up/down/left/right: ToeJam, Big Earl, rap, and Jammin samples respectively.
- Start (`0x80`): record scratch.

Route two requests FM channel templates 22/20/19 for A/B/C and 21 for direction
or Start (`fm_effects.h`). A can also set the no-song event flag.
Each serviced route clears its pending word.
Multiple set bits issue multiple requests in source order. The matched
`PlaySound`/`PlaySoundEffect` wrappers accept idle requests or requests at least as
high-priority as the current effect; later accepted requests can replace pending
values before the sequencer services them. Audio gives pending PCM precedence
over indexed PSG (`docs/subsystems/audio.md`).

## Random-sample VBlank

`JamOutRandomSampleVBlank` uses the same music service and counter ownership,
but polls both controllers every callback and routes both words in one call.
It preserves the solo-Earl audio-route swap/temporary counter adjustment.

For each route, A/B/C request the corresponding selected entry of
`PCMPlaybackPointers`. A direction bit rerolls all three route indices modulo 42.
The sample requests occur before the reroll in the source.
Route-one Start cycles seven song selections and then a no-song reset state.
Route-two Start cycles three template modes: release/reset, ordinary bank,
and alternate bank; changing bank forces a loaded/requested mismatch.
The counter/index fields are shared state, not carried in a local menu object.

## Lead animation state machine

`StartJamOut` maps pending animation input independently of the audio callbacks:

- A picks one of two bass-drum sequences at period six using RNG.
- B picks one of two snare sequences at period six using RNG.
- C selects clap at period three and locks the lead's animation.
- Direction or Start selects vocal/scratch at period two and locks it.

`JamOutSetToejamSequence` and Earl equivalent ignore changes while locked;
otherwise they install the sequence, reset frame duration/index, set period,
and update the cached sequence pointer.
`JamOutUpdateLeadAnimations` switches a completed animation between idle
transition and idle loop, resets its timer/index/period, and caches the frame.
It clears lock state once the cached sequence is one of those idle sequences.
The function contains attempted partner-frame assignments before resetting
`frame=-1`; the observed final reset is the stronger effect, not guaranteed
phase synchronization.

## Shared graphics and copy objects

`JamOutCreateCharacterCopies` attempts nine dynamic entities per character.
ToeJam reserves 16 tiles/one sprite piece; Earl reserves 19 tiles/two pieces.
Each starts offscreen at `(160,-50)` with the idle-loop sequence, period six,
frame duration one, frame index minus one, and priority tile attributes.
Slot zero, if successfully allocated, supplies lead tile and parent identity;
nonzero slots set `ENTITY_SHARED_TILES` and reuse that state. A later first
successful allocation is not a fallback initializer.
Only viewport zero is explicitly allocated by this helper.

`JamOutSynchronizeEntityCopies` walks 40 records. For active shared-tile objects,
it copies the parent's frame, readiness, current frame pointer, and each hardware
piece's size/tile fields. Position remains each copy's own layout position.
The caller of this helper is not represented in the still-absent high-level
`UpdateAndQueueEntitySprites` body; the global shared-copy flag is evidence of the intended integration,
not a reconstructed hidden call sequence.
`JamOutReleaseCharacterCopies` frees nonzero handles but does not explicitly
zero the handle arrays in the shown body.

## Layout/scroll state machine

`JamOutUpdateLayoutsAndScroll(1)` selects phase zero/countdown 170, zeroes scroll
and step, writes scroll registers, and displays one lead pair.
Ordinary updates subtract step from horizontal scroll and add it to vertical
scroll, or reset both when step zero; they publish those scroll values directly.
When countdown expires, the phase advances modulo six and clears animation locks:

| Phase | Layout/change | Next countdown |
| --- | --- | --- |
| 0 | Lead pair; stop scroll. | 170 |
| 1 | Four pairs; stop scroll. | 170 |
| 2 | Release exit text; nine pairs. | 68 |
| 3 | Diagonal scroll step +1. | 68 |
| 4 | Diagonal scroll step -1. | 68 |
| 5 | Stop scroll. | 68 |

`JamOutShowLeadPairLayout` hides copies one through eight, places the two leads,
and draws a 16-by-22 packed background map.
`JamOutShowFourPairLayout` hides copies four through eight, places four pairs,
and draws four 14-by-20 background blocks.
`JamOutShowNinePairLayout` places all pairs in a three-by-three grid and resets
the upper/title-transition map arrangement.
Packed map bits `0x40/0x80` become VDP horizontal/vertical flip attributes.
Hide helpers move copies offscreen rather than freeing their resources.

## Shared frame timing and interactions

`CutsceneStepFrameAndMusicCadence` animates the transition background, queues
sprites, waits a VBlank, loads them, selects viewport zero, and increments its
scene update counter. Music events are cleared and debounced by ten counter ticks;
accepted events can advance the 24-entry background-pair cycle at the requested period.
Jam Out uses this mechanism rather than a separate rhythm clock.

- Main menu supplies selection and controls solo/both player indicator.
- Audio provides PCM samples, FM templates, alternate song banks, and event flag.
- Sprites provides 18 object records and shared-tile copy rendering.
- Cutscene text supplies the instruction line and its release.
- Camera-follow/world simulation is not called by the Jam Out mode loop.

## ORIGINAL BUG list

No `ORIGINAL BUG:` comments occur in the represented Jam Out source.
The compiler-coaxing unused assignment in `JamOutReleaseCharacterCopies` is an
explicit matching qualification, not an original-bug label or design requirement.

## Open questions

- Allocation failure of a lead before later copies is not covered by a marked
  bug or a documented resource guarantee in the shown constructor.
- The missing `UpdateAndQueueEntitySprites` body leaves the shared-copy helper's
  callsite and full animation-advance ordering open.
- Some cycle/input words are not explicitly reset in `StartJamOut`; persistent
  cross-entry behavior should be derived from full callers before changing it.

## Port notes

- Platform layer: replace `JamOutVBlank` and `JamOutRandomSampleVBlank` display
  callback/input routing with a mode-owned update/render/audio-input pipeline.
- Shared-screen coupling: explicitly assign `gJamAudioRoute1PendingInput` and
  route-two ownership, local exit, and shared-session exit for online players.
- Determinism: timestamp input before those callbacks, order simultaneous
  requests, and seed random-mode sample/pose choices independently of rendering.
- Content tables: externalize the 42-sample pool, route mappings, sequences,
  phase durations, layouts, and song/template-bank selections.
- Camera/visibility: widescreen layouts should retain authored composition or
  extend the grid deliberately; offscreen hiding must not require hardware slots.
