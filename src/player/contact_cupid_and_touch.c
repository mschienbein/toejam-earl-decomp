#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "earthling_types.h"
#include "terrain.h"
#include "entities.h"
#include "earthlings.h"
#include "assets_audio.h"
#include "assets_earthlings.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

#define NEXT_RANDOM PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex]

/* Flatten an eligible player, apply direct damage, and play the squish sound. */
void Player_Squish(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if ((earthling->kind != EARTHLING_ICE_CREAM_TRUCK || (earthling->state & 0xfc) == 4) &&
        PlayerCanReceiveEarthlingEffect(player, player_number)) {
        player->animation_motion_axes = ANIMATION_MOTION_XY;
        SetSprite(player, PLAYER_ANIM_FLATTENED);
        player->draw_flags |= ENTITY_DRAW_LATE;
        if (gPlayerTerrainSinking[player_number])
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
    if (PlayerCanReceiveEarthlingEffect(player, player_number) &&
        !(player->animation_group_mask & (PLAYER_GROUP_LEAN | PLAYER_GROUP_STATIONARY)) &&
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
            if ((PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
                (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 3) == 3) {
                sprite = PLAYER_ANIM_HULA;
                HULA_DANCE[player_number] = 28;
                if (distance_x < 20 && distance_y < 15) {
                    if (!gHulaSpeechCooldown) {
                        GenerateSpeech(player_number, "Hubba hubba", 1);
                        PlaySoundEffect((long)Sfx_Aloha, 50, 1);
                        gHulaSpeechCooldown = 128;
                    } else
                        --gHulaSpeechCooldown;
                }
            }
            HULA_DANCE_TIMER[player_number] = (distance >> 5) + 4;
        }
    } else if (player->sprite_state == PLAYER_ANIM_HULA) {
        sprite = GetActivePresentSprite(player);
        HULA_DANCE_TIMER[player_number] = 0;
    }
    if (sprite != -1 && sprite != player->sprite_state) {
        player->animation_motion_axes = ANIMATION_MOTION_XY;
        SetSprite(player, sprite);
    }
}

/* Begin a horizontal earthling attack when its target is over 65 pixels away. */
int EarthlingStartHorizontalAttack(earthling, target)
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

/* Track a nearby player, then put Cupid into its falling attack state when
 * the player's height and horizontal separation enter the observed bounds. */
void CupidAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    short delta_x;
    int exclusion_index;
    struct object_state *player;

    /* Keep the promoted exclusion-array index separate from the bounded
     * object-table index; this reproduces the observed scratch allocation. */
    exclusion_index = player_number;
    if (!gPlayerInteractionChoiceActive[exclusion_index] && earthling->state != 3)
        player = &gPlayerInfo[player_number];
    else
        return;
    if (!(player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
          (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
          player->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE || (player->animation_group_mask & 0x1834c03c)))
        return;
    if ((earthling->state & 0xfc) != 8 && player->height < 40 &&
        abs(player->y - earthling->y) <= 8)
        EarthlingStartHorizontalAttack(earthling, player);
    if (player->height > 10 && player->height < 75) {
        delta_x = player->x - earthling->x;
        if (abs(delta_x) < 25) {
            SetEarthlingState(earthling, 3);
            player = &gPlayerInfo[earthling->object];
            player->frame = -1;
            player->acceleration_height = -32;
            player->velocity_height = -512;
            player->behavior_flags &= ~ENTITY_COLLIDABLE;
        }
    }
}

/* Bounce a falling earthling at ground level, or damage it after its animation
 * ends or its terrain query exceeds 0x50. */
void EarthlingUpdateBouncingDamageState(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    short map_number;

    object = &gPlayerInfo[earthling->object];
    if (!earthling->object) {
        EarthlingRemoveFromLevel(earthling);
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
        if ((unsigned short)GetMapTerrainAtWorldPosition(map_number, object->x, object->y) > 0x50)
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
    if (!PlayerCanReceiveEarthlingEffect(player, player_number) &&
        !(player->animation_group_mask & PLAYER_GROUP_INNERTUBE_MOVE) && player->sprite_state != PLAYER_ANIM_INNERTUBE_WATER_IDLE)
        return;
    SetSprite(player, PLAYER_ANIM_WHIRLWIND);
    player->animation_motion_axes = ANIMATION_MOTION_XY;
    player->x = earthling->x;
    player->y = earthling->y;
    player->height = 0;
    player->velocity_x = 0;
    player->velocity_y = 0;
    player->velocity_height = 0;
    player->acceleration_x = 0;
    player->acceleration_y = 0;
    player->acceleration_height = 0;
    player->draw_flags |= ENTITY_SHADOW;
    gPlayerWhirlwindTimers[player_number] = 225;
    FreeEntity(earthling->object);
    earthling->object = 0;
    earthling->kind = EARTHLING_UNUSED;
}

/* Sprite 78 counts down to restoring the active present or a terrain pose.
 * Sprite 79 wanders in timed directions until its movement timer expires. */
void PlayerUpdateWhirlwindAndRecovery(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    char sprite_or_direction = -1;
    short player_number;
    int quarter_velocity;

    player_number = player->entity_index;
    if (player->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY) {
        if (--gPlayerWhirlwindRecoveryTimers[player_number] <= 0) {
            if (gPlayer1Powerup[player_number] == PRESENT_ROCKET_SKATES)
                sprite_or_direction = GetActivePresentSprite(player);
            else if (!gPlayer1Powerup[player_number]) {
                sprite_or_direction = 61;
                player->velocity_height = -1;
                player->animation_motion_axes = ANIMATION_MOTION_NONE;
            } else if (terrain >= TERRAIN_WATER) {
                if (terrain == TERRAIN_VOID)
                    PlayerStartFalling(player, player_number);
                else
                    sprite_or_direction = 65;
            } else {
                sprite_or_direction = GetActivePresentSprite(player);
                player->animation_motion_axes = ANIMATION_MOTION_XY;
                if ((NEXT_RANDOM & 3) == 1)
                    GenerateSpeech(player_number, "I feel sick", 1);
            }
            if (sprite_or_direction != -1)
                SetSprite(player, sprite_or_direction);
        } else if (!(gPlayerWhirlwindRecoveryTimers[player_number] & 15))
            ++player->frame_phase;
    } else if (player->sprite_state == PLAYER_ANIM_WHIRLWIND) {
        if (--gPlayerWhirlwindTimers[player_number] <= 0) {
            player->velocity_x = 0;
            player->velocity_y = 0;
            SetSprite(player, PLAYER_ANIM_WHIRLWIND_RECOVERY);
            gPlayerWhirlwindRecoveryTimers[player_number] = 48;
            player->draw_flags &= ~ENTITY_SHADOW;
        } else {
            if (--gPlayerWhirlwindDirectionTimers[player_number] <= 0) {
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
                gPlayerWhirlwindDirectionTimers[player_number] = 20 + NEXT_RANDOM % 6;
            }
            EntityUpdateTerrainDrawFlag(player);
        }
    }
}

/* Test active decoy objects against the earthling definition's contact ranges,
 * stopping when an interaction consumes one of those contacts. */
void EarthlingCheckDecoyAttackRange(earthling, definition)
struct earthling_state *earthling;
struct earthling_definition *definition;
{
    struct object_state *object, *decoy;
    short player_number;

    if ((earthling->state & 252) == 8)
        return;
    object = &gPlayerInfo[earthling->object];
    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        if (gPlayerDecoyEntities[player_number]) {
            decoy = &gPlayerInfo[gPlayerDecoyEntities[player_number]];
            if (EntityIsWithinRange(object, decoy, definition->contact_distance_x,
                           definition->contact_distance_y) &&
                EarthlingStartHorizontalAttack(earthling, decoy))
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
        !PlayerCanReceiveEarthlingEffect(player, player_number))
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
