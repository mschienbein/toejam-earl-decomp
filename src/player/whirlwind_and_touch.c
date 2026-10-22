#define TJE_LOCAL_FUNC_00D9DC

#include "entities.h"
#include "earthlings.h"
#include "ram.h"
#include "assets.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "sprite_calls.h"
#include "romcalls.h"

/* ORIGINAL BUG: this consumer keeps a unsigned short result view of func_00d9dc,
 * whose canonical return is unsigned char. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern unsigned short func_00d9dc();
#define NEXT_RANDOM PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0]

/* Bounce a falling earthling at ground level, or damage it after its animation
 * ends or its terrain query exceeds 0x50. */
void func_01b746(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    short map_number;

    object = &gPlayerInfo[earthling->object];
    if (!earthling->object) {
        func_018da4(earthling);
        return;
    }
    if (!object->frame_duration)
        DamageEarthling(earthling);
    else if (object->height < 3 && object->velocity_height < 0) {
        object->acceleration_height = -64;
        object->velocity_height = 512;
        object->height = 0;
        if (!object->frame)
            object->frame_duration = 1;
        /* ORIGINAL BUG: map_number is unset if neither loaded map has
         * the object's level. The original code makes no fallback choice. */
        if (object->level == gLevel912C)
            map_number = 0;
        else if (object->level == gLevel912E)
            map_number = 1;
        if (func_00d9dc(map_number, object->x, object->y) > 0x50)
            DamageEarthling(earthling);
    }
}

/* A state-4 whirlwind captures an eligible player, clears all movement, and
 * retires its earthling object while the player's 225-tick timer runs. */
void WhirlWindAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    if (earthling->state != 4)
        return;
    player = &gPlayerInfo[player_number];
    if (!func_01b0aa(player, player_number) &&
        !(player->flags_52 & 0x40000) && player->sprite_state != 77)
        return;
    SetSprite(player, 79);
    player->mode_15 = 3;
    player->x = earthling->x;
    player->y = earthling->y;
    player->height = 0;
    player->velocity_x = 0;
    player->velocity_y = 0;
    player->velocity_height = 0;
    player->acceleration_x = 0;
    player->acceleration_y = 0;
    player->acceleration_height = 0;
    player->draw_flags |= 64;
    ram_ffe1b6[player_number] = 225;
    func_00d970(earthling->object);
    earthling->object = 0;
    earthling->kind = -1;
}

/* Sprite 78 counts down to restoring the active present or a terrain pose.
 * Sprite 79 wanders in timed directions until its movement timer expires. */
void func_01b8ba(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    char sprite_or_direction = -1;
    short player_number;
    int quarter_velocity;

    player_number = player->entity_number;
    if (player->sprite_state == 78) {
        if (--ram_ffe1bc[player_number] <= 0) {
            if (gPlayer1Powerup[player_number] == 5)
                sprite_or_direction = GetActivePresentSprite(player);
            else if (!gPlayer1Powerup[player_number]) {
                sprite_or_direction = 61;
                player->velocity_height = -1;
                player->mode_15 = 0;
            } else if (terrain >= 128) {
                if (terrain == 144)
                    SetToejamFalling(player, player_number);
                else
                    sprite_or_direction = 65;
            } else {
                sprite_or_direction = GetActivePresentSprite(player);
                player->mode_15 = 3;
                if ((NEXT_RANDOM & 3) == 1)
                    GenerateSpeech(player_number, "I feel sick", 1);
            }
            if (sprite_or_direction != -1)
                SetSprite(player, sprite_or_direction);
        } else if (!(ram_ffe1bc[player_number] & 15))
            ++player->frame_phase;
    } else if (player->sprite_state == 79) {
        if (--ram_ffe1b6[player_number] <= 0) {
            player->velocity_x = 0;
            player->velocity_y = 0;
            SetSprite(player, 78);
            ram_ffe1bc[player_number] = 48;
            player->draw_flags &= ~64;
        } else {
            if (--ram_ffe1ba[player_number] <= 0) {
                sprite_or_direction = NEXT_RANDOM & 7;
                if (sprite_or_direction < 2 || sprite_or_direction > 7)
                    player->velocity_y = -256;
                else if (sprite_or_direction > 2 && sprite_or_direction < 6)
                    player->velocity_y = 256;
                if (sprite_or_direction > 0 && sprite_or_direction < 4)
                    player->velocity_x = 256;
                else if (sprite_or_direction > 4)
                    player->velocity_x = -256;
                if (sprite_or_direction & 1) {
                    quarter_velocity = player->velocity_x >> 2;
                    player->velocity_x = quarter_velocity * 3;
                    quarter_velocity = player->velocity_y >> 2;
                    player->velocity_y = quarter_velocity * 3;
                }
                ram_ffe1ba[player_number] = 20 + NEXT_RANDOM % 6;
            }
            func_01942c(player);
        }
    }
}

/* Test active decoy objects against the earthling definition's contact ranges,
 * stopping when an interaction consumes one of those contacts. */
void func_01bb62(earthling, definition)
struct earthling_state *earthling;
struct earthling_definition *definition;
{
    struct object_state *object, *decoy;
    short player_number;

    if ((earthling->state & 252) == 8)
        return;
    object = &gPlayerInfo[earthling->object];
    for (player_number = 0; player_number <= 1; ++player_number) {
        if (ram_ffde58[player_number]) {
            decoy = &gPlayerInfo[ram_ffde58[player_number]];
            if (func_00dd1e(object, decoy, definition->contact_distance_x,
                           definition->contact_distance_y) &&
                func_01b5ca(earthling, decoy))
                return;
        }
    }
}

/* Hurt a close player; occasional distant targeting chooses an axis-facing
 * state after the common player eligibility test succeeds. */
void ChickenAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;
    short distance_x, distance_y;
    char next_state;

    player = &gPlayerInfo[player_number];
    distance_x = abs(player->x - earthling->x);
    distance_y = abs(player->y - earthling->y);
    if (distance_x < 25 && distance_y < 15)
        Player_Youch(player_number, earthling);
    if (NEXT_RANDOM % 100 < 92 || (earthling->state & 252) == 8 ||
        (distance_y < 50 && distance_x < 75) ||
        !func_01b0aa(player, player_number))
        return;
    if (distance_x < distance_y) {
        if (player->y < earthling->y)
            next_state = 8;
        else
            next_state = 9;
    } else {
        if (player->x < earthling->x)
            next_state = 10;
        else
            next_state = 11;
    }
    SetEarthlingState(earthling, next_state);
}

/* An unsuccessful touch turns the boogie toward the player and plays its cry. */
void AI_BoogieTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;
    char next_state;

    if ((earthling->state & 252) == 8 || Player_Youch(player_number, earthling))
        return;
    player = &gPlayerInfo[player_number];
    if ((short)abs(player->x - earthling->x) > 8) {
        if (player->x > earthling->x)
            next_state = 11;
        else
            next_state = 10;
    } else {
        if (player->y > earthling->y)
            next_state = 9;
        else
            next_state = 8;
    }
    SetEarthlingState(earthling, next_state);
    PlaySoundEffect((long)Sfx_BoogieBoogie, 50, 0);
}
