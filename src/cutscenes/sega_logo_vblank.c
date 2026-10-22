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
    if (gSegaPaletteUploadPending) {
        gSegaPaletteUploadPending = 0;
        CopyPaletteToCRAMSlot(gSegaPaletteUploadSource, 3);
    }
    if (gSegaVBlankSpriteUpdatesEnabled) {
        UploadQueuedSpriteGraphics(0, 0);
        SelectViewportSpriteChain(0);
    }
    MusicUpdateSequencer();
    ++gFrameCounter;
}
