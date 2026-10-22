#ifndef TJE_CORE_CALLS_H
#define TJE_CORE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern int CheckStartButton();
extern void ClearSkipVHBlankFlags();
extern void DelayRoutine1();
extern void DrawLevelGraphics();
extern void FurtherInitialisation();
extern void GetPlayerInput();
extern int GetSkipVBlankFlag();
extern void HandleHBlank();
extern void HandleVBlank();
extern void InitialiseTheZ80();
extern long MovePadInputIntoD0();
extern void ProcessInput();
extern void ResetVDP();
extern void SetSkipVHBlankFlags();
extern void ToggleLOC_00FF8196();
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
