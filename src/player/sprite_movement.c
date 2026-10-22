#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Select movement speeds, directional sprite, and animation mode. */
void PlayerChooseDirectionalMovement(player, player_number, input, land)
struct object_state *player;
short player_number, input;
unsigned short land;
{
    short velocity_x, velocity_y, animation_mode, sprite;
    short speed_x, speed_y, current_sprite;

    sprite = -1;
    speed_x = 320;
    speed_y = 240;
    if (land == TERRAIN_FAST_GROUND) {
        speed_x = 400;
        speed_y = 300;
        if (gPlayerTerrainSinking[player_number]) {
            player->height = 0;
            player->draw_flags &= ~ENTITY_SHADOW;
        }
    }
    velocity_x = player->height;
    if (gPlayerTerrainSinking[player_number] && velocity_x < 0) {
        if (velocity_x < -16)
            velocity_x = -16;
        speed_x += velocity_x * 10;
        speed_y += velocity_x * 8;
    }
    if (gPlayerElevatorApproachInputGate[player_number]) {
        if (input & 1)
            input = 1;
        else
            gPlayerElevatorApproachInputGate[player_number] = 0;
    }
    if (!(player->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY)) {
        velocity_x = player->velocity_x;
        velocity_y = player->velocity_y;
    } else {
        velocity_x = player->acceleration_x;
        velocity_y = player->acceleration_y;
    }
    if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS) {
        speed_x *= 3;
        speed_y *= 3;
    }
    current_sprite = player->sprite_state;
    animation_mode = player->animation_motion_axes;
    if (player->animation_group_mask & (long)PLAYER_GROUP_JUMP_WINDUP) {
        if ((current_sprite == PLAYER_ANIM_JUMP_WINDUP_UP && (input & 1)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_DOWN && (input & 2)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_LEFT && (input & 4)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_RIGHT && (input & 8)))
            return;
        sprite = 0;
    } else if (!(input & 15) && (velocity_x || velocity_y)) {
        sprite = 0;
        velocity_x = 0;
        velocity_y = 0;
        animation_mode = 0;
    } else if (input & 12) {
        if (((input & 4) && velocity_x >= 0) ||
            ((input & 8) && velocity_x <= 0)) {
            if (input & 4)
                sprite = PLAYER_ANIM_WALK_LEFT;
            else
                sprite = PLAYER_ANIM_WALK_RIGHT;
            velocity_y = 0;
            animation_mode = 3;
        }
        if (input & 3) {
            if (input & 1)
                velocity_y = -speed_y;
            else
                velocity_y = speed_y;
            if (input & 4)
                velocity_x = -speed_y;
            else
                velocity_x = speed_y;
        } else {
            velocity_y = 0;
            if (input & 4)
                velocity_x = -speed_x;
            else
                velocity_x = speed_x;
        }
    } else if (input & 3) {
        if (velocity_x || ((input & 1) && velocity_y >= 0) ||
            ((input & 2) && velocity_y <= 0)) {
            if (input & 1)
                sprite = PLAYER_ANIM_WALK_UP;
            else
                sprite = PLAYER_ANIM_WALK_DOWN;
            animation_mode = 2;
            velocity_x = 0;
        }
        if (input & 1)
            velocity_y = -speed_x;
        else
            velocity_y = speed_x;
    }
    PlayerApplyDirectionalMovementAnimation(player, player_number, input, sprite, velocity_x, velocity_y, animation_mode);
}

/* Apply movement, select the active animation family, and preserve its phase. */
void PlayerApplyDirectionalMovementAnimation(player, player_number, input, sprite, velocity_x, velocity_y, mode)
struct object_state *player;
short player_number, input, sprite, velocity_x, velocity_y, mode;
{
    char frame, frame_duration;
    short sequence_index;

    frame = player->frame;
    if (player->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY) {
        player->acceleration_x = velocity_x >> 4;
        player->acceleration_y = velocity_y >> 4;
        --frame;
    } else if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS) {
        player->velocity_x = velocity_x;
        player->velocity_y = velocity_y >> 1;
        player->animation_motion_axes = ANIMATION_MOTION_NONE;
    } else {
        player->velocity_x = velocity_x;
        player->velocity_y = velocity_y;
        player->animation_motion_axes = mode;
    }
    if (sprite != -1) {
        if (!sprite) {
            sprite = GetActivePresentSprite(player);
            frame = -1;
        } else {
            sprite &= 3;
            if ((player->animation_group_mask & (long)PLAYER_GROUP_INNERTUBE_MOVE) || player->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE)
                sprite += PLAYER_ANIM_INNERTUBE_MOVE_UP;
            else if ((player->animation_group_mask & (long)PLAYER_GROUP_SWIM) || player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE)
                sprite += PLAYER_ANIM_SWIM_UP;
            else if (player->animation_group_mask & (long)PLAYER_GROUP_SPRING_SHOES)
                sprite += PLAYER_ANIM_SPRING_SHOES_UP;
            else if (player->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY)
                sprite += PLAYER_ANIM_ICARUS_FLY_UP;
            else if ((input & 64) && !ELEVATOR_FLAG[player_number] &&
                     gPlayer1Powerup[player_number] == PRESENT_SUPER_HITOPS) {
                sprite += PLAYER_ANIM_HITOPS_UP;
                if (!(player->animation_group_mask & (long)PLAYER_GROUP_HITOPS))
                    PlaySoundEffect((long)Sfx_Running, 50, 0);
            } else if ((input & 64) && gPlayer1Powerup[player_number] == PRESENT_NONE)
                sprite += PLAYER_ANIM_SNEAK_UP;
            else {
                sprite += PLAYER_ANIM_WALK_UP;
                if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS)
                    player->animation_motion_axes = ANIMATION_MOTION_XY;
            }
        }
    } else if (sprite == -1) {
        if (input & 64) {
            if (player->animation_group_mask & PLAYER_GROUP_WALK) {
                if (gPlayer1Powerup[player_number] == PRESENT_NONE)
                    sprite = player->sprite_state + 4;
                else if (gPlayer1Powerup[player_number] == PRESENT_SUPER_HITOPS && !ELEVATOR_FLAG[player_number]) {
                    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_HITOPS_UP;
                    PlaySoundEffect((long)Sfx_Running, 50, 0);
                }
            }
        } else if ((player->animation_group_mask & (long)(PLAYER_GROUP_SNEAK | PLAYER_GROUP_HITOPS)) && player->frame_duration <= 1) {
            sprite = (player->sprite_state & DIRECTION_MASK) + 8;
            player->animation_motion_axes = ANIMATION_MOTION_XY;
            if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS) {
                player->velocity_x = velocity_x / 3;
                player->velocity_y = velocity_y / 3;
            }
        }
    }
    if (sprite != -1) {
        if ((sprite & 252) == (player->sprite_state & 252)) {
            frame_duration = player->frame_duration;
            if (!frame_duration)
                frame_duration = 1;
        } else {
            if (!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
                (player->animation_group_mask & (long)(PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE)
                frame = -1;
            frame_duration = 1;
        }
        SetSprite(player, sprite);
        player->frame_duration = frame_duration;
        player->frame = frame;
        for (sequence_index = 1; sequence_index <= frame + 1; ++sequence_index) {
            if ((player->sequence[sequence_index] & 240) == 240) {
                player->frame = -1;
                break;
            }
        }
    }
}
