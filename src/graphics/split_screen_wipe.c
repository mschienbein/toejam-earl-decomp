#include "entities.h"
#include "hardware.h"
#include "core_calls.h"
#include "graphics_calls.h"
#include "map_calls.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Ensure both primary sprites are resident, wipe paired rows inward, then
 * fill the 28-by-40 destination and write 0x1c to VDP register 18. */
void AnimateSplitScreenWipe()
{
    short pass, step, lower_row, upper_row, column;
    struct object_state *entity;

    entity = &gPlayerInfo[0];
    for (step = 0; step < 2; ++step, ++entity) {
        while (!(entity->draw_flags & 2)) {
            entity->sprite_ready = 0;
            AllocateEntityViewportSprites(entity, 0);
            if (!(entity->draw_flags & 2))
                EntityReleaseOneByPriority();
        }
    }
    while (!gPlayerInfo[0].sprite_ready || !gPlayerInfo[1].sprite_ready) {
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(1);
        SpriteLoadingStuff(0, 0);
    }
    WaitForNextVBlank(1);
    LinkPlayerSpritesForSplitWipe();
    upper_row = gCamera1TileRowCursor & 31;
    lower_row = (gCamera1TileRowCursor + 29) & 31;
    for (step = 0; step < 15; ++step) {
        WaitForNextVBlank(1);
        VDP_COMMAND = (upper_row << 23) + 0x60000002L;
        for (pass = 0; pass < 2; ++pass) {
            for (column = 0; column < 64; ++column)
                VDP_DATA = 0x25b4;
            VDP_COMMAND = (lower_row << 23) + 0x60000002L;
        }
        if (step == 14)
            break;
        upper_row = (upper_row + 1) & 31;
        lower_row = (lower_row - 1) & 31;
        VDP_COMMAND = (upper_row << 23) + 0x60000002L;
        for (pass = 0; pass < 2; ++pass) {
            for (column = 0; column < 64; ++column)
                VDP_DATA = 0x5ab;
            VDP_COMMAND = (lower_row << 23) + 0x60000002L;
        }
    }
    SelectSpriteChainStart(1);
    for (pass = 0; pass < 28; ++pass) {
        VDP_COMMAND = (pass << 23) + 0x50000003L;
        for (column = 0; column < 40; ++column)
            VDP_DATA = 0xa5b4;
    }
    VDP_CONTROL_WORD = 0x921c;
}
