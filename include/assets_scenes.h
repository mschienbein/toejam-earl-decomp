#ifndef TJE_ASSETS_SCENES_H
#define TJE_ASSETS_SCENES_H

#include "sprites.h"

/* Shared Funkatron, ship-screen, and transition-background ROM assets.
 * Symbol addresses are translated for the selected ROM revision. */
extern unsigned char FunkatronExtraTiles[];
extern struct scene_descriptor FunkatronSceneDescriptors[30];
extern struct scene_placement FunkatronScenePlacements[277];
extern char ImgStarfieldLarge[];
extern unsigned char TransitionBackgroundTileOffsets[8][8];
extern struct render_frame *Spr_TransitionBackground[50];
extern short StarryBackgroundSize;
/* Two opaque pointer words: only slot 1 is proved to target a frame table.
 * A sequence/frame-descriptor interpretation of the pair is not established
 * [INFERENCE]; no pointee type is asserted for slot 0. */
extern void *IntroAndShipAnimationPointers[2];
extern char FixedWorldShipPieces[10];

/* Tile indices for the 32-by-32 starfield background. */
extern char StarfieldByteMap[32][32];

#endif
