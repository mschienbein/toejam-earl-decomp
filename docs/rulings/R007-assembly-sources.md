# R007 — Hand-written assembly and the Z80 driver as source (2026-10-22)

Context: about 40 routines (start-up, VDP/DMA, RLE unpackers, Z80 bus and sound interface, ~10 KB)
are classified hand-written 68000 assembly with evidence, and the Z80 sound driver (1,332 bytes
plus a 38-byte boot stub) is Z80 machine code. "Done" requires code from source and no generated
assembly units, and committing ROM-derived assembly to the public repository needs the user's
approval.

User decision:

- Commit the hand-written 68000 routines as readable GNU `as` source (`src/asm/`): named labels,
  comments, equates and structure, reviewed like the C. The build assembles them in place of the
  generated units, byte-identical in both revisions.
- Commit the Z80 driver as Z80 assembly source, assembled with the pinned `z80-elf` binutils at
  build time in place of the extracted binary units.
- Extracted assets (graphics, sound samples, levels, tables) stay uncommitted, as before.
