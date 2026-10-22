#ifndef TJE_AUDIO_CALLS_H
#define TJE_AUDIO_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void LoadSoundDriverToZ80();
extern void MassiveRoutine();
extern void PlaySound();
extern void PlaySoundEffect();
extern void RAMWritesRoutine1();
extern void func_03e40e();
extern void func_03e418();
extern void func_03e43c();
extern void func_03e446();
extern void func_03e46a();
extern void func_03e47e();
extern void func_03e4f8();

#endif
