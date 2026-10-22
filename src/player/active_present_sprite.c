#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "entities.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"

/* Select the player's idle sprite from the active present and terrain,
 * resetting the animation and horizontal velocity for a new selection. */
int GetActivePresentSprite(player)
struct object_state *player;
{
    register unsigned short land;
    short player_number;
    char sprite;

    player_number = player->entity_index;
    land = player_number == PLAYER_TOEJAM ? LAND_UNDER_TOEJAM : LAND_UNDER_EARL;
    land &= TERRAIN_CLASS_MASK;
    if (gPlayer1Powerup[player_number] == PRESENT_INNERTUBE) {
        if (land == TERRAIN_WATER)
            sprite = PLAYER_ANIM_INNERTUBE_WATER_IDLE;
        else if (!ELEVATOR_FLAG[player_number])
            sprite = PLAYER_ANIM_INNERTUBE_IDLE;
        else {
            sprite = PLAYER_ANIM_IDLE;
            player->frame = -1;
        }
    } else if (gPlayer1Powerup[player_number] == PRESENT_ROCKET_SKATES) {
        OpenRocketSkates(player, player_number, land);
        sprite = -1;
        return sprite;
    } else if (land == TERRAIN_WATER && !player->height)
        sprite = PLAYER_ANIM_SWIM_IDLE;
    else if (gPlayer1Powerup[player_number] == PRESENT_SPRING_SHOES && !ELEVATOR_FLAG[player_number])
        sprite = PLAYER_ANIM_SPRING_SHOES_DOWN;
    else if (!gPlayer1Powerup[player_number] && !ELEVATOR_FLAG[player_number]) {
        if ((player->animation_group_mask & PLAYER_GROUP_ICARUS_FLY) && player->height > 0) {
            sprite = -1;
            return sprite;
        } else {
            player->draw_flags |= ENTITY_SHADOW;
            sprite = PLAYER_ANIM_ICARUS_IDLE;
        }
    } else {
        sprite = PLAYER_ANIM_IDLE;
        player->frame = -1;
    }
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite != 4)
        player->draw_flags &= ~ENTITY_SHADOW;
    return sprite;
}
