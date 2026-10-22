#include "hardware.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Terminate the sprite chain beginning at VRAM address 0xf802. */
void TerminateSpriteChainAtHead()
{
    VDP_COMMAND = 0x78020003L;
    VDP_DATA = 79;
}

/* Terminate the primary viewport chain. */
void TerminatePrimarySpriteChain()
{
    VDP_COMMAND = 0x78ba0003L;
    VDP_DATA = 79;
}

/* Enable the primary viewport's chain and terminate the secondary one. */
void SelectPrimaryViewportSprites()
{
    VDP_COMMAND = 0x78c20003L;
    VDP_DATA = 79;
    VDP_COMMAND = 0x78ba0003L;
    VDP_DATA = (char)gPrimaryViewportUploadedLink;
}

/* Link the two viewport sentinels and enable the secondary chain. */
void SelectSecondaryViewportSprites()
{
    VDP_COMMAND = 0x78ba0003L;
    VDP_DATA = 24;
    VDP_COMMAND = 0x78c20003L;
    VDP_DATA = (char)gSecondaryViewportUploadedLink;
}
