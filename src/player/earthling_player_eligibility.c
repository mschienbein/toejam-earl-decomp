#include "entities.h"
#include "player.h"
#include "ram_ui.h"

/* Check the per-player exclusion byte and the allowed sprite/state masks. */
int PlayerCanReceiveEarthlingEffect(player, player_number)
struct object_state *player;
short player_number;
{
    if (!gPlayerInteractionChoiceActive[player_number] &&
        (!player->sprite_state || player->sprite_state == 4 ||
         (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
         (player->flags_52 & 0x830409c)))
        return 1;
    return 0;
}
