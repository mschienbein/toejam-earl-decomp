#ifndef TJE_HARDWARE_H
#define TJE_HARDWARE_H

/* Separate names preserve the hardware access width at the same VDP port.
 * MCC68K has no volatile keyword; these retain the original K&R accesses. */
#define VDP_CONTROL_WORD (*(unsigned short *)0xc00004L)
#define VDP_COMMAND (*(unsigned long *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)
#define VRAM_WRITE(address) (0x40000000L + (((address) & 0x3fff) << 16) + (((address) & 0xc000) >> 14))

#define Z80_RAM ((char *)0xa00000L)
#define Z80_RESET (*(short *)0xa11200L)
#define Z80_BUS_REQUEST (*(short *)0xa11100L)

/* Controller port data/direction accesses retain their observed byte types. */
#define CONTROL_1_DIRECTION (*(unsigned char *)0xa10009L)
#define CONTROL_1_DATA (*(char *)0xa10003L)
#define CONTROL_2_DIRECTION (*(unsigned char *)0xa1000bL)
#define CONTROL_2_DATA (*(char *)0xa10005L)

#endif
