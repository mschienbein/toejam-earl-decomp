# Data

Every ROM byte outside the catalogued code is a classified, named data unit in
`config/<revision>/data_units.json`; classification does not imply a recovered reader or meaning.
The build extracts each unit's asset from the user's ROM
(`tools/assets.py extract`), packs every unit back from its asset alone (`pack`, which never reads
the ROM) and links the packed units; the image must still be byte-identical. Assets and packed
units live under `build/` and are never committed. The config holds only names, kinds, addresses,
sizes, layout parameters and provenance.

Status (2026-10-22): **REV 02: 4,163 units, 867,776 bytes; 867,770 typed, 6 unreferenced, 0 raw.
REV 00: the same 4,163 units by name, 867,888 bytes; 867,882 typed, 6 unreferenced, 0 raw.**
Neither revision has anonymous bytes;
both build every data byte from assets. Between the revisions, 2,519 units are identical, 1,641
differ only by relocated pointers (6,019 pointer fields, all translated), the cartridge header and
SleepZAnimation differ in content, and the final fill is 112 bytes longer in REV 00.

## Formats

| Kind | Units | Bytes | Asset |
|---|---:|---:|---|
| `tiles` | 2,208 | 438,564 | indexed 4bpp PNG (16-colour palette from the unit's palette group); 509 simple-RLE, 1,525 four-lane RLE, 174 uncompressed |
| `metasprite` | 1,287 | 32,598 | JSON: prefix plus inline ten-byte sprite records |
| `pcm` | 41 | 291,136 | WAV, 8-bit unsigned; the stored count and payload length are kept separately (five samples differ by design) |
| `pointer_table` | 320 | 7,108 | JSON of symbolic big-endian pointers |
| `values`, `records` | 63 + 31 | 46,362 + 15,705 | JSON typed arrays and named-field records (maps, metatiles, lookup tables, FM patches, channel templates); one reviewed, unindexed trailing element is separately exposed |
| `music` | 3 | 25,383 | JSON sound-VM bytecode (waits, notes, instruments, loops, jumps, callbacks) |
| `animation` | 83 | 2,381 | JSON frame programs (frame index, flip bit, restart, hold); two unreferenced two-command programs have `referenced: false` |
| `text` | 50 | 4,580 | JSON text with exact Latin-1 bytes and padding |
| `palette` | 49 | 1,568 | JSON 3-bit channels, unused CRAM bits preserved |
| `vectors`, `cartridge_header` | 1 + 1 | 256 + 256 | JSON symbolic vector table; fixed-width header fields (checksum not recomputed) |
| `z80_binary` | 2 | 1,332 | the Z80 sound-driver upload image and its boot stub; built from Z80 source (`src/z80/`, ruling R007), the extracted image is only the reference |
| `sprite_header`, `padding` | 10 + 13 | 100 + 441 | JSON; recovered singleton alignment units specify checked fill and alignment |
| `unreferenced` | 1 | 6 | Bounded, reviewed unknown storage, JSON `unreferenced-bytes-v1`; excluded from typed-byte credit |
| `raw` | 0 | 0 | no raw-kind units |

Bytes per kind are from the build receipt; formats restated from bstreiff/tje-disasm (ruling R003)
and checked against the code that reads them.

Four former raw extents are singleton FF padding before word-accessed units. The level-choice
table stores 521 elements, but its matched reader uses only 26 by 20; the extra element is kept in
`trailing_values`, not silently made playable. Its table ownership is [INFERENCE]. Both sections
use the owning table's width, signedness and big-endian values codec: there is no raw-byte suffix.
`trailing_elements` has a hard cap of two, requires structured ownership evidence, and is currently
allowed only for this reviewed one-element layout in the two known revisions.

Two trailing pairs decode as frame zero followed by restart with the existing interpreter; no
reader has been found, so their animation-storage role is [INFERENCE] and `referenced: false` is
explicit. The six-byte FM-adjacent suffix has neither an established reader nor a known format.
It is `unreferenced`, not semantic recovery. That kind has a hard eight-byte limit and is restricted
to the reviewed `FMPatchAuxiliary` identity, per-revision address and six-byte extent. Load-time,
extract and pack validation require structured scan evidence, a checked observed pattern, and the
two contiguous, reviewed typed neighbours. Other layouts need a new reviewed allowance in the tool;
arbitrary small units, large payloads and truthy/nontext evidence do not qualify.

Build receipts and progress accounting count `unreferenced` separately from typed and raw bytes;
ByteMatch's typed-data numerator excludes it. `data_inventory.json` retains the earlier discovery
history; `data_units.json` and receipts are authoritative for current ownership, kinds and evidence.

## Compression

The RLE graphics are rebuilt from pixels. A deterministic encoder that sees only the pixels and the
compression mode reproduces 1,947 of the 2,034 compressed streams exactly; the other 87 keep a
compact operation/count plan (one byte per run, no pixel values) inside their private PNG. Plans
total 2.7% of the original compressed bytes. The original encoder's exact choice rule is not
recovered; this rule is inferred from the streams.

## Open

- Six `unreferenced` bytes at REV 02 `0x03C34A` / REV 00 `0x03C2DA`: outside the 49 complete
  38-byte FM patches. No FM-field interpretation or runtime consumer is established; the reviewed
  `unreferenced` type closes them (ruling [R012](rulings/R012-closure-interpretations.md)).
- Forty-three text units (2,330 bytes per revision) are emitted in the build by the matched C
  functions' own string literals (MCC68K places a function's strings right after it); their
  extracted assets serve as extraction checks. Source closes those bytes (R012).
- Runtime use of the two animation-compatible tails remains unestablished; structural typing
  is not proof of reachability. Reference scans exclude direct roots, not arbitrary computed use.
- PCM playback rate: 8000 Hz is only an audition default; the sample header has no rate.

Credits: DataSmith lanes (design, formats, codecs; REV 00 derivation), DataReview (independent
reviews of both revisions), RawResolver (tail classification and bounded unreferenced accounting).
