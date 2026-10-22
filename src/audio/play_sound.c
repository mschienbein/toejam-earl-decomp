#include "ram_sound.h"
#include "ram_speech.h"

/* Queue a sound index when its priority can replace the active speech/sample.
 *
 * MATCHING QUALIFICATION: the discarded sizeof expression has no gameplay
 * effect. MCC68K 3.3K observes the unevaluated address use and leaves
 * sound_number in its incoming stack slot. Removing it changes the matching
 * code. This is compiler coaxing, not a claim of historical source spelling.
 * No alternate compiler profile or argument-layout fiction is required.
 * No natural spelling tried matched with the pinned MCC68K 3.3K compiler.
 */
void PlaySound(sound_number, priority)
short sound_number, priority;
{
    sizeof(&sound_number);

    if (!(gPcmEffectCooldownTicks || gMusicUnknownE46E) ||
        priority >= ram_ffe484) {
        gMusicUnknownE47E = sound_number;
        ram_ffe484 = priority;
    }
}
