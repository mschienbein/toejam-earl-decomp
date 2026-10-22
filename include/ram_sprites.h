#ifndef TJE_RAM_SPRITES_H
#define TJE_RAM_SPRITES_H

#include "sprites.h"

struct object_state;

/* Eighty hardware sprite records. The following byte views retain
 * their canonical types: gHardwareSpriteRootLink is record zero's link byte, gHudHardwareSpriteBytes
 * begins at record eleven, and gPrimaryViewportSentinelLink is record 23's link byte.
 * The unsized view does not establish an independent allocation. */
extern struct hardware_sprite gHardwareSpriteTable[80];
extern unsigned char gHardwareSpriteRootLink;
extern char gHudHardwareSpriteBytes[];
extern unsigned char gPrimaryViewportSentinelLink;

/* gSpriteTileUploadBuffer[8192] is the existing bounded byte view. Its capacity is an
 * inferred available span to the adjacent state word, not a recovered
 * independent allocator bound. */
#ifndef TJE_LOCAL_RAM_FFB8DA
extern char gSpriteTileUploadBuffer[8192];
#endif

/* Tile-cache state, sprite-load words and hardware-link bookkeeping. */
extern unsigned short gSpriteTileBufferOffset;
extern unsigned short gSpriteTileUploadRuns[40];
extern unsigned char gSpriteTileUploadQueueWordCount;
extern unsigned char gPrimaryViewportUploadedLink;
extern unsigned char gSecondaryViewportUploadedLink;

/* Entity draw-list heads; only pointer declarations need the record tag. */
extern struct object_state *gEntityDrawListHead;
extern struct object_state *gInventoryDrawListHead;

/* Allocation flags for 54 sprite slots, 80 tile groups and 34 dynamic
 * object slots. The dynamic allocator uses 20 slots normally and 34 on
 * level 26; the existing array capacities are retained. */
extern char gViewportSpriteSlotFlags[54];
extern char gSpriteTileGroupAllocated[80];
extern char gDynamicEntitySlotAllocated[34];

/* Funkatron placement-to-object handles and scene update phase. */
extern char gFunkatronPlacementEntities[277];
extern char gFunkatronUpdatePhase;

/* Additional selected final-batch storage and resource interfaces. */
extern short gSharedTileSpriteCopiesEnabled;

#endif
