# Audio: 68000 sequencer, FM synthesis, PSG effects, and PCM

## Purpose and evidence

The 68000 runs music sequencing, FM patch/frequency writes, and request arbitration.
The Z80 runs interruptible PSG routines and one unsigned-PCM stream using the
YM2612 channel-six DAC (`src/z80/sound_driver.s` introduction and bodies).
This is not a conventional Z80 music sequencer with a passive 68000 interface.

Evidence is the C request files/headers, `src/asm/sound*.s`, `z80bus.s`,
`include/sound.inc`, `include/hardware.inc`, and both Z80 sources.
All source paths refer to the current repository-root layout.
`PlaySound` and `PlaySoundEffect` have matched C bodies in `src/audio/`; they
apply shared priority/cooldown policy before publishing requests.

## Files and roles

- `src/audio/play_sound.c`: indexed PSG request admission and shared priority.
- `play_sound_effect.c`: direct PCM request admission and optional FM/sequencer stop.
- `request_channel_release.c`: queues six-channel FM release.
- `request_track_reset.c`: queues clearing six music owner/stream pointers.
- `request_template_effect.c`: queues an auxiliary FM template effect number.
- `src/asm/sound.s`: auxiliary templates, YM patch/frequency/key-mask writes,
  and `MusicUpdateSequencer`, the per-tick music bytecode interpreter.
- `sound_schedule.s`: song banks, six-channel templates, restart callback.
- `sound_init.s`: copies the 276-byte initial music RAM image.
- `sound_channels.s`: releases all six stored FM key masks.
- `sound_clear.s`: clears music owner/stream pointers.
- `sound_psg.s`: mailbox command-one submission and longer retry fragment.
- `sound_pcm.s`: direct and sequencer PCM submissions, shorter retry fragment.
- `z80bus.s`: uploads the Z80 program and performs bus/reset handshake.
- `src/z80/sound_driver.s`: mailbox, PSG dispatch/routines, PCM/bank/pacing loop.
- `src/z80/initial_bootstrap.s`: reset bootstrap, distinct from the sound driver.
- `include/sound.inc`: 38-byte channel/patch offsets and timing constants.
- `include/{audio_calls,ram_sound,sound_types,assets_audio}.h`: C interfaces.
- `include/{psg_effects,fm_effects,music_ids}.h`: separate request-number domains.

## Channel records and pending state

A music-channel record is `0x26` (38) bytes. The C reset view names the saved
stream bytes, current stream pointer, and remaining callback/channel byte block.
Assembly `sound.inc` supplies the finer interpreted offsets:

- Active stream at `+4`, callback at `+8`, hardware FM channel at `+0x0c`.
- Signed priority `+0x0d`, patch `+0x0e`, wait byte `+0x0f`.
- Frequency bytes `+0x10/+0x11`, note-active `+0x12`, selected-owner `+0x13`.
- Loop depth and indexed count storage beginning at `+0x14`.

`gMusicFmChannelOwners` is the six-entry hardware-channel owner table.
Six primary channel records start at `gMusicChannels`; additional auxiliary
records begin at `gFmEffectChannels` (`AudioLoadPendingChannelTemplateEffect`).
`gMusicChannelScanCountMinusOne` controls the inclusive record-walk count in `MusicUpdateSequencer`;
it is not safe to assume the interpreter always visits exactly six records.
FM patch records also occupy 38 bytes; their final byte stores the operator mask.

The C wrappers write requests, not hardware. `PlaySound` accepts an indexed request
when both `gPcmEffectCooldownTicks` and `gPsgEffectStatus` are zero, or when its
priority is at least `gSoundEffectPriority`; it records the pending PSG ID and
new priority. `PlaySoundEffect` uses the same gate for a pending PCM pointer.
A nonzero stop-channels argument also sets `gPcmBlocksSequencer` and requests
FM release; a zero argument does not clear an already set blocking latch.
Later accepted requests can replace the shared pending value before service.

The other wrappers publish the following requests:

- `AudioRequestFmChannelRelease`: `gFmChannelReleaseRequested = 1`.
- `MusicRequestTrackPointerReset`: `gMusicTrackResetRequested = 1`.
- `AudioRequestChannelTemplateEffect(n)`: `gPendingFmEffect = n`.

