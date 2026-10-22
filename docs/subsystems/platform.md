# Platform: the hand-written 68000 replacement surface

## Purpose and evidence

This document identifies the low-level services a native Windows/Mac/web port
must replace rather than transliterate as game logic: reset/interrupt control,
VDP memory operations and DMA, controllers, chip audio coordination, sprite
unpacking, and compiler arithmetic helpers.

Evidence is `src/asm/*.s`, `src/z80/*.s`, `include/hardware.inc`,
`include/sound.inc`, and hardware/call headers at repository-root paths.
These readable assembly implementations do not earn C decompilation credit
(`AGENTS.md`, `docs/ACCEPTANCE.md`). No assembly listing or ROM bytes appear here.
Orchestration is in `docs/subsystems/core.md` and `docs/subsystems/audio.md`.

## Files and responsibilities

- `src/asm/startup.s`: `_start`, reset setup stream, TMSS, VDP/Z80/RAM/PSG setup.
- `interrupts.s`: `GameProgram`, SR masks, VBlank/HBlank wrappers, software reset.
- `vdp.s`: clears, palette upload, port fills, word copies, rectangular upload.
- `dma.s`: `VDPWritesRoutine1`, 68000 memory-to-VRAM DMA programming.
- `controllers.s`: `GetCtrl1Input` and `GetCtrl2Input`, three-button TH multiplexing.
- `rle.s`: raw, contiguous RLE, and four-lane interleaved sprite unpackers.
- `runtime.s`: stock Microtec long multiply/divide/remainder and absolute value.
- `z80bus.s`: sound-driver upload, bounded bus polling, reset pulse.
- `sound.s`: FM register writes and 68000 sequencer.
- `sound_init.s`, `sound_clear.s`, `sound_channels.s`, `sound_schedule.s`:
  driver state/template/channel services.
- `sound_pcm.s`, `sound_psg.s`: mailbox handshakes and retry fragments.
- `src/z80/initial_bootstrap.s`, `sound_driver.s`: other-CPU programs, not 68000 C.
- `include/hardware.inc`: equates only; it emits no storage.
- `include/sound.inc`: channel/patch layout and timing constants, also no storage.

## Hardware addresses and shared RAM

The 68000 interface includes VDP data/control at `0xc00000/0xc00004`,
controller data/direction registers, Z80 bus/reset control, mapped Z80 RAM,
and mapped YM2612 ports (`hardware.inc`, `hardware.h`).
Byte, word, and long accesses are distinct interfaces even when addresses match.
For example `VDP_CONTROL_WORD` and `VDP_COMMAND` share the control port but have
different transfer widths; an ordinary integer assignment is not an abstraction.

`gMode2RegSoftcopy` is the VDP mode-register shadow consumed by DMA and C display
control. `USED_BY_VDP_1/2` are the two views used to publish a packed DMA command.
`gSpriteTileUploadBuffer` and `gSpriteTileBufferOffset` are shared decoder output
state, not allocations owned by each decompression call.
Z80 mailbox state includes command, lock, argument, PCM address/count, DAC pacing,
and PSG-active bytes; see `docs/subsystems/audio.md` for the protocol.

## Reset and interrupt replacement

`_start` optionally performs full machine setup according to existing I/O
control state, consumes `SetupValues`, and enters `GameProgram`.
`GameProgram` masks interrupts, clears 64 KiB work RAM, and enters C video/game
initialization. The reset path and its data stream are hardware bootstrap, not
portable world-generation rules (`startup.s`, `interrupts.s`).

`DisableAllInterrupts` sets SR priority seven and `EnableAllInterrupts` clears
priority bits; these are not equivalent to acquiring an application mutex.
`Level6Isr` saves all registers around `HandleVBlank`; `Level4Isr` preserves the
C-call volatile set around `HandleHBlank` and both return with exception return.
`RestartFromResetEntry` reloads the initial cartridge stack pointer and jumps to
reset, rather than resetting selected application objects.
[INFERENCE] A native port should express restart through owned application state,
not imitate stack/register manipulation.

