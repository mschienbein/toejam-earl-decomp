#ifndef TJE_EARTHLING_CALLS_H
#define TJE_EARTHLING_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void AI_GenericEnemyThink();
extern void AI_MailboxTouch();
extern void AI_OperaSingerTouch();
extern void AI_WisemanTouch();
extern void AI_WizardTouch();
extern void CheckAButton();
extern int CheckEarthlingOverlap();
extern void DamageEarthling();
extern void DoVikingAbility();
extern void DropPresentsAbility();
extern int FindSpawnLocationForEarthling();
extern void LoadInEarthlings();
extern int NiceEarthlingCheck();
extern void OpenEarthlingPresent();
extern void ProcessEarthlings();
extern void SantaLogic();
extern void SetEarthlingState();
extern void SetupInitialEarthlings();
extern void func_0174b6();
extern int func_01771a();
extern void func_017a94();
extern void func_017ac8();
extern void func_01804e();
extern void func_0180fe();
extern void func_01814c();
extern void func_018834();
extern void func_0188ee();
extern void func_018a6c();
extern void func_018c74();
extern int func_018ce2();
extern void func_018da4();
extern void func_018f1e();
extern void func_019086();
extern int func_01916c();
extern void func_0191d2();
extern int func_0193fa();
extern void func_01942c();
extern void func_01948a();
extern char func_0196b0();
extern int func_01975c();
extern char func_019d3e();
extern void func_019e12();
extern void func_019f0e();
extern void func_01a3a4();
extern void func_01a722();
extern void func_01a7f0();
extern void func_01a8a0();
extern void func_020f6c();
extern int func_021c60();
extern void func_021d32();
extern void func_02234c();
extern void func_0223f6();

#endif
