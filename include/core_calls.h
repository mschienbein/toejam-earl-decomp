#ifndef TJE_CORE_CALLS_H
#define TJE_CORE_CALLS_H

/* K&R return interfaces for core routines. */
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
extern void func_00059e();
extern void func_000be2();
extern void func_000c2e();
extern void func_000d7e();
extern void func_000ee6();
extern int func_000fb6();
extern void func_00105a();
#ifndef TJE_LOCAL_FUNC_001262
extern short func_001262();
#endif

#endif