## VDP clear, fill, and transfer primitives

`ClearVideoRAM` writes zero to all 32768 VRAM words using the existing increment.
`ClearColourRAM` clears 64 CRAM color words.
Despite its historical name, `ClearSRAM` clears 40 VSRAM scrolling words; it does
not erase cartridge save RAM (`vdp.s` explicit comment and command selection).

`MoveDataToVRAM(source,count)` copies words to the current VDP destination.
`FillVdpDataWords(value,count)` writes the low word repeatedly.
`FillVdpDataBytes(value,count)` performs byte-sized port writes.
Each uses predecrement plus an inclusive DBRA loop; these are 16-bit low-counter
semantics, not unrestricted host `size_t` operations.
[INFERENCE] Passing zero to those predecremented loops is not an ordinary empty
copy; content callers must not be assumed to support arbitrary zero-size inputs.

`CopyLongwordRunToVramTile(source,tile,count)` packs the tile destination and
copies `(count*8)+1` longwords, explicitly retaining an inclusive count.
Its extra longword is documented behavior, not silently normalized here.
`UploadVramWordRectangle` accepts destination byte address, source, words per
row, row count, and destination row stride in words. It issues one address
command per row and copies contiguous source rows.
`CopyPaletteToCRAMSlot` masks the slot with three and uploads 16 color words.

These APIs depend on the current VDP destination, auto-increment, palette/tile
encoding, and display state. Their apparent source/count signatures omit that
implicit state, so merely replacing the final stores is insufficient.

## DMA flow

`VDPWritesRoutine1(source,vram_byte_address,word_count)`:

1. Reads the mode-two shadow and enables DMA in the programmed register value.
2. Writes low/high 16-bit DMA word-count registers.
3. Converts the source byte address to the VDP's word-addressed source fields.
4. Packs low/high VRAM address pieces and the DMA-start command.
5. Stores the packed command in shared RAM and writes its two word views with
   the original NOP separation.

It preserves D2-D4 and returns without exposing a portable asynchronous-transfer
handle (`dma.s`). Existing callers impose timing through VBlank/display flags.
A native renderer needs explicit resource lifetimes/ownership rather than
assuming the target device reads the source at an interchangeable instant.

## Controller sampling and sound coupling

`GetCtrl1Input` and `GetCtrl2Input` take Z80 bus request, configure TH output,
read TH-low Start/A bits, then TH-high directions/B/C, and invert active-low data.
Their delay uses the original small DBRA loops (`controllers.s`).
The resulting logical byte uses directions `0x01/02/04/08`, B `0x10`,
C `0x20`, A `0x40`, Start `0x80`; core can exchange A/C or physical ports.

Controller sampling also subtracts two from the Z80 DAC pacing byte and copies
PSG-active status into the existing music-status storage before releasing bus.
This is cross-device timing compensation, not a property of a gamepad.
UI's `AreBothControllersConnected` separately performs a direction/data-register
presence probe while holding bus request (`src/ui/main_menu.c`).
[INFERENCE] Keyboard, modern controller, and online-input support should replace
both hardware protocols while keeping logical-player button mapping separate.

## Sprite unpacking formats

All three unpackers read `render_piece.data` at record offset six and write at
`gSpriteTileUploadBuffer + gSpriteTileBufferOffset` (`rle.s`).
They do not themselves advance the shared upload offset.
`SpriteUnpackRaw` copies eight longwords, 32 bytes, per tile; the historical
inner label `Copy8Bytes` does not describe the actual amount.

`SpriteUnpackRLE` expands a contiguous target of tile-count times 32 bytes:

- `0xc0..0xff`: repeat the following byte by the negated signed token count.
- `0x80..0xbf`: zero run, converting token bits to the same negative count form.
- `0x40..0x7f`: fill `0xff` for the low-six-bit count when at least one.
- `0x00..0x3f`: copy literal bytes for the token count when at least one.

`SpriteUnpackInterleavedRLE` decodes four lanes, each eight bytes per tile,
writing a four-byte output stride. Its token families differ for positive fills:

