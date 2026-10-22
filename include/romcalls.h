#ifndef TJE_ROMCALLS_H
#define TJE_ROMCALLS_H

/* K&R return interfaces for rom routines. */
/* ROM-only declarations describe the observed ABI; the exact historical
 * C spelling is not uniquely established by result normalization alone. */
extern void CheckAButton();
extern void CheckPlayerFalling();
extern void ClearColourRAM();
extern void ClearSRAM();
extern void ClearVideoRAM();
extern void CopyLevelBetweenPlayers();
extern void CopyPaletteToCRAMSlot();
extern void DropPresent();
extern void EnableAllInterrupts();
extern short GenerateSpeech();
extern int GetCtrl1Input();
extern int GetCtrl2Input();
extern void LoadSoundDriverToZ80();
extern void MassiveRoutine();
extern void MoveDataToVRAM();
extern void NewGameMainMenu();
extern int NiceEarthlingCheck();
extern void OpenEarthlingPresent();
extern void OpenTelephone();
extern void PlaySound();
extern void PlaySoundEffect();
extern int Player_Youch();
extern void ProcessLand();
extern void RAMWritesRoutine1();
extern void SEGAAndIntroVideoRoutine();
extern void ShowShipPieces();
extern void SpriteChecks();
extern void SpriteUnpackInterleavedRLE();
extern void SpriteUnpackRLE();
extern void UpdateLandAroundPlayer();
extern void VDPWritesRoutine1();
extern long abs();
extern void func_00034e();
extern void func_00046a();
extern void func_0016d2();
extern void func_001ba8();
extern void func_0020aa();
extern void func_004842();
extern unsigned long func_004f50();
extern void func_005776();
extern void func_00871a();
extern void func_00aeb8();
extern void func_00b3e4();
extern void func_00b590();
extern int func_00c7a8();
extern void func_00cb54();
extern void func_00cc82();
extern void func_00dd82();
extern void func_00f69e();
extern void func_010294();
extern void func_010ffc();
extern int func_012028();
extern void func_018da4();
extern char func_01afb8();
extern void func_01c2a2();
extern void func_01e2c4();
extern int func_01e312();
extern int func_01e38e();
extern void func_01e4e4();
extern int func_01e736();
extern void func_01e94e();
extern void func_01f98e();
extern void func_020d3e();
extern int func_020e70();
#ifndef TJE_LOCAL_FUNC_020EAA
extern char func_020eaa();
#endif
extern void func_021d32();
extern void func_02234c();
extern void func_0223f6();
extern void func_02297c();
extern void func_023012();
extern void func_023554();
extern void func_023c6e();
extern void func_023cb4();
extern int func_025bce();
extern int func_025ca4();
extern int func_025d80();
extern int func_025e72();
extern int func_025f42();
extern void func_0282c0();
extern void func_03e40e();
extern void func_03e43c();
extern void func_03e46a();
extern void func_03e4f8();

#endif
