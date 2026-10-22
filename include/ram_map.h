#ifndef TJE_RAM_MAP_H
#define TJE_RAM_MAP_H

#include "terrain.h"

/* World/level selection and per-level seeds. */
extern char gRequestedMapLevel;
extern char gPlayer1MapLevel;
extern char gPlayer2MapLevel;
extern char gSavedMapLevel;
extern char gHighestLevel;
extern char gWorldType;
extern unsigned short gLevelSeeds[26];

/* Canonical map banks. gSavedMapPathDirections is the existing byte view beginning
 * at gMapPathDirectionBanks[26][0], not an additional independent terrain allocation. */
extern unsigned short gPlayer1LevelMap[MAP_ROWS][MAP_COLUMNS];
extern unsigned short gPlayer2LevelMap[MAP_ROWS][MAP_COLUMNS];
extern unsigned short gSavedLevelMap[MAP_ROWS][MAP_COLUMNS];
extern unsigned char gMapPathDirectionBanks[MAP_ROWS * 2][MAP_COLUMNS];
extern unsigned char gSavedMapPathDirections[MAP_ROWS][MAP_COLUMNS];

/* Transition-path coordinates and map-generation state. */
extern short gTransitionPathVisitCount;
extern short gTransitionPathMinimumVisits;
extern char gMapDirectionalPathMode;

/* DisplayMap accesses gMapVisitedCoverBits and gMapRevealedCoverBits with the same seven-byte
 * row stride; the former 26-row extent is [INFERENCE] from the next table. */
extern unsigned char gMapVisitedCoverBits[26][7];
extern unsigned char gMapRevealedCoverBits[26][7];

/* Map-display state and the terrain-query word. gTerrainQueryMetatileLowByte is its
 * existing low-byte alias on the big-endian target. */
extern short gMapScreenNextSpriteIndex;
extern short gMapRevealMode;
extern unsigned short gTerrainQueryMetatileWord;
extern unsigned char gTerrainQueryMetatileLowByte;

/* Eight land-animation object handles and the current traversal direction. */
extern char gLandPathAnimationEntities[8];
extern char gLandPathTraversalDirection;

#endif