`gPendingPsgEffect` is a pending indexed PSG request, with negative none sentinel.
`gPendingPcmSample` is a pending direct PCM pointer.
`gPcmEffectCooldownTicks` and `gPcmBlocksSequencer` control speech blocking.
`gPsgEffectStatus` is the 15-tick PSG status countdown.
`gMusicAlternateTemplateBankEnabled`, `gBoomBoxMusicTemplateHold`, and
`gMusicTemplateRestartState` affect template restart scheduling.
`gMusicAnimationEvent` and `gMusicAnimationChannel` provide a music event flag/channel selector used by Jam Out.

## Boot and driver loading

The reset-time 38-byte `InitialiseTheMegadrive_Z80SetupValues` clears Z80 RAM,
initializes registers, disables interrupts, and idles at address zero.
It differs from the later six-byte idle program written by C `InitialiseTheZ80`.
`LoadSoundDriverToZ80` requests the bus with a bounded poll, copies the driver's
inclusive `SoundDriverProgramSize` range, pulses reset, releases the bus, and
sets a two-tick start countdown (`z80bus.s`).
`MusicRestoreInitialState` restores 276 bytes beginning at `gMusicFmChannelOwners` from the
initial-state resource; it is not a generic work-RAM clearing function.

The Z80 driver sets IM1 but keeps interrupts disabled; its IRQ entry returns.
`DriverInitialize` clears command/busy/background-noise state, silences PSG,
performs a short DAC startup ramp, centers it, then disables DAC.
Normal execution polls the command byte rather than waiting on an IRQ.

## Main audio update flow

Core normally calls `MusicUpdateSequencer` from VBlank; an outstanding music-bank
change instead runs `MusicSelectSongTemplateBank` for that service slot.
Cutscenes and Jam Out can supply different VBlank callbacks.
`MusicUpdateSequencer` services requests in this order:

1. Release all FM channels when requested.
2. Clear six owner/stream pointers when requested.
3. Load a nonnegative auxiliary-template request.
4. Submit direct PCM if pending, otherwise submit nonnegative PSG request.
5. Decrement PSG status and PCM/speech cooldown timers.
6. If speech-blocking remains active, return before channel processing.
7. Resolve hardware FM channel ownership across live stream records.
8. Interpret each live stream and invoke its installed per-tick callback.

Ownership compares signed priority bytes. A later candidate with equal or
higher priority replaces the previous owner; nonowners still have stream state
but do not issue the guarded FM hardware writes.
`AudioReleaseAllFmChannels` visits hardware channels five down to zero and clears
its request flag. `MusicClearTrackPointers` clears six owners and corresponding
active stream pointers, then clears its request flag.
Auxiliary template loading releases active auxiliary channels, zeros three
records, copies requested table/count records, and consumes the request.
Template effect zero means clear, not an ordinary effect stream.

## Music bytecode

`MusicUpdateSequencer` interprets multiple commands in one tick until a wait/stop path.
A negative command byte sets a wait count from its low seven bits; the tick
path immediately decrements the wait and invokes any installed callback.
Nonnegative commands are:

| Opcode | Observed operation |
| --- | --- |
| 0 | Stop stream, release owned hardware channel, clear callback/note/selection. |
| 1 | Read two frequency bytes, mark note active, load patch/key owned channel. |
| 2 | Emit stored key mask for an owned channel; clear note-active. |
| 3 | Update stored frequency and, when selected, hardware frequency. |
| 4 | Translate patch ID through the song's sixteen-entry instrument map. |
| 5 | Push a two-byte loop count into indexed channel loop storage. |
| 6 | Decrement indexed loop count and take the stored relative branch if positive. |
| 7 | Align stream and copy four bytes of callback address. |
| 8 | Signed sixteen-bit relative stream branch. |
| 9 | Try record-scratch PCM. |
| 10 | Try clap PCM. |
| 11 | Try bass-drum PCM and set the music event flag. |
| 12 | Try snare-drum PCM. |

The loop-storage indexing and branch-base arithmetic are historical format
rules, not interchangeable with a generic scripting VM (`sound.s`, VM handlers).
The code has no new validation layer for malformed streams or callback pointers.

`MusicLoadSongChannelTemplates` releases FM, restores initial RAM, and copies
one 228-byte six-channel template selected from `SongChannelTemplates`.
`MusicSelectSongTemplateBank` sets countdown two and adds seven to the selection
when the alternate bank is enabled. `MusicProcessTemplateRestartHook` clears
its callback, decrements the countdown, handles zero/`-2` sentinel transitions,
and defers one tick while the boombox hold blocks the next template.

## YM2612 hardware surface

