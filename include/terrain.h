#ifndef TJE_TERRAIN_H
#define TJE_TERRAIN_H

/* Metatile-grid dimensions, distinct from the numerically equal final-level ID. */
#define MAP_ROWS 26
#define MAP_COLUMNS 19
#define LEVEL_FINAL 26
/* Level identities: the hidden level 0, the first level, the last ordinary level (the last that can
 * hold a ship piece before the final one), and the "no level loaded" sentinel. */
#define LEVEL_ZERO 0
#define LEVEL_FIRST 1
#define LEVEL_LAST_ORDINARY 25
#define LEVEL_UNLOADED (-1)

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
