#ifndef TJE_TITLE_CALLS_H
#define TJE_TITLE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void SEGAAndIntroVideoRoutine();
extern void SEGAVideo();
extern void SegaLogoVBlank();
extern unsigned short TitleScreenVideo();
extern void func_0272a0();
extern void func_0273ae();
extern void func_02743a();
extern void func_02747c();
extern void func_0274be();
extern void func_0275e6();
extern void func_0276f4();
extern void func_0277b8();
extern void func_02782e();
extern void func_0278ae();
extern void func_0278e8();
extern int func_02791e();
extern void func_03a48a();
extern void func_03a70e();
extern void func_03a784();

#endif