`SetupYM2612` selects either three-channel YM port bank, reads an FM patch,
waits on busy status, uploads operator/envelope/algorithm/pan state, writes
frequencies, and emits the operator key mask. Channel-three mode has additional
frequency-register handling. Nine-NOP gaps retain original register timing.
`YM2612UpdateFrequency` only updates frequency for a valid channel.
`AudioWriteFmChannelReleaseMask` combines the stored operator mask and channel
code; release is the canonical API name, not proof every emitted mask is zero.

These routines request the Z80 bus, adjust the DAC pacing byte, restore the
YM DAC-data address latch, and release the bus. FM and Z80 PCM therefore share
hardware latch/timing ownership, not independent sound devices.

## Mailbox protocol and request failure behavior

Z80 RAM offsets `0x40..0x49` hold command, snapshot lock, argument, little-endian
24-bit PCM payload address, and little-endian 16-bit byte count.
The 68000 consumes the sample's big-endian length header before posting it.
Command is published last; one means PSG, two means PCM.
The lock covers copying PCM arguments, not the entire playback duration.

Senders request the bus with a bounded poll. A snapshot lock causes bus release,
a short delay, then retry; keeping the bus stopped would prevent lock completion.
`AudioSubmitPendingIndexedCommand` consumes the PSG request even on bus timeout;
accepted submission starts the 15-tick status timer.
`AudioSubmitPendingPcmSample` retains its PCM pointer on timeout, cancels pending
PSG, and on acceptance sets cooldown to the integer quotient of length by 138,
plus five, then clears the pointer.
`AudioTrySubmitSequencerPcmSample` rejects during speech cooldown, alternate-bank
inhibit, or nonzero PSG-active state; it does not start speech cooldown.

## Z80 PCM and PSG state machines

`StartPCM` locks/snapshots arguments, clears command/lock, selects the 68000 ROM
bank, enables DAC, and outputs unsigned bytes. Count is tested after the first
sample: zero means 65536 bytes, not empty playback.
`PCMWaitSample` polls for a new command and paces samples using the signed delay
byte. Any new command preempts PCM; `StopPCM` centers and disables DAC.
When the source pointer wraps, `AdvanceROMBank` advances the saved 24-bit address
by 32 KiB and programs A15..A23 into the serial bank latch.

`DispatchPSGEffect` clears command, silences PSG, sets active, validates ID `<30`,
and dispatches through big-endian routine pointers. IDs 0 and 27..29 silence;
entries 30..33 exist but are rejected by the limit check.
Effects are authored code routines: tone pulses/sweeps/phrases, dual-tone fade,
noise sweeps/fades/crescendos, and periodic noise (`PSGEffectTable`).
`WaitForCommand` polls during delay units; a new command resets the Z80 stack,
silences PSG, and returns to dispatch, abandoning nested effect returns.
Effect 5 enables persistent tone-two-clocked white noise; effect 6 clears it.
`PSGEffectFinished` restores that background state after subsequent PSG effects.
Some routines silence midway, so zero PSG-active does not prove final completion.

## Interactions

- UI pause requests release/clear/reset and invalidates music selection on exit.
- Cutscene scripts request exact music selections and sampled sounds.
- Jam Out uses the music event flag, direct PCM, and auxiliary FM effects.
- Gameplay has three separate ID domains: PSG indexed effects, FM templates,
  and direct PCM resource pointers. They must not be merged by numeric coincidence.
- Speech bubbles and PCM cooldown share terminology but have separate lifetimes.

## ORIGINAL BUG list

- `DispatchPSGEffect` (`src/z80/sound_driver.s`): an out-of-range ID returns to
  polling without clearing the PSG-active value set before validation.

## Open questions

- Exact audible sample rate depends on hardware-cycle pacing and bank/FM steals;
  the code does not declare a portable Hz rate.
- Stream callbacks are raw pointers; an external format needs explicit event IDs.

## Port notes

- Platform layer: replace `AudioSubmitPendingPcmSample` bus/mailbox submission,
  YM/PSG MMIO, bank latch, and cycle-counted DAC with a native audio backend.
- Shared-screen coupling: local menus should not globally issue
  `AudioRequestFmChannelRelease` for the partner unless shared audio policy requires it.
- Determinism: schedule `MusicUpdateSequencer` and `gPcmEffectCooldownTicks` from an
  explicit tick clock; timestamp sound events and mix on independent audio time.
- Content tables: export songs, instrument maps/patches, template counts, PCM
  headers/payloads, and PSG envelopes/routines without shipping ROM byte blobs here.
- Camera/visibility: keep audience/panning policy explicit for remote players;
  widening the view should not accidentally change authoritative effect admission.
