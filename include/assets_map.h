#ifndef TJE_ASSETS_MAP_H
#define TJE_ASSETS_MAP_H

#include "map.h"

/* Shared map, terrain, and map-screen ROM tables. Address-spelled names
 * always identify REV 02 storage, including when building REV 00. */
extern unsigned short EarlMapIcon[16];
extern unsigned short ElevatorMapIcon[16];
extern unsigned short FixedWorldSeeds[26];
extern unsigned short FunkatronMapData[26][19];
extern unsigned char Level0MapData[5][4];
extern unsigned char Level1MapData[26][19];
extern unsigned short LevelMetatileMaps[122][8][16];
extern unsigned char LevelTiles[];
extern short LevelTilesSize;
extern unsigned char MapCoverLayout[4][5];
extern struct map_generation_tables MapGenerationLookupTables;
extern short MapScreenTileCount;
extern unsigned char MapScreenTiles[];
extern struct map_transition_descriptor MapTransitionDescriptors[16];
extern unsigned short OverlayMetatileMaps[15][8][16];
extern unsigned char PlayerTilePropertyLookup[398];
extern unsigned short ShipPieceMapIcon[64];
#ifndef TJE_LOCAL_TILEALTERNATIVEPOINTERS
extern unsigned char *TileAlternativePointers[98];
#endif
extern unsigned char TileLookupAuxiliary[20];
extern unsigned short ToejamMapIcon[16];
/* Address-spelled views retain the lookup bank's current element types
 * and bounds, including its present-terrain and input-direction tables. */
extern unsigned char MapBoundaryShapesPlain[20];
extern unsigned char MapBoundaryShapesAlternate[20];
extern unsigned char MapPathTurnDirections[];
extern char LargeItemExcludedTerrainIds[23];
extern unsigned char ControlRemapDirections[4][10];

/* Additional selected final-batch storage and resource interfaces. */
extern unsigned char MapGenerationRangePairs[8][8][2];
extern char MapGenerationRangeRow[];

#endif
