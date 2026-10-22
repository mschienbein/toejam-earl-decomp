#ifndef TJE_HARDWARE_CALLS_H
#define TJE_HARDWARE_CALLS_H

/* ROM-only reset, interrupt, controller, and VDP transfer entries. These
 * retain established caller interfaces; the ABI alone does not uniquely
 * establish historical C return spellings for unrepresented implementations. */
extern void ClearColourRAM();
extern void ClearSRAM();
extern void ClearVideoRAM();
extern void CopyPaletteToCRAMSlot();
extern void EnableAllInterrupts();
extern int GetCtrl1Input();
extern int GetCtrl2Input();
extern void MoveDataToVRAM();
extern void VDPWritesRoutine1();
/* Machine reset trampoline: its void spelling is a caller interface only. */
extern void func_00034e();
extern void func_00046a();

#endif
