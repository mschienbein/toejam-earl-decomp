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

    player_number = player->entity_number;
    land = player_number == 0 ? LAND_UNDER_TOEJAM : LAND_UNDER_EARL;
    land &= 0xf0;
    if (gPlayer1Powerup[player_number] == 2) {
        if (land == 0x80)
            sprite = 77;
        else if (!ELEVATOR_FLAG[player_number])
            sprite = 76;
        else {
            sprite = 0;
            player->frame = -1;
        }
    } else if (gPlayer1Powerup[player_number] == 5) {
        OpenRocketSkates(player, player_number, land);
        sprite = -1;
        return sprite;
    } else if (land == 0x80 && !player->height)
        sprite = 3;
    else if (gPlayer1Powerup[player_number] == 1 && !ELEVATOR_FLAG[player_number])
        sprite = 57;
    else if (!gPlayer1Powerup[player_number] && !ELEVATOR_FLAG[player_number]) {
        if ((player->flags_52 & 0x8000) && player->height > 0) {
            sprite = -1;
            return sprite;
        } else {
            player->draw_flags |= 0x40;
            sprite = 4;
        }
    } else {
        sprite = 0;
        player->frame = -1;
    }
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite != 4)
        player->draw_flags &= ~0x40;
    return sprite;
}
