#ifndef TJE_ASSETS_PRESENTS_H
#define TJE_ASSETS_PRESENTS_H

#include "presents.h"

struct render_frame;

/* Shared present, ground-item, and projectile ROM assets. The frame tables
 * contain pointers only; their complete records belong to sprites.h.
 * Address-spelled identities always refer to REV 02, including REV 00 builds. */
extern struct render_frame *FramePointers_AF09C[3];
extern struct item_position Level1PresentLocations[12];
extern char PresentAuxiliarySignedLookup[16];
extern struct render_frame *PresentSpriteTable[28];
extern char PresentWeightTable[27];
extern char ProjectilePlacementTriples[][2][4][3];
extern struct render_frame *Spr_GroundItem[17];
extern struct render_frame *Spr_PresentSelector[1];
/* Both current address-spelled views identify the same sequence storage. */
extern char dat_0ae4a0[];
extern char rom_0ae4a0[];

#endif
