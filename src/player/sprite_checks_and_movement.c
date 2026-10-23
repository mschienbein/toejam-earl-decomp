#include "player_animation.h"
#include "player_animation_groups.h"
#include "entities.h"
#include "ram_player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_system.h"
#include "player.h"
#include "present_calls.h"
#include "sprite_calls.h"
#include "elevator_calls.h"

/* Filter movement at world boundaries and during restricted sprite states. */
void SpriteChecks(player_number, input)
short player_number, input;
{
    unsigned short land;
    short sprite;
    int width;
    int player_y;
    struct object_state *player;

    if (!player_number) {
        land = gPlayer1TerrainUnderfoot;
        player = gPlayerInfo;
    } else {
        land = gPlayer2TerrainUnderfoot;
        player = &gPlayer2Entity;
    }
    if (player->draw_flags & ENTITY_SKIP_PHYSICS)
        return;
    if (player->sprite_state == PLAYER_ANIM_HULA) {
        if (gPlayerHulaDanceTicks[player_number] > 0)
            --gPlayerHulaDanceTicks[player_number];
        else {
            SetSprite(player, PLAYER_ANIM_IDLE);
            player->velocity_x = 0;
            player->velocity_y = 0;
        }
        return;
    }
    if ((player->animation_group_mask & PLAYER_GROUP_BLOCK_DIRECTIONAL_INPUT) ||
        (player->animation_group_mask & (long)PLAYER_GROUP_REACTION) ||
        player->sprite_state == PLAYER_ANIM_WHIRLWIND ||
        player->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY ||
        player->sprite_state == PLAYER_ANIM_GHOST ||
        player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST ||
        player->sprite_state == PLAYER_ANIM_FLATTENED ||
        ((player->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY) && player->height < 4))
        return;
    if (gPlayerControlRemapEntities[player_number])
        input = PlayerRemapDirectionalInput(player_number);
    if (player->x <= 8)
        input &= ~INPUT_LEFT;
    /* MATCHING QUALIFICATION: these single-use int ownership locals are
     * assigned inside the comparisons to select the original scratch registers.
     * Direct field expressions, separate assignments, and block-local
     * initializers did not match. Neither assignment changes a field or input. */
    if ((width = player->width) + player->x >= 2672)
        input &= ~INPUT_RIGHT;
    if ((player_y = player->y) - player->height_pixels <= 8)
        input &= ~INPUT_UP;
    if (player->y >= 1776)
        input &= ~INPUT_DOWN;
    if (gPlayerElevatorFlags[player_number]) {
        if (!ProcessElevator(gPlayerElevatorFlags[player_number]))
            input = 0;
        else
            input &= ~INPUT_HORIZONTAL_MASK;
    }
    if ((gDoorwayPlayerContactFlags[0] == ((player_number + 1) | 4) ||
         gSecondDoorwayPlayerContactFlags == ((player_number + 1) | 4)) && (input & INPUT_UP))
        input &= ~INPUT_HORIZONTAL_MASK;
    sprite = player->sprite_state;
    if (player->animation_group_mask & (long)PLAYER_GROUP_LEAN) {
        if ((sprite == PLAYER_ANIM_LEAN_UP && (input & INPUT_UP)) ||
            (sprite == PLAYER_ANIM_LEAN_DOWN && (input & INPUT_DOWN)) ||
            (sprite == PLAYER_ANIM_LEAN_LEFT && (input & INPUT_LEFT)) ||
            (sprite == PLAYER_ANIM_LEAN_RIGHT && (input & INPUT_RIGHT)))
            return;
        player->velocity_x = 0;
        player->velocity_y = 0;
    }
    PlayerChooseDirectionalMovement(player, player_number, input, land);
}

