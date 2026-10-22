#include "entities.h"
#include "earthlings.h"
#include "ram.h"
#include "assets.h"
#include "earthling_calls.h"
#include "player.h"
#include "sprite_calls.h"
#include "romcalls.h"
/* Flatten an eligible player, apply direct damage, and play the squish sound. */
void Player_Squish(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if ((earthling->kind != 19 || (earthling->state & 0xfc) == 4) &&
        func_01b0aa(player, player_number)) {
        player->mode_15 = 3;
        SetSprite(player, 2);
        player->draw_flags |= 128;
        if (ram_ffda24[player_number])
            player->height = -3;
        else
            player->height = 0;
        if (earthling->state)
            earthling->timer = 30;
        TOEJAMS_HEALTH[player_number] -= EARTHLING_DATA[earthling->kind].damage;
        PlaySoundEffect((long)Sfx_Squish, 50, 0);
    }
}

/* Nearby eligible players periodically dance. Close dancing can also trigger
 * the shared speech/sound cooldown; leaving the radius restores their pose. */
void HulaAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char sprite = -1;
    short distance_x, distance_y, distance;
    struct object_state *player, *dancer;

    player = &gPlayerInfo[player_number];
    if (!player->level)
        return;
    if (func_01b0aa(player, player_number) &&
        !(player->flags_52 & 0x20000010) &&
        (dancer = &gPlayerInfo[earthling->object],
         distance_x = abs(dancer->x - player->x),
         distance_y = abs(dancer->y - player->y),
         distance_x <= 100 && distance_y <= 75)) {
        if (--HULA_DANCE_TIMER[player_number] < 0) {
            distance_y += distance_y / 3;
            if (distance_x > distance_y)
                distance = distance_x;
            else
                distance = distance_y;
            if ((PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                (ram_ff91e0 = 0) : ++ram_ff91e0] & 3) == 3) {
                sprite = 89;
                HULA_DANCE[player_number] = 28;
                if (distance_x < 20 && distance_y < 15) {
                    if (!ram_ffe1b4) {
                        GenerateSpeech(player_number, "Hubba hubba", 1);
                        PlaySoundEffect((long)Sfx_Aloha, 50, 1);
                        ram_ffe1b4 = 128;
                    } else
                        --ram_ffe1b4;
                }
            }
            HULA_DANCE_TIMER[player_number] = (distance >> 5) + 4;
        }
    } else if (player->sprite_state == 89) {
        sprite = GetActivePresentSprite(player);
        HULA_DANCE_TIMER[player_number] = 0;
    }
    if (sprite != -1 && sprite != player->sprite_state) {
        player->mode_15 = 3;
        SetSprite(player, sprite);
    }
}

/* Begin a horizontal earthling attack when its target is over 65 pixels away. */
int func_01b5ca(earthling, target)
struct earthling_state *earthling;
struct object_state *target;
{
    short delta_x;
    struct object_state *object;

    delta_x = target->x - earthling->x;
    if (abs(delta_x) > 65) {
        SetEarthlingState(earthling, (short)(delta_x > 0 ? 11 : 10));
        object = &gPlayerInfo[earthling->object];
        object->frame = -1;
        return 1;
    }
    return 0;
}
