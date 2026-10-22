#ifndef TJE_TERRAIN_H
#define TJE_TERRAIN_H

/* Tile high-nibble classes and separately named packed-metatile bits; these are not map-generation
 * region labels. */
#define TERRAIN_CLASS_MASK 0xf0
#define TERRAIN_GROUND 0x10
#define TERRAIN_SINKING 0x20
#define TERRAIN_FAST_GROUND 0x30
#define TERRAIN_WALKABLE_MAX 0x50
#define TERRAIN_WATER 0x80
#define TERRAIN_VOID 0x90
#define METATILE_MIRROR_X 0x80
#define METATILE_BASE_MASK 0x7f
#define METATILE_OVERLAY_MASK 0x1f00

#endif
