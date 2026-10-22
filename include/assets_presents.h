#ifndef TJE_ASSETS_PRESENTS_H
#define TJE_ASSETS_PRESENTS_H

#include "presents.h"

struct render_frame;

/* Shared present, ground-item, and projectile ROM assets. The frame tables
 * contain pointers only; their complete records belong to sprites.h.
 * Symbol addresses are translated for the selected ROM revision. */
extern struct render_frame *LargeGroundItemFrames[3];
extern struct item_position Level1PresentLocations[12];
extern char PresentAuxiliarySignedLookup[16];
extern struct render_frame *PresentSpriteTable[28];
extern char PresentWeightTable[27];
extern char ProjectilePlacementTriples[][2][4][3];
extern struct render_frame *Spr_GroundItem[17];
extern struct render_frame *Spr_PresentSelector[1];
/* Shared one-frame hold sequence used by item and selector rendering. */
extern char SingleFrameHoldAnimation[];
extern char SingleFrameHoldAnimation[];

#endif
