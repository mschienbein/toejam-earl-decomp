#ifndef TJE_RAM_EARTHLINGS_H
#define TJE_RAM_EARTHLINGS_H

#include "earthlings.h"

/* Two twenty-record earthling banks. EARTHLINGS_IN_LEVEL_COPY is the
 * existing record-array alias of EARTHLINGS_IN_LEVEL[1]. The load-flag
 * copy likewise aliases EARTHLING_LOAD_FLAG[1]; incomplete outer bounds
 * retain the canonical views rather than an adjacent-symbol estimate. */
extern struct earthling_state EARTHLINGS_IN_LEVEL[2][20];
extern struct earthling_state EARTHLINGS_IN_LEVEL_COPY[20];
extern char EARTHLING_LOAD_FLAG[];
extern char EARTHLING_LOAD_FLAG_COPY;
extern char EARTHLING_LEVEL[];

/* Per-level spawn masks and bank processing phase. */
extern unsigned long ram_ffe142[26];
extern short ram_ffe1ae;

/* Two six-byte special-earthling records and kind-dependent contact
 * state. ram_ffe23d keeps the scalar byte alias of ram_ffe23c[1]. */
extern struct special_earthling_state ram_ffe226[2];
extern char ram_ffe23a[];
extern char ram_ffe23c[];
extern char ram_ffe23d;
extern char ram_ffe23e[];

/* Per-player mole/present interaction state keeps its incomplete bounds. */
extern char ram_ffe24a[];
extern char ram_ffe24c[];

#endif
