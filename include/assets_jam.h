#ifndef TJE_ASSETS_JAM_H
#define TJE_ASSETS_JAM_H

#include "sprites.h"

/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern unsigned char CrashPackedTilemapA[16][22];
extern unsigned char CrashPackedTilemapB[14][20];
extern char JamOutCharacterAnimations[];
extern struct render_frame *Spr_JamoutEarl[];
extern struct render_frame *Spr_JamoutToejam[];
extern char JamBassDrumAlternateSequence[];
extern char JamLeadIdleLoopSequence[];
extern char JamSnareDrumFirstSequence[];
extern char JamSnareDrumSecondSequence[];
extern char JamClapSequence[];
extern char JamVocalOrScratchSequence[];
extern char JamLeadIdleTransitionSequence[];

#endif
