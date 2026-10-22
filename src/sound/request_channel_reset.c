#include "audio_calls.h"
#include "ram_system.h"

/* Request the six FM channels to be stopped by the driver. */
void AudioRequestFmChannelRelease()
{
    gMusicUnknownE478 = 1;
}
