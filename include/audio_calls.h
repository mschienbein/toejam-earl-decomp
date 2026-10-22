#ifndef TJE_AUDIO_CALLS_H
#define TJE_AUDIO_CALLS_H

/* PlaySound/PlaySoundEffect preempt requests of equal or lower priority.
 * Fifty is the common request level in the represented callers. */
#define AUDIO_PRIORITY_NORMAL 50

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void LoadSoundDriverToZ80();
extern void MusicUpdateSequencer();
extern void PlaySound();
extern void PlaySoundEffect();
extern void MusicRestoreInitialState();
extern void MusicRequestTrackPointerReset();
extern void MusicClearTrackPointers();
extern void AudioRequestFmChannelRelease();
extern void AudioReleaseAllFmChannels();
extern void MusicSelectSongTemplateBank();
extern void MusicLoadSongChannelTemplates();
extern void AudioRequestChannelTemplateEffect();

#endif
