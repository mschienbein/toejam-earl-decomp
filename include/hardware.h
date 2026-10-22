#ifndef TJE_HARDWARE_H
#define TJE_HARDWARE_H

/* Separate names preserve the hardware access width at the same VDP port.
 * MCC68K has no volatile keyword; these retain the original K&R accesses. */
#define VDP_CONTROL_WORD (*(unsigned short *)0xc00004L)
#define VDP_COMMAND (*(unsigned long *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)
#define VRAM_WRITE(address) (0x40000000L + (((address) & 0x3fff) << 16) + (((address) & 0xc000) >> 14))

/* One 8x8 four-bit-per-pixel Genesis pattern. */
#define VDP_TILE_BYTES 32

/* Packed VDP control-port commands; suffixes preserve their longword view.
 * ResetVDP selects plane A at A000, window at D000 and plane B at E000. */
#define VDP_WRITE_VRAM_BASE 0x40000000L
#define VDP_WRITE_PLANE_A 0x60000002L
#define VDP_WRITE_WINDOW 0x50000003L
#define VDP_WRITE_PLANE_B 0x60000003L
#define VDP_WRITE_CRAM_BASE 0xc0000000L
#define VDP_WRITE_VSRAM_BASE 0x40000010L
#define VDP_MODE2_REGISTER 0x8104
#define VDP_AUTOINCREMENT_WORD 0x8f02
#define VDP_AUTOINCREMENT_TILE_ROW 0x8f40
#define VDP_TILE_INDEX_MASK 0x07ff
#define VDP_TILE_HORIZONTAL_FLIP 0x0800
#define VDP_TILE_VERTICAL_FLIP 0x1000
#define VDP_TILE_PRIORITY 0x8000
#define VDP_TILE_PALETTE_3 0x6000
#define VDP_SPRITE_SIZE_4X1 12

#define Z80_RAM ((char *)0xa00000L)
#define Z80_RESET (*(short *)0xa11200L)
#define Z80_BUS_REQUEST (*(short *)0xa11100L)

/* Controller port data/direction accesses retain their observed byte types. */
#define CONTROL_1_DIRECTION (*(unsigned char *)0xa10009L)
#define CONTROL_1_DATA (*(char *)0xa10003L)
#define CONTROL_2_DIRECTION (*(unsigned char *)0xa1000bL)
#define CONTROL_2_DATA (*(char *)0xa10005L)

#endif
