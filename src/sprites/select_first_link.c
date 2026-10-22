#include "hardware.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Change the first sprite's link in VRAM and its RAM mirror. */
void SelectSpriteChainStart(primary_only)
short primary_only;
{
    if (primary_only) {
        VDP_COMMAND = 0x78020003L;
        VDP_DATA = 1;
        gHardwareSpriteRootLink = 1;
    } else {
        VDP_COMMAND = 0x78020003L;
        VDP_DATA = 11;
        gHardwareSpriteRootLink = 11;
    }
}
