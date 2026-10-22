#ifndef TJE_RAM_PRESENTS_H
#define TJE_RAM_PRESENTS_H

#include "presents.h"

/* Present identity and inventory storage; display/selection state lives
 * in ram_ui.h, without importing that header here. */
extern struct present_info gPresentInfo[28];
extern char gPlayerInventory[2][16];

/* Two 32-record world-item banks. LEVEL_ITEMS_2 is the existing alias
 * of gObjectTable[1], not another bank allocation. The scalar loaded
 * and player flags likewise alias element one of their byte-array views.
 * Incomplete outer bounds are retained where capacity is not established. */
extern struct level_item gObjectTable[2][32];
extern struct level_item LEVEL_ITEMS_2[32];
extern char LEVEL_PLAYERS_FLAG_1[];
extern char LEVEL_PLAYERS_FLAG_2;
extern char LEVEL_LOADED_1[];
extern char LEVEL_LOADED_2;

/* Dropped-present records and the existing per-level item mask. */
extern struct level_item gDroppedPresentTable[32];
extern unsigned long ram_ffdde8[26];

/* Two eight-byte telephone records and separate ringing state.
 * Existing incomplete state-array bounds are not promoted to guessed sizes. */
extern struct telephone_state TELEPHONE_1[2];
extern char ram_ffe25e[];
extern char gTelephoneRingsLeft[];
extern char gTelephoneTimeToRing[];

#endif