- `0xc0..0xff` and `0x80..0xbf`: repeat-byte and zero runs as above.
- `0x40..0x5f`: `0xff` fill for the low-five-bit count when at least one.
- `0x60..0x7f`: for counts at least two, alternate the following byte and its
  nibble-swapped form, with an odd trailing original byte when needed.
- `0x00..0x3f`: literals at four-byte stride for token counts at least one.

The routines do not validate these minimums. Literal/fill count zero underflows
the inclusive DBRA counter and executes 65536 iterations. Alternating count zero
executes 65536 pairs; count one executes 65536 pairs plus a trailing original
byte. Whether actual assets reach these encodings is not established. These are
codec safety qualifications, not additional marked original-bug classifications.

The old `TokenBetween40And5F`/`60And7F` labels are reversed relative to the
actual bit-test branches; the assembly comments explicitly explain this.
Decoded quotas are reduced by run lengths; no per-source extent or destination
capacity parameter is supplied. [INFERENCE] An external-asset loader needs a
validated codec boundary rather than trusting arbitrary modded data as ROM data.

## Arithmetic runtime

`runtime.s` identifies stock Microtec MCC68K 3.3K modules, not game algorithms.
`abs` accepts a stack long and negates negative values.
Other entries take D0/D1, return D0, preserve D2, and use A0/A1 as scratch.
`lmult` retains the low 32 bits from halfword products/cross-products.
`ldivt` divides signed magnitudes and restores quotient sign.
`lmodt` restores the dividend's sign to the remainder.
`uldivt` and `ulmodt` select quotient/remainder from one unsigned body.
Word-sized divisors use native divide paths; wider divisors use restoring steps.

[INFERENCE] Host-language arithmetic must preserve target-width wrap, signed
remainder, narrowing, and exceptional edge behavior where gameplay depends on
it; substituting 64-bit `long` on a modern ABI is not equivalent.
The helpers should disappear behind explicit-width native arithmetic, not remain
as register-convention APIs in game code.

## Audio replacement boundary

Z80 upload, reset/bus access, FM/PSG writes, and cycle-counted PCM are hardware
services. The music stream interpreter and request priorities are game/content
behavior worth preserving independently (`docs/subsystems/audio.md`).
FM code and controller reads adjust DAC pacing; audio mixing should not inherit
that incidental coupling to input polling or a CPU's instruction timings.
The boot Z80 image, later idle image, and final sound image are three distinct
stages; there is no reason to emulate all three to implement the native soundtrack.

## ORIGINAL BUG list

No `ORIGINAL BUG:` comments occur in the inspected canonical 68000 assembly.
The paired Z80 driver's marked defects are owned solely by
`docs/subsystems/audio.md`; consult its ORIGINAL BUG list.
Inclusive counts, misleading legacy names, and unchecked ROM-format assumptions
above are observed semantics, not newly asserted original-bug classifications.

## Open questions

- The intended caller domain for zero/overflowing transfer counts is not specified.
- Exact hardware-cycle audio behavior is not a portable sample-rate declaration.
- Some buffer extents are inferred available spans, not checked allocator bounds.
- A port must choose compatibility behavior for target arithmetic exceptions and
  undefined original C cases rather than claiming host defaults are identical.

## Port notes

- Platform layer: replace `VDPWritesRoutine1` MMIO/DMA and
  `gMode2RegSoftcopy` hardware shadowing with an explicit renderer interface.
  Replace audio/reset, decoder, and timing operations at the same boundary.
- Shared-screen coupling: make per-player UI/view passes independent of HBlank
  and live sprite-chain patching rather than using a global graphics pause.
- Determinism: replace `GetCtrl1Input` bus/delay compensation with logical input;
  simulation must not depend on VBlank, bus grants, audio callbacks, or polling rate.
- Content tables: decode/export resources through typed validated formats;
  preserve `SpriteUnpackRLE` format semantics while validating run counts/extents.
- Camera/visibility: replace wrapped 64-by-32 tilemaps with explicit world/view
  sampling while preserving any gameplay visibility policy separately from GPU culling.
