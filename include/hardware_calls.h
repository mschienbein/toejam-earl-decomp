#ifndef TJE_HARDWARE_CALLS_H
#define TJE_HARDWARE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void ClearColourRAM();
extern void ClearSRAM();
extern void ClearVideoRAM();
extern void CopyPaletteToCRAMSlot();
extern void EnableAllInterrupts();
extern int GetCtrl1Input();
extern int GetCtrl2Input();
extern void MoveDataToVRAM();
extern void VDPWritesRoutine1();
extern void RestartFromResetEntry();
extern void DrawStarfieldPlane();

#endif
