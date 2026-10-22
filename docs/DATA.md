# Data

Every ROM byte outside the catalogued code is a typed, named data unit in
`config/rev02/data_units.json`. The build extracts each unit's asset from the user's ROM
(`tools/assets.py extract`), packs every unit back from its asset alone (`pack`, which never reads
the ROM) and links the packed units; the image must still be byte-identical. Assets and packed
units live under `build/` and are never committed. The config holds only names, kinds, addresses,
sizes, layout parameters and provenance.

Status (REV 02, 2026-10-22): **4,163 units, 867,776 bytes; 867,761 typed, 15 raw, none anonymous.**
REV 00 still links slices of its original until its own units are validated (most later resources
sit 112 bytes earlier and are identical).

## Formats

| Kind | Units | Bytes | Asset |
|---|---:|---:|---|
| `tiles` | 2,208 | 438,564 | indexed 4bpp PNG (16-colour palette from the unit's palette group); 509 simple-RLE, 1,525 four-lane RLE, 174 uncompressed |
| `metasprite` | 1,287 | 32,598 | JSON: prefix plus inline ten-byte sprite records |
| `pcm` | 41 | 291,136 | WAV, 8-bit unsigned; the stored count and payload length are kept separately (five samples differ by design) |
| `pointer_table` | 320 | 7,108 | JSON of symbolic big-endian pointers |
| `values`, `records` | 63 + 31 | 46,372 + 15,705 | JSON typed arrays and named-field records (maps, metatiles, lookup tables, FM patches, channel templates) |
| `music` | 3 | 25,383 | JSON sound-VM bytecode (waits, notes, instruments, loops, jumps, callbacks) |
| `animation` | 81 | 2,377 | JSON frame programs (frame index, flip bit, restart, hold) |
| `text` | 50 | 4,580 | JSON text with exact Latin-1 bytes and padding |
| `palette` | 49 | 1,568 | JSON 3-bit channels, unused CRAM bits preserved |
| `vectors`, `cartridge_header` | 1 + 1 | 256 + 256 | JSON symbolic vector table; fixed-width header fields (checksum not recomputed) |
| `z80_binary` | 2 | 1,332 | the Z80 sound-driver upload image and its boot stub, binary assets |
| `sprite_header`, `padding` | 10 + 9 | 100 + 437 | JSON |
| `raw` | 7 | 15 | named binary assets, each with a recorded reason |

Bytes per kind are from the build receipt; formats restated from bstreiff/tje-disasm (ruling R003)
and checked against the code that reads them.

## Compression

The RLE graphics are rebuilt from pixels. A deterministic encoder that sees only the pixels and the
compression mode reproduces 1,947 of the 2,034 compressed streams exactly; the other 87 keep a
compact operation/count plan (one byte per run, no pixel values) inside their private PNG. Plans
total 2.7% of the original compressed bytes. The original encoder's exact choice rule is not
recovered; this rule is inferred from the streams.

## Open

- 15 raw bytes: two-byte tails at REV 02 `0x0F093A` and `0x0FFE56` (animation program or padding),
  and five auxiliary units without an established reader.
- PCM playback rate: 8000 Hz is only an audition default; the sample header has no rate.
- The Z80 driver stays a binary asset; turning it into Z80 assembly source is a separate decision.
- REV 00 units.

Credits: DataSmith lane (design, formats, codecs), DataReview (independent review, two rounds).
