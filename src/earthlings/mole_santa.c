#include "player_animation.h"
#include "player_animation_groups.h"
#include "psg_effects.h"
#include "hardware.h"
#define TJE_LOCAL_ISENTITYVISIBLEINVIEWPORT

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_earthlings.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_system.h"
#include "random.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: the explicit (int)AllocateEntity() result view
 * retains the observed call-result width with the shared short declaration.
 * Exact historical spelling remains unknown [INFERENCE]. */

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int IsEntityVisibleInViewport();

/* Allocate a six-slot effect and copy its position and level from the mole. */
void EarthlingSpawnMoleEffect(earthling, mole)
struct earthling_state *earthling;
struct object_state *mole;
{
    short slot;
    struct object_state *effect;
    struct animation_descriptor *animation;

    for (slot = 0; slot < 6; ++slot) {
        if (!gMoleEffectEntities[slot])
            break;
    }
    if (slot == 6)
        return;
    if (gMoleEffectEntities[slot] = (int)AllocateEntity()) {
        effect = &gPlayerInfo[gMoleEffectEntities[slot]];
        effect->x = mole->x;
        effect->y = mole->y;
        effect->level = mole->level;
        effect->width = 16;
        effect->height_pixels = 8;
        effect->sprite_count = 1;
        effect->tile_count = 8;
        effect->tile = VDP_TILE_PRIORITY;
        animation = &MoleEffectAnimations[0];
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = 6;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->draw_flags |= ENTITY_DRAW_LATE;
    }
}

/* Release completed mole effects and presents whose height reaches zero. */
void ReleaseFinishedMoleEffectsAndStolenPresents()
{
    short slot, object_number;

    for (slot = 0; slot < 6; ++slot) {
        if ((object_number = gMoleEffectEntities[slot]) &&
            !gPlayerInfo[object_number].frame_duration) {
            FreeEntity(object_number);
            gMoleEffectEntities[slot] = 0;
        }
    }
    for (slot = 0; slot <= 1; ++slot) {
        if ((object_number = gPlayerStolenPresentEntities[slot]) &&
            gPlayerInfo[object_number].height <= 0) {
            FreeEntity(object_number);
            gPlayerStolenPresentEntities[slot] = 0;
        }
    }
}

/* Choose Santa's alert, flight, and idle states from both players. */
void SantaLogic(earthling)
struct earthling_state *earthling;
{
    struct object_state *santa, *toejam, *earl;
    short next_state = -1;

    if (!earthling->object)
        return;
    santa = &gPlayerInfo[earthling->object];
    toejam = &gPlayerInfo[PLAYER_TOEJAM];
    earl = &gPlayerInfo[PLAYER_EARL];
    if (earthling->state == 3 || earthling->state == 2) {
        if (!santa->frame_duration) {
            santa->acceleration_height = 16;
            next_state = 5;
            santa->draw_flags |= ENTITY_SHADOW;
            PlaySound(PSG_START_BACKGROUND_NOISE, 94);
        }
    } else if ((earthling->state & 252) == 4) {
        if (santa->height > 20 && !santa->velocity_x && !santa->velocity_y) {
            next_state = (gCachedRandomValues[gRandomCacheIndex == 58 ?
                (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 3) + 4;
            switch (next_state) {
            case 5:
                next_state = 4;
            case 4:
                santa->velocity_y = -256;
                santa->acceleration_y = -32;
                break;
            case 6:
                santa->velocity_x = -256;
                santa->acceleration_x = -32;
                break;
            case 7:
                santa->velocity_x = 256;
                santa->acceleration_x = 32;
                break;
            }
            if (santa->velocity_y) {
                if (gCachedRandomValues[gRandomCacheIndex == 58 ?
                    (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1) {
                    santa->velocity_x = -256;
                    santa->acceleration_x = -16;
                } else {
                    santa->velocity_x = 256;
                    santa->acceleration_x = 16;
                }
            } else
                santa->velocity_y = GetRandomNumber() % 129 + -64L;
        }
        if (santa->height > 30) {
            santa->acceleration_height = 0;
            santa->velocity_height = 0;
        }
        {
            /* MATCHING QUALIFICATION: the scoped flags assignment and
             * unsigned mask preserve the byte-promotion copy and scratch
             * allocation. The earlier random range also retains + -64L:
             * subtraction by 64 changes the code in both revisions.
             * These are compiler-sensitive spellings, not recovered
             * historical expressions [INFERENCE]. */
            int flags;
            if (!((flags = santa->draw_flags) & (unsigned long)6)) {
                do {
                    EarthlingRelocateRecord(earthling);
                    santa->x = earthling->x;
                    santa->y = earthling->y;
                } while (IsEntityVisibleInViewport(santa, 0) || IsEntityVisibleInViewport(santa, 1));
                if (!(gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & PLAYER_GROUP_ALL_SKATES) &&
                    !(gPlayerInfo[PLAYER_EARL].animation_group_mask & PLAYER_GROUP_ALL_SKATES))
                    PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
            }
        }
    } else if (earthling->state == 1) {
        if (santa->frame > 2 && santa->frame < 5) {
            if ((EntityIsWithinRange(santa, toejam, 125, 75) &&
                 toejam->sprite_state && toejam->sprite_state != PLAYER_ANIM_ICARUS_IDLE &&
                 !(toejam->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) && toejam->sprite_state != PLAYER_ANIM_INNERTUBE_IDLE) ||
                (EntityIsWithinRange(santa, earl, 150, 75) &&
                 earl->sprite_state && earl->sprite_state != PLAYER_ANIM_ICARUS_IDLE &&
                 !(earl->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) && earl->sprite_state != PLAYER_ANIM_INNERTUBE_IDLE))
                next_state = 2;
        } else if (!santa->frame_duration)
            next_state = 0;
    } else if (!earthling->state) {
        if (EntityIsWithinRange(santa, toejam, 125, 75)) {
            if (gPlayerBurpsLeft[0])
                next_state = 2;
            else if (toejam->animation_group_mask & PLAYER_GROUP_SNEAK) {
                if (GetRandomNumber() % 100 < 10)
                    next_state = 1;
            } else if (toejam->sprite_state && toejam->sprite_state != PLAYER_ANIM_ICARUS_IDLE &&
                       !(toejam->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) &&
                       toejam->sprite_state != PLAYER_ANIM_INNERTUBE_IDLE)
                next_state = 2;
        }
        if (next_state != 2 && EntityIsWithinRange(santa, earl, 125, 75)) {
            if (gPlayer2BurpsLeft)
                next_state = 2;
            else if (earl->animation_group_mask & PLAYER_GROUP_SNEAK) {
                if (GetRandomNumber() % 100 < 10)
                    next_state = 1;
            } else if (earl->sprite_state && earl->sprite_state != PLAYER_ANIM_ICARUS_IDLE &&
                       !(earl->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) &&
                       earl->sprite_state != PLAYER_ANIM_INNERTUBE_IDLE)
                next_state = 2;
        }
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        santa->frame = -1;
    }
}

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
            nearby_land = &gPlayer1NearbyTerrain[0][0];
        else
            nearby_land = &gPlayer2NearbyTerrain[0][0];
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
