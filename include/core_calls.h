#ifndef TJE_CORE_CALLS_H
#define TJE_CORE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern int CheckStartButton();
extern void DisableBlankingGraphics();
extern void DelayIterations();
extern void DrawLevelGraphics();
extern void FurtherInitialisation();
extern void GetPlayerInput();
extern int GetVBlankGraphicsEnabled();
extern void HandleHBlank();
extern void HandleVBlank();
extern void InitialiseTheZ80();
extern long ReadMappedPlayerInput();
extern void ProcessInput();
extern void ResetVDP();
extern void EnableBlankingGraphics();
extern void SetCameraTilePriorityEnabled();
extern void VDPInitialisation();
extern void WaitForNextVBlank();
extern void SelectViewportSpriteChain();
extern void ReloadActiveCameraViews();
extern void EnterSplitScreen();
extern void EnterSharedScreen();
extern void CenterSharedCameraOnPlayer();
extern int PlayersRequireSeparateViews();
extern void UpdateScreenSplitMode();
extern short GetLevelMusicIndex();

#endif
