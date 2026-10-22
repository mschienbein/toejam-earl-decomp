#ifndef TJE_SPRITE_CALLS_H
#define TJE_SPRITE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void ClearLOC_00FFB8DA();
extern void OpeningPresents7();
extern void Process2FEntityBit0();
extern void Process2FEntityBit6();
extern void Process2FEntityFlag();
extern void SetRAMIndexes();
extern void SetSprite();
extern void SpriteLoadingStuff();
extern void SpriteUnpackInterleavedRLE();
extern void SpriteUnpackRLE();
#ifndef TJE_LOCAL_FUNC_00BDD8
extern short func_00bdd8();
#endif
extern void func_00beb2();
extern void func_00bfc6();
extern void func_00c170();
extern void func_00c242();
extern void func_00c4fa();
extern void func_00c6b0();
extern int func_00c7a8();
extern short func_00c85a();
extern void func_00c8ec();
extern void func_00c920();
extern void func_00cb54();
extern void func_00cc82();
extern void func_00cfba();
extern void func_00cffa();
extern void func_00d00e();
extern void func_00d022();
extern void func_00d04e();
extern void func_00d07a();
extern void func_00d152();
extern int func_00d406();
extern char func_00d586();
extern void func_00d5c0();
extern void func_00d5fa();
extern int func_00d612();
extern short func_00d630();
extern void func_00d6b2();
extern void func_00d776();
extern void func_00d8f2();
#ifndef TJE_LOCAL_FUNC_00D904
extern short func_00d904();
#endif
extern void func_00d970();

#endif
