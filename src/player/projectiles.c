#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "fm_effects.h"
#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_presents.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Find an unused projectile slot, then use the player's facing, input, and
 * selected powerup to set its origin and launch velocity. */
void PlayerLaunchPresentProjectile(player, player_number, powerup)
struct object_state *player;
short player_number, powerup;
{
    short slot, direction, facing, powerup_index, speed_x, speed_y;
    int quarter_velocity;
    struct object_state *projectile;
    struct animation_descriptor *animation;

    slot = 0;
    do {
        if (!gSharedProjectileEntities[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4)
        return;
    if (gSharedProjectileEntities[slot] = (int)AllocateEntity()) {
        gSharedProjectileOwners[slot] = player_number;
        projectile = &gPlayerInfo[gSharedProjectileEntities[slot]];
        direction = player->sprite_state & DIRECTION_MASK;
        powerup_index = powerup;
        powerup_index -= 3;
        projectile->x = player->x + ProjectilePlacementTriples[powerup_index][player_number][direction][0];
        projectile->y = player->y + ProjectilePlacementTriples[powerup_index][player_number][direction][1];
        projectile->height = player->height + ProjectilePlacementTriples[powerup_index][player_number][direction][2];
        /* The facing index is now replaced by a movement-direction bitmask. */
        direction = P1_PROCESSED_INPUT[player_number];
        direction &= 15;
        facing = player->sprite_state & DIRECTION_MASK;
        if (facing == DIRECTION_UP && !(direction & 1))
            direction = 1;
        else if (facing == DIRECTION_DOWN && !(direction & 2))
            direction = 2;
        else if (facing == DIRECTION_LEFT && !(direction & 4))
            direction = 4;
        else if (facing == DIRECTION_RIGHT && !(direction & 8))
            direction = 8;
        if (powerup == PRESENT_TOMATOES) {
            speed_x = 896;
            speed_y = 512;
            projectile->velocity_height = 160;
            projectile->acceleration_height = -32;
        } else {
            speed_x = 1152;
            speed_y = 768;
            projectile->velocity_height = 64;
            projectile->acceleration_height = -8;
            AudioRequestChannelTemplateEffect(FM_EFFECT_SLING_SHOT);
        }
        if (direction & 1)
            projectile->velocity_y = -speed_y;
        else if (direction & 2)
            projectile->velocity_y = speed_y;
        if (direction & 4)
            projectile->velocity_x = -speed_x;
        else if (direction & 8)
            projectile->velocity_x = speed_x;
        if (projectile->velocity_x && projectile->velocity_y) {
            quarter_velocity = projectile->velocity_x >> 2;
            projectile->velocity_x = quarter_velocity * 3;
            quarter_velocity = projectile->velocity_y >> 2;
            projectile->velocity_y = quarter_velocity * 3;
        }
        projectile->level = player->level;
        projectile->width = 9;
        projectile->height_pixels = 7;
        projectile->sprite_count = 1;
        projectile->tile_count = 16;
        projectile->tile = VDP_TILE_PRIORITY;
        animation = AnimationDescriptors_0AEFF6;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= ENTITY_SHADOW;
        projectile->sprite_state = -1;
    }
}

/* Shooting animation frames spawn one projectile. The last block selects
 * a shooting pose from held input, outside the elevator. */
void PlayerUpdatePresentShooting(player, unused)
struct object_state *player;
short unused;
{
    short player_number;
    char shooting_sprite, direction;

    player_number = player->entity_index;
    if (player->animation_group_mask & (long)PLAYER_GROUP_THROW_TOMATO) {
        if (player->frame == 1) {
            PlayerLaunchPresentProjectile(player, player_number, PRESENT_TOMATOES);
            ++player->frame;
        } else if (!player->frame_duration)
            SetSprite(player, GetActivePresentSprite(player));
    } else if (player->animation_group_mask & (long)PLAYER_GROUP_SLING_SHOT) {
        if (player->frame == 2) {
            PlayerLaunchPresentProjectile(player, player_number, PRESENT_SLING_SHOT);
            ++player->frame;
        } else if (!player->frame_duration)
            SetSprite(player, GetActivePresentSprite(player));
    }
    shooting_sprite = -1;
    if (!ELEVATOR_FLAG[player_number] && (player->animation_group_mask & PLAYER_GROUP_WALK) &&
        (P1_PROCESSED_INPUT[player_number] & 64) &&
        (P1_PROCESSED_INPUT[player_number] & 15)) {
        direction = player->sprite_state & DIRECTION_MASK;
        if (gPlayer1Powerup[player_number] == PRESENT_TOMATOES)
            shooting_sprite = direction + PLAYER_ANIM_THROW_TOMATO_UP;
        if (gPlayer1Powerup[player_number] == PRESENT_SLING_SHOT)
            shooting_sprite = direction + PLAYER_ANIM_SLING_SHOT_UP;
        if (shooting_sprite != -1)
            SetSprite(player, shooting_sprite);
    }
}
