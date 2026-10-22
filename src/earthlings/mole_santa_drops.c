#include "entities.h"
#include "earthlings.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "present_calls.h"
#include "ram_player.h"
#include "random.h"

/* Drop two or three presents at Santa, preserving the player's land cache
 * and coordinates while the present-placement helpers use that position. */
void DropPresentsAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    short index, saved_x, saved_y;
    struct object_state *player;
    unsigned char *nearby_land;
    unsigned char saved_land[25];

    if (earthling->state <= 1) {
        SetEarthlingState(earthling, 3);
        gPlayerInfo[earthling->object].frame = -1;
        player = &gPlayerInfo[player_number];
        saved_x = player->x;
        saved_y = player->y;
        player->x = earthling->x;
        player->y = earthling->y;
        if (!player_number)
            nearby_land = &LAND_NEAR_TOEJAM[0][0];
        else
            nearby_land = &LAND_NEAR_EARL[0][0];
        for (index = 0; index < 25; ++index)
            saved_land[index] = nearby_land[index];
        UpdateLandAroundPlayer(player_number);
        index = ((short)GetRandomNumber() & 1) + 2;
        while (index > 0) {
            DropPresent(player_number, SelectRandomPresent());
            --index;
        }
        for (index = 0; index < 25; ++index)
            nearby_land[index] = saved_land[index];
        player->x = saved_x;
        player->y = saved_y;
    }
}
