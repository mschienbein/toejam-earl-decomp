#include "entities.h"
#include "ram.h"
#include "player.h"
/* Check the per-player exclusion byte and the allowed sprite/state masks. */
int func_01b0aa(player, player_number)
struct object_state *player;
short player_number;
{
    if (!ram_ffe236[player_number] &&
        (!player->sprite_state || player->sprite_state == 4 ||
         (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
         (player->flags_52 & 0x830409c)))
        return 1;
    return 0;
}
