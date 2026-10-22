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
extern int IsToejamNearEdge();
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
extern void func_00334a();
extern void func_003604();
extern void func_0039e8();
extern void func_003cb6();
extern void func_004174();
extern void func_00433e();
extern void func_0044e2();
extern int func_0046c6();
extern void func_004842();
extern void func_004dfe();
extern unsigned long func_004f50();
extern void func_004fee();
extern short func_0053c2();
extern short func_00557a();
extern void func_005776();
extern void func_005afa();
extern short func_005bfe();
extern void func_0060ae();
extern void func_0062ca();
extern void func_0064ea();
extern void func_00655a();
extern void func_00671c();
extern int func_006b22();
extern int func_006b74();
extern void func_0071e8();
extern int func_0072f0();
extern int func_0077ae();
extern short func_0077d8();
extern void func_007804();
extern void func_0078a8();
extern int func_007b24();
extern int func_00815c();
extern void func_008232();
extern int func_008356();
extern void func_00871a();
#ifndef TJE_LOCAL_FUNC_00D9DC
extern unsigned char func_00d9dc();
#endif
extern short func_01e736();
extern void func_01e94e();
extern char func_01e96c();
extern char func_01e9a0();
extern unsigned char func_01ea06();
extern char func_01ea66();
extern void func_01eddc();
extern void func_01ef56();
extern void func_01f0c4();
extern void func_01f1b0();
extern void func_01f418();
extern void func_01f4f8();

#endif
