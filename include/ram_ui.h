#ifndef TJE_RAM_UI_H
#define TJE_RAM_UI_H

#include "entities.h"

/* Inventory control and text state keeps its canonical byte views.
 * Incomplete outer bounds are retained, not inferred from adjacent symbols.
 * P1_MENU_OPEN[1] also has the scalar HBLANK_FLAG_1 name in ram_system.h. */
extern char P1_TEXT_WIGGLE;
extern char P1_MENU_COUNTER;
extern char P1_MENU_OPEN[];
extern char P1_MENU_SCROLL[];
extern char P1_MENU_SEL_ROW[];
extern char P1_MENU_SEL_COL[];
extern char ram_ff9362[];
extern char ram_ff936c;
extern short ram_ff936e;
extern short ram_ff9370;
extern short ram_ff9372;
extern short ram_ff9374;
extern char ram_ff9378;
extern char gPlayer1MailboxItems[][3];
extern char gPlayer1MailboxPrices[][3];
extern char ram_ff9386;
extern char TEXT_SPRITE_DATA_START[][1888];

/* Existing inventory-object aliases within gPlayerInfo[40]: selector
 * records 26 and 33, and six visible-item records 27..32 and 34..39.
 * The scalar flag bytes alias the selectors' flags_4d fields; these are
 * overlapping views, not additional object or byte allocations. */
extern struct object_state ram_ffaf5a;
extern char ram_ffafa7;
extern struct object_state ram_ffafda[6];
extern struct object_state ram_ffb2da;
extern char ram_ffb327;
extern struct object_state ram_ffb35a[6];

/* One-time present-display drawing state. */
extern char ram_ffe1ec;

/* Per-player two-choice dialog handles/results and input inhibition.
 * Existing incomplete bounds and independent parallel arrays are retained. */
extern char ram_ffe232[];
extern char ram_ffe234[];
extern char ram_ffe236[];
extern char ram_ffe238[];

/* Life-request dialog state and its display object handle. */
extern short ram_ffe240;
extern short ram_ffe242;

#endif
