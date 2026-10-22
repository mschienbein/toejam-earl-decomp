#ifndef TJE_RAM_SYSTEM_H
#define TJE_RAM_SYSTEM_H

/* Frame synchronization and interrupt override. INPUT_READING_FLAG_LO
 * is the low-byte alias of TIME_COUNTER on the big-endian target. */
extern unsigned short TIME_COUNTER;
extern char INPUT_READING_FLAG_LO;
extern void (*gVBlankFunc)();
extern char SKIP_HBLANK_FLAG;
extern char SKIP_VBLANK_FLAG;
extern unsigned char VBLANK_FLAG_2;
extern char VBLANK_FLAG_3;
extern char VBLANK_FLAG_4;
extern char VBLANK_FLAG_5;
extern char VBLANK_FLAG_6;
extern char VBLANK_FLAG_7;
extern char VBLANK_FLAG_8;
extern char VHBLANK_FLAG_2;
extern short WAIT_FOR_VBLANK_FLAG;

/* Existing scalar byte alias of P1_MENU_OPEN[1] in ram_ui.h; this is
 * not a separate interrupt-state allocation. */
extern char HBLANK_FLAG_1;

/* Input state keeps its existing byte-array and word views. Incomplete
 * outer bounds are retained; no adjacent-symbol guess establishes capacity.
 * ram_ff8021 also names P1_PREV_PROCESSED_INPUT[1]. */
extern unsigned short P1_PAD_INPUT;
extern unsigned short P2_PAD_INPUT;
extern unsigned char P1_PROCESSED_INPUT[];
extern unsigned char P1_PREV_PROCESSED_INPUT[];
extern unsigned char P1_KEYDOWN[];
extern char ram_ff801c;
extern unsigned char ram_ff8021;
extern char ram_ff8030;

/* VDP register soft copy and main-loop palette/frame state. */
extern unsigned char gMode2RegSoftcopy;
extern char ram_ff8024;
extern char ram_ff8026;
extern char ram_ff8028;

/* Shared random cache, its current index and generator state. */
extern unsigned short PREGEN_RANDOMS[59];
extern unsigned short ram_ff91e0;
extern long gRNGState;

/* Existing shared sound-driver state views. */
extern short gMusicUnknownE472;
extern short ram_ffe474;
extern char gMusicUnknownE488;

#endif
