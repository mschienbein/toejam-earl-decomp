#include "entities.h"
#include "hardware.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Move the selected active sprite list to hardware Y coordinate one. */
void func_00d152(player)
short player;
{
    struct object_state *entity;
    char *sprite_list;
    short sprite_count;
    char sprite_number;

    entity = &gPlayerInfo[player];
    if (!(entity->flags_4d & 16) || !entity->sprite_ready)
        return;
    if (entity->draw_flags & 2)
        sprite_list = entity->primary_sprites;
    else {
        if (!(entity->draw_flags & 4))
            return;
        sprite_list = entity->secondary_sprites;
    }
    for (sprite_count = entity->sprite_count; sprite_count > 0; --sprite_count) {
        sprite_number = *sprite_list++;
        VDP_COMMAND = (sprite_number << 19) + 0x78000003L;
        VDP_DATA = 1;
    }
}
