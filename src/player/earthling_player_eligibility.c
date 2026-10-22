#include "player_animation.h"
#include "player_animation_groups.h"
#include "entities.h"
#include "player.h"
#include "ram_ui.h"

/* Check the per-player exclusion byte and the allowed sprite/state masks. */
int PlayerCanReceiveEarthlingEffect(player, player_number)
struct object_state *player;
short player_number;
{
    if (!gPlayerInteractionChoiceActive[player_number] &&
        (!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
         (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
         (player->animation_group_mask & 0x830409c)))
        return 1;
    return 0;
}
