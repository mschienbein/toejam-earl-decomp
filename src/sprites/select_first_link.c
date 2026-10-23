#include "hardware.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Change the first sprite's link in VRAM and its RAM mirror. */
void SelectSpriteChainStart(primary_only)
short primary_only;
{
    if (primary_only) {
        VDP_COMMAND = 0x78020003L;
        VDP_DATA = SPRITE_CHAIN_PLAYER_HEAD_SLOT;
        gHardwareSpriteRootLink = SPRITE_CHAIN_PLAYER_HEAD_SLOT;
    } else {
        VDP_COMMAND = 0x78020003L;
        VDP_DATA = SPRITE_CHAIN_HUD_HEAD_SLOT;
        gHardwareSpriteRootLink = SPRITE_CHAIN_HUD_HEAD_SLOT;
    }
}
