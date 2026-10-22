#ifndef TJE_RAM_SPRITES_H
#define TJE_RAM_SPRITES_H

#include "sprites.h"

struct object_state;

/* Eighty hardware sprite records. The following byte views retain
 * their canonical types: ram_ffb65d is record zero's link byte, ram_ffb6b2
 * begins at record eleven, and ram_ffb715 is record 23's link byte.
 * The unsized view does not establish an independent allocation. */
extern struct hardware_sprite ram_ffb65a[80];
extern unsigned char ram_ffb65d;
extern char ram_ffb6b2[];
extern unsigned char ram_ffb715;

/* ram_ffb8da[8192] is the existing bounded byte view. Its capacity is an
 * inferred available span to the adjacent state word, not a recovered
 * independent allocator bound. */
#ifndef TJE_LOCAL_RAM_FFB8DA
extern char ram_ffb8da[8192];
#endif

/* Tile-cache state, sprite-load words and hardware-link bookkeeping. */
extern unsigned short SPRITE_STATE_TABLE;
extern unsigned short ram_ffd8dc[40];
extern unsigned char ram_ffd92e;
extern unsigned char ram_ffd932;
extern unsigned char ram_ffd934;

/* Entity draw-list heads; only pointer declarations need the record tag. */
extern struct object_state *ram_ffd938;
extern struct object_state *ram_ffd93c;

/* Allocation flags for 54 sprite slots, 80 tile groups and 34 dynamic
 * object slots. The dynamic allocator uses 20 slots normally and 34 on
 * level 26; the existing array capacities are retained. */
extern char ram_ffd940[54];
extern char ram_ffd976[80];
extern char ram_ffd9c6[34];

/* Funkatron placement-to-object handles and scene update phase. */
extern char ram_ffe264[277];
extern char ram_ffe390;

#endif
