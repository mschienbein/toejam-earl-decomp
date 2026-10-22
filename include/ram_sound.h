#ifndef TJE_RAM_SOUND_H
#define TJE_RAM_SOUND_H

#include "sound_types.h"

/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern struct music_channel_reset gMusicChannels[6];
extern char gMusicFmChannelOwners[];
extern short gPsgEffectStatus;
extern short gPcmBlocksSequencer;
extern short gPendingPsgEffect;
extern long gPendingPcmSample;
extern short gSoundEffectPriority;
extern short gMusicTemplateRestartState;

#endif
