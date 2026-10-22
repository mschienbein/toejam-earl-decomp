#include "audio_calls.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "ram_cutscene.h"
#include "ram_system.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Upload a queued logo palette and sprites, service sound, and count frames. */
void SegaLogoVBlank()
{
    if (SEGA_PAL_LOAD_FLAG) {
        SEGA_PAL_LOAD_FLAG = 0;
        CopyPaletteToCRAMSlot(SEGA_PAL_TO_LOAD, 3);
    }
    if (gSegaVBlankSpriteUpdatesEnabled) {
        SpriteLoadingStuff(0, 0);
        SelectViewportSpriteChain(0);
    }
    MassiveRoutine();
    ++TIME_COUNTER;
}
