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
void func_01befa(player, player_number, powerup)
struct object_state *player;
short player_number, powerup;
{
    short slot, direction, facing, powerup_index, speed_x, speed_y;
    int quarter_velocity;
    struct object_state *projectile;
    struct animation_descriptor *animation;

    slot = 0;
    do {
        if (!ram_ffe1c0[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4)
        return;
    if (ram_ffe1c0[slot] = (int)func_00d904()) {
        ram_ffe1cc[slot] = player_number;
        projectile = &gPlayerInfo[ram_ffe1c0[slot]];
        direction = player->sprite_state & 3;
        powerup_index = powerup;
        powerup_index -= 3;
        projectile->x = player->x + ProjectilePlacementTriples[powerup_index][player_number][direction][0];
        projectile->y = player->y + ProjectilePlacementTriples[powerup_index][player_number][direction][1];
        projectile->height = player->height + ProjectilePlacementTriples[powerup_index][player_number][direction][2];
        /* The facing index is now replaced by a movement-direction bitmask. */
        direction = P1_PROCESSED_INPUT[player_number];
        direction &= 15;
        facing = player->sprite_state & 3;
        if (facing == 0 && !(direction & 1))
            direction = 1;
        else if (facing == 1 && !(direction & 2))
            direction = 2;
        else if (facing == 2 && !(direction & 4))
            direction = 4;
        else if (facing == 3 && !(direction & 8))
            direction = 8;
        if (powerup == 3) {
            speed_x = 896;
            speed_y = 512;
            projectile->velocity_height = 160;
            projectile->acceleration_height = -32;
        } else {
            speed_x = 1152;
            speed_y = 768;
            projectile->velocity_height = 64;
            projectile->acceleration_height = -8;
            func_03e4f8(11);
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
        projectile->tile = 0x8000;
        animation = AnimationDescriptors_0AEFF6;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= 64;
        projectile->sprite_state = -1;
    }
}

/* Shooting animation frames spawn one projectile. The last block selects
 * a shooting pose from held input, outside the elevator. */
void func_01c188(player, unused)
struct object_state *player;
short unused;
{
    short player_number;
    char shooting_sprite, direction;

    player_number = player->entity_number;
    if (player->flags_52 & 0x100000L) {
        if (player->frame == 1) {
            func_01befa(player, player_number, 3);
            ++player->frame;
        } else if (!player->frame_duration)
            SetSprite(player, GetActivePresentSprite(player));
    } else if (player->flags_52 & 0x200000L) {
        if (player->frame == 2) {
            func_01befa(player, player_number, 4);
            ++player->frame;
        } else if (!player->frame_duration)
            SetSprite(player, GetActivePresentSprite(player));
    }
    shooting_sprite = -1;
    if (!ELEVATOR_FLAG[player_number] && (player->flags_52 & 4) &&
        (P1_PROCESSED_INPUT[player_number] & 64) &&
        (P1_PROCESSED_INPUT[player_number] & 15)) {
        direction = player->sprite_state & 3;
        if (gPlayer1Powerup[player_number] == 3)
            shooting_sprite = direction + 80;
        if (gPlayer1Powerup[player_number] == 4)
            shooting_sprite = direction + 84;
        if (shooting_sprite != -1)
            SetSprite(player, shooting_sprite);
    }
}
