#ifndef TJE_ASSETS_MAP_H
#define TJE_ASSETS_MAP_H

#include "map.h"

/* Shared map, terrain, and map-screen ROM tables, translated for each revision. */
extern unsigned short EarlMapIcon[16];
extern unsigned short ElevatorMapIcon[16];
extern unsigned short FixedWorldSeeds[26];
extern unsigned short FunkatronMapData[MAP_ROWS][MAP_COLUMNS];
extern unsigned char Level0MapData[5][4];
extern unsigned char Level1MapData[MAP_ROWS][MAP_COLUMNS];
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
/* Overlapping views retain the lookup bank's existing element types
 * and bounds, including its present-terrain and input-direction fields. */
extern unsigned char MapBoundaryShapesPlain[20];
extern unsigned char MapBoundaryShapesAlternate[20];
extern unsigned char MapPathTurnDirections[];
extern char LargeItemExcludedTerrainIds[23];
extern unsigned char ControlRemapDirections[4][10];

/* Level-dependent coordinate ranges used during map generation. */
extern unsigned char MapGenerationRangePairs[8][8][2];
extern char MapGenerationRangeRow[];

/* Twelve frames of the map-cover tile uncovering animation. */
#define MAP_UNCOVER_FRAME_COUNT 12
#define MAP_UNCOVER_PIECE_COUNT 2
extern struct render_frame *Spr_MapTileUncover[MAP_UNCOVER_FRAME_COUNT];
#endif
