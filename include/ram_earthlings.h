#ifndef TJE_RAM_EARTHLINGS_H
#define TJE_RAM_EARTHLINGS_H

#include "earthlings.h"

/* Two twenty-record earthling banks. gEarthlingBank2Records is the
 * existing record-array alias of gEarthlingBanks[1]. The load-flag
 * copy likewise aliases gEarthlingBankUsers[1]; incomplete outer bounds
 * retain the canonical views rather than an adjacent-symbol estimate. */
extern struct earthling_state gEarthlingBanks[2][20];
extern struct earthling_state gEarthlingBank2Records[20];
extern char gEarthlingBankUsers[];
extern char gEarthlingBank2Users;
extern char gEarthlingBankLevels[];

/* Per-level spawn masks and bank processing phase. */
extern unsigned long gEarthlingSpawnFailureMasks[26];
extern short gEarthlingUpdateParity;

/* Two six-byte special-earthling records and kind-dependent contact
 * state. gPlayer2WisemanInteractionState keeps the scalar byte alias of gPlayerWisemanInteractionStates[1]. */
extern struct special_earthling_state gMailboxLevelStates[2];
extern char gPlayerWizardInteractionStates[];
extern char gPlayerWisemanInteractionStates[];
extern char gPlayer2WisemanInteractionState;
extern char gPlayerOperaSingerInteractionStates[];

/* Per-player mole/present interaction state keeps its incomplete bounds. */
extern char gPlayerStolenPresentEntities[];
extern char gPlayerMoleTheftCounts[];

#endif
