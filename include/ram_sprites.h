#ifndef TJE_RAM_SPRITES_H
#define TJE_RAM_SPRITES_H

#include "sprites.h"

/* Separate pools: the matching counts of hardware sprites and tile groups
 * do not imply that they share allocation state. */
#define HARDWARE_SPRITE_COUNT 80
#define VIEWPORT_SPRITE_SLOT_COUNT 54
#define SPRITE_TILE_GROUP_COUNT 80
#define VIEWPORT_SPRITE_FIRST 25
#define SPRITE_TILE_POOL_FIRST 640
#define SPRITE_TILES_PER_GROUP 8

/* Dynamic entity handles map to the allocator's separate byte flags. */
#define DYNAMIC_ENTITY_FIRST_SLOT 6
#define DYNAMIC_ENTITY_GAMEPLAY_SLOTS 20
#define DYNAMIC_ENTITY_SLOT_COUNT 34

/* gViewportSpriteSlotFlags bits, as written by allocation and tested by release. */
#define SPRITE_SLOT_ALLOCATED 1
#define SPRITE_SLOT_LOCKED 2
#define SPRITE_SLOT_SECONDARY_VIEW 4

/* Both animation-motion and graphics-queue passes enforce this per-frame tile budget. */
#define SPRITE_FRAME_TILE_BUDGET 112

struct object_state;

/* Eighty hardware sprite records. The following byte views retain
 * their canonical types: gHardwareSpriteRootLink is record zero's link byte, gHudHardwareSpriteBytes
 * begins at record eleven, and gPrimaryViewportSentinelLink is record 23's link byte.
 * The unsized view does not establish an independent allocation. */
extern struct hardware_sprite gHardwareSpriteTable[HARDWARE_SPRITE_COUNT];
extern unsigned char gHardwareSpriteRootLink;
extern char gHudHardwareSpriteBytes[];
extern unsigned char gPrimaryViewportSentinelLink;

/* gSpriteTileUploadBuffer[8192] is the existing bounded byte view. Its capacity is an
 * inferred available span to the adjacent state word, not a recovered
 * independent allocator bound. */
#ifndef TJE_LOCAL_GSPRITETILEUPLOADBUFFER
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
extern char gViewportSpriteSlotFlags[VIEWPORT_SPRITE_SLOT_COUNT];
extern char gSpriteTileGroupAllocated[SPRITE_TILE_GROUP_COUNT];
extern char gDynamicEntitySlotAllocated[DYNAMIC_ENTITY_SLOT_COUNT];

/* Funkatron placement-to-object handles and scene update phase. */
extern char gFunkatronPlacementEntities[277];
extern char gFunkatronUpdatePhase;

/* Enable copying of sprite pieces that share previously uploaded tile data. */
extern short gSharedTileSpriteCopiesEnabled;

#endif
