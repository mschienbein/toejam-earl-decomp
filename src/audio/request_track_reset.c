#include "audio_calls.h"
#include "ram_system.h"

/* Request the six music pointer slots to be cleared by the driver. */
void MusicRequestTrackPointerReset()
{
    gMusicUnknownE47A = 1;
}