#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "entities.h"
#include "sprites.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
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
        if (input & INPUT_UP)
            input = INPUT_UP;
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
        if ((current_sprite == PLAYER_ANIM_JUMP_WINDUP_UP && (input & INPUT_UP)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_DOWN && (input & INPUT_DOWN)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_LEFT && (input & INPUT_LEFT)) ||
            (current_sprite == PLAYER_ANIM_JUMP_WINDUP_RIGHT && (input & INPUT_RIGHT)))
            return;
        sprite = 0;
    } else if (!(input & INPUT_DIRECTION_MASK) && (velocity_x || velocity_y)) {
        sprite = 0;
        velocity_x = 0;
        velocity_y = 0;
        animation_mode = ANIMATION_MOTION_NONE;
    } else if (input & INPUT_HORIZONTAL_MASK) {
        if (((input & INPUT_LEFT) && velocity_x >= 0) ||
            ((input & INPUT_RIGHT) && velocity_x <= 0)) {
            if (input & INPUT_LEFT)
                sprite = PLAYER_ANIM_WALK_LEFT;
            else
                sprite = PLAYER_ANIM_WALK_RIGHT;
            velocity_y = 0;
            animation_mode = ANIMATION_MOTION_XY;
        }
        if (input & INPUT_VERTICAL_MASK) {
            if (input & INPUT_UP)
                velocity_y = -speed_y;
            else
                velocity_y = speed_y;
            if (input & INPUT_LEFT)
                velocity_x = -speed_y;
            else
                velocity_x = speed_y;
        } else {
            velocity_y = 0;
            if (input & INPUT_LEFT)
                velocity_x = -speed_x;
            else
                velocity_x = speed_x;
        }
    } else if (input & INPUT_VERTICAL_MASK) {
        if (velocity_x || ((input & INPUT_UP) && velocity_y >= 0) ||
            ((input & INPUT_DOWN) && velocity_y <= 0)) {
            if (input & INPUT_UP)
                sprite = PLAYER_ANIM_WALK_UP;
            else
                sprite = PLAYER_ANIM_WALK_DOWN;
            animation_mode = ANIMATION_MOTION_Y;
            velocity_x = 0;
        }
        if (input & INPUT_UP)
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
            sprite &= DIRECTION_MASK;
            if ((player->animation_group_mask & (long)PLAYER_GROUP_INNERTUBE_MOVE) || player->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE)
                sprite += PLAYER_ANIM_INNERTUBE_MOVE_UP;
            else if ((player->animation_group_mask & (long)PLAYER_GROUP_SWIM) || player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE)
                sprite += PLAYER_ANIM_SWIM_UP;
            else if (player->animation_group_mask & (long)PLAYER_GROUP_SPRING_SHOES)
                sprite += PLAYER_ANIM_SPRING_SHOES_UP;
            else if (player->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY)
                sprite += PLAYER_ANIM_ICARUS_FLY_UP;
            else if ((input & INPUT_BUTTON_A) && !gPlayerElevatorFlags[player_number] &&
                     gPlayerPowerups[player_number] == PRESENT_SUPER_HITOPS) {
                sprite += PLAYER_ANIM_HITOPS_UP;
                if (!(player->animation_group_mask & (long)PLAYER_GROUP_HITOPS))
                    PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            } else if ((input & INPUT_BUTTON_A) && gPlayerPowerups[player_number] == PRESENT_NONE)
                sprite += PLAYER_ANIM_SNEAK_UP;
            else {
                sprite += PLAYER_ANIM_WALK_UP;
                if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS)
                    player->animation_motion_axes = ANIMATION_MOTION_XY;
            }
        }
    } else if (sprite == -1) {
        if (input & INPUT_BUTTON_A) {
            if (player->animation_group_mask & PLAYER_GROUP_WALK) {
                if (gPlayerPowerups[player_number] == PRESENT_NONE)
                    sprite = player->sprite_state + (PLAYER_ANIM_SNEAK_UP - PLAYER_ANIM_WALK_UP);
                else if (gPlayerPowerups[player_number] == PRESENT_SUPER_HITOPS && !gPlayerElevatorFlags[player_number]) {
                    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_HITOPS_UP;
                    PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
                }
            }
        } else if ((player->animation_group_mask & (long)(PLAYER_GROUP_SNEAK | PLAYER_GROUP_HITOPS)) && player->frame_duration <= 1) {
            sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_WALK_UP;
            player->animation_motion_axes = ANIMATION_MOTION_XY;
            if (player->animation_group_mask & (long)PLAYER_GROUP_HITOPS) {
                player->velocity_x = velocity_x / 3;
                player->velocity_y = velocity_y / 3;
            }
        }
    }
    if (sprite != -1) {
        if ((sprite & PLAYER_ANIM_FAMILY_MASK) == (player->sprite_state & PLAYER_ANIM_FAMILY_MASK)) {
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
            if ((player->sequence[sequence_index] & ANIMATION_COMMAND_MASK) == ANIMATION_COMMAND_MASK) {
                player->frame = -1;
                break;
            }
        }
    }
}
