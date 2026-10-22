#ifndef TJE_AUDIO_CALLS_H
#define TJE_AUDIO_CALLS_H

/* ROM-only audio/driver entries, including the 03e3f4/03e4xx control bank.
 * These retain established caller interfaces, not proven historical C return
 * spellings; the sound driver's implementation is not represented C here. */
extern void LoadSoundDriverToZ80();
extern void MassiveRoutine();
extern void PlaySound();
extern void PlaySoundEffect();
extern void RAMWritesRoutine1();
extern void func_03e40e();
extern void func_03e43c();
extern void func_03e46a();
extern void func_03e4f8();

#endif
