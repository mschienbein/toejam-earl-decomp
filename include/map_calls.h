#ifndef TJE_MAP_CALLS_H
#define TJE_MAP_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern int ChangeLevel();
extern void CopyLevelBetweenPlayers();
extern void DisplayMap();
extern void DrawGameMap();
extern void DrawMapCharacters();
extern void GenerateLevelSeeds();
extern int PlayerIsNearTerrainEdge();
extern void LoadFinalLevel();
extern void LoadLevel1Tiles();
extern void LoadMapIntoVRAM();
extern void Map_DrawElevator();
extern void Map_DrawPlayer();
extern void Map_DrawShipPiece();
extern void ProcessLand();
extern void SetPositionOnNewLevel();
extern int TerrainValidation();
extern void UpdateLandAroundPlayer();
extern void MapRefineFilledShapeMasks();
extern void MapBuildFilledTerrainShapes();
extern void MapBuildDirectionalPathShapes();
extern void MapScatterTerrainFeatures();
extern void MapChooseMetatileAlternatives();
extern void MapSwapPlayerBanks();
extern void MapSwapPlayerWithSavedBank();
extern int MapGenerateSeparatedAnchors();
extern void MapConnectAnchorPositions();
extern void MapLoadPlayerLevel();
extern unsigned long MapGetRandomGenerationParameter();
extern void MapCarveDirectionalPath();
extern short MapFindRegionGrowthOrigin();
extern short MapFindInteriorGrowthOrigin();
extern void MapGrowTerrainRegion();
extern void MapFillRectangleAndMergeRegions();
extern short MapExpandRegionsBesidePaths();
extern void MapWidenHorizontalRegionEdges();
extern void MapWidenVerticalRegionEdges();
extern void MapStabilizeRegionConnections();
extern void MapSeedFilledTerrainRegions();
extern void MapBuildRegionEdgeShapes();
extern int MapTransitionContainsMetatile();
extern int MapTraceTransitionChain();
extern void MapAssignTransitionChain();
extern int MapFindRouteToSecondAnchor();
extern int MapIsAnchorConnectionMissing();
extern short MapChooseBackgroundTileBucket();
extern void MapDrawFixedBackgroundDecorations();
extern void MapDrawBackgroundAndTerrain();
extern int MapCoverBlockContainsTerrain();
extern int MapAppendCoverBlockSprites();
extern void MapInitializeScreenSprites();
extern int PlayerHasOddElevatorState();
extern void MapUploadCoverAnimationFrame();
#ifndef TJE_LOCAL_FUNC_00D9DC
extern unsigned char GetMapTerrainAtWorldPosition();
#endif
extern short AllocateEntityWithEviction();
extern void EntityReleaseOneByPriority();
extern char MapReversePathDirection();
extern char MapFindPathShapeForMetatile();
extern unsigned char MapGetBankPathDirections();
extern char MapConsumePathCellDirections();
extern void MapQueueVisibleMetatileRow();
extern void MapQueueVisibleMetatileColumn();
extern void EntityStartLandPathAnimation();
extern void MapAnimateMetatilePathChange();
extern void EntityReserveLandPathAnimations();
extern void MapAnimateMarkedLandPaths();

#endif
