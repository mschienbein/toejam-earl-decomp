#ifndef TJE_MAP_H
#define TJE_MAP_H

#include "terrain.h"

/* MapCarveDirectionalPath and MapReversePathDirection establish this separate direction encoding. */
#define MAP_PATH_UP 1
#define MAP_PATH_RIGHT 2
#define MAP_PATH_DOWN 4
#define MAP_PATH_LEFT 8
/* MapReversePathDirection preserves -1 when no traversal direction has been selected. */
#define MAP_PATH_DIRECTION_NONE (-1)
/* MapConsumePathCellDirections returns -1 when the cell has no pending traversal to consume. */
#define MAP_PATH_CELL_UNAVAILABLE (-1)
/* MapGrowTerrainRegion tests all four attempted directions against this mask. */
#define MAP_PATH_DIRECTION_MASK 15
/* MapFindRegionGrowthOrigin and MapAnimateMetatilePathChange distinguish the two axes. */
#define MAP_PATH_VERTICAL_MASK 5
#define MAP_PATH_HORIZONTAL_MASK 10
/* MapGetBankPathDirections and CopyLevelBetweenPlayers preserve the other bank's upper nibble. */
#define MAP_PATH_SECONDARY_BANK_MASK 0xf0

/* MapFindRouteToSecondAnchor dispatches region kinds separately from their low-byte directions. */
#define MAP_REGION_KIND_MASK 0x3000
/* MapGrowTerrainRegion merges labels of this boundary-region kind. */
#define MAP_REGION_BOUNDARY 0x1000
/* MapCarveDirectionalPath writes this kind before MapBuildDirectionalPathShapes selects shapes. */
#define MAP_REGION_PATH 0x2000
/* MapSeedFilledTerrainRegions requests this kind for MapBuildFilledTerrainShapes. */
#define MAP_REGION_FILLED 0x3000
/* MapFindInteriorGrowthOrigin preserves the three-bit region label in bits 8..10. */
#define MAP_REGION_LABEL_MASK 0x700
/* MapSeedFilledTerrainRegions and MapBuildRegionEdgeShapes extract that shifted label. */
#define MAP_REGION_LABEL_INDEX_MASK 7
/* MapFindRouteToSecondAnchor and the shape passes mark cells visited in their current scan. */
#define MAP_REGION_VISITED 0x800
/* MapSeedFilledTerrainRegions and MapBuildFilledTerrainShapes clear the temporary visit bit. */
#define MAP_REGION_VISITED_CLEAR_MASK 0xf7ff
/* MapBuildRegionEdgeShapes selects edge_alternate/filled_alternate through this bit. */
#define MAP_REGION_ALTERNATE_SHAPES 0x8000

/* MapBuildRegionEdgeShapes and MapBuildFilledTerrainShapes test the four adjoining 2x2 corners. */
#define MAP_CORNER_UPPER_LEFT 1
#define MAP_CORNER_UPPER_RIGHT 2
#define MAP_CORNER_LOWER_RIGHT 4
#define MAP_CORNER_LOWER_LEFT 8
/* MapRefineFilledShapeMasks tests whether all four corner bits are present. */
#define MAP_CORNER_MASK 15
/* MapTraceTransitionChain stores a descriptor number in bits 8..12, not a final overlay selector. */
#define MAP_TRANSITION_MASK 0x1f00
/* MapAssignTransitionChain clears that descriptor field before its scan. */
#define MAP_TRANSITION_CLEAR_MASK 0xe0ff

/* MapFindItemPlacementPosition rejects this bit; path conversion and consumption set it. */
#define METATILE_ITEM_PLACEMENT_INHIBITED 0x2000
/* MapBuildDirectionalPathShapes marks pending land changes; ProcessLand consumes them. */
#define METATILE_PENDING_PATH_CHANGE 0x4000
/* MapChooseMetatileAlternatives keeps pending-path and alternate-shape bits while replacing IDs. */
#define METATILE_GENERATION_PRESERVE_MASK 0xc000
/* MapConsumePathCellDirections keeps alternate-shape and overlay bits while replacing a path cell. */
#define METATILE_PATH_REPLACEMENT_MASK 0x9f00

/* Complete 196-byte map-generation lookup bank at REV 02 08bf4a.
 * Its terrain-exclusion and directional-input views are shared below. */
struct map_generation_tables {
    unsigned char tiles[16];                         /* 00: shape-to-metatile IDs; ROM-authored. */
    unsigned char edge_plain[20], edge_alternate[20]; /* 10/24: boundary shape lookups; ROM-authored. */
    unsigned char filled_plain[32], filled_alternate[32]; /* 38/58: filled shape lookups; ROM-authored. */
    unsigned char direction_choices[12];             /* 78: path turn choices; ROM-authored. */
    char present_terrain[23];                        /* 84: excluded large-item metatile IDs; ROM-authored. */
    unsigned char unused;                            /* 9b: no represented consumer; ROM-authored. */
    unsigned char input_directions[4][10];            /* 9c: control-remap directional masks; ROM-authored. */
};

/* Ten bytes with two-byte target alignment: pointer fields are at 02/06.
 * shapes points to a count byte followed by that many shape bytes. */
struct map_transition_descriptor {
    unsigned char bank, chain_flags;                 /* 00/01: shape-bank selector and chain flags; ROM-authored. */
    unsigned char *shapes, *alternatives;             /* 02/06: counted shapes and replacement IDs; ROM-authored. */
};

#endif
