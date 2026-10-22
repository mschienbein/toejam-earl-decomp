#include "audio_calls.h"
#include "ram_system.h"

/* Queue a channel-template effect number for the next driver update. */
void AudioRequestChannelTemplateEffect(effect_number)
short effect_number;
{
    gPendingFmEffect = effect_number;
}
