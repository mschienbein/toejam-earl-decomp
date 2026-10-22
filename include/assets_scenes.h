#ifndef TJE_ASSETS_SCENES_H
#define TJE_ASSETS_SCENES_H

#include "sprites.h"

/* Shared Funkatron, ship-screen, and transition-background ROM assets.
 * Address-spelled names keep their REV 02 identities in both revisions. */
extern unsigned char FunkatronExtraTiles[];
extern struct scene_descriptor FunkatronSceneDescriptors[30];
extern struct scene_placement FunkatronScenePlacements[277];
extern char ImgStarfieldLarge[];
extern unsigned char ShipRelatedData[8][8];
extern struct render_frame *Spr_TransitionBackground[50];
extern short StarryBackgroundSize;
/* Two opaque pointer words: only slot 1 is proved to target a frame table.
 * A sequence/frame-descriptor interpretation of the pair is not established
 * [INFERENCE]; no pointee type is asserted for slot 0. */
extern void *AnimationPointerWords_AF040[2];
extern char FixedWorldShipPieces[10];

/* Additional selected final-batch storage and resource interfaces. */
extern char StarfieldByteMap[32][32];

#endif
