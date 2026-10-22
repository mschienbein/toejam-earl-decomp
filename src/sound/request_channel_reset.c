#include "audio_calls.h"
#include "ram_system.h"

/* Request the six FM channels to be stopped by the driver. */
void func_03e43c()
{
    gMusicUnknownE478 = 1;
}
