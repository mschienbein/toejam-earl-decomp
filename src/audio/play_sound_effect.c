#include "ram_sound.h"
#include "ram_speech.h"
#include "audio_calls.h"

/* Queue a sample when its priority can replace the active speech/sample.
 *
 * MATCHING QUALIFICATION: the two discarded sizeof expressions have no
 * gameplay effect. MCC68K 3.3K nevertheless observes these unevaluated address
 * uses and leaves sample and stop_channels in their incoming stack slots.
 * Removing either expression changes the matching code. This is compiler
 * coaxing, not a claim that these expressions appeared in the original source.
 * The parameter types and scalar calling convention are unchanged.
 * No natural spelling tried matched with the pinned MCC68K 3.3K compiler.
 */
void PlaySoundEffect(sample, priority, stop_channels)
long sample;
short priority, stop_channels;
{
    sizeof(&sample);
    sizeof(&stop_channels);

    if (!(gPcmEffectCooldownTicks || gPsgEffectStatus) ||
        priority >= gSoundEffectPriority) {
        gPendingPcmSample = sample;
        gSoundEffectPriority = priority;
        if (stop_channels) {
            gPcmBlocksSequencer = 1;
            AudioRequestFmChannelRelease();
        }
    }
}
