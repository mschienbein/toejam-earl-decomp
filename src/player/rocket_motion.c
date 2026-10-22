#include "player_animation.h"
#include "player_animation_groups.h"
#include "terrain.h"
#include "entities.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
#include "random.h"
#include "sprite_calls.h"

/* Choose a direction from velocity, then select standing, jumping, or
 * braking sprites according to the terrain and the active movement flags. */
void PlayerUpdateRocketSkatesSprite(state, player, terrain)
struct object_state *state;
short player;
unsigned short terrain;
{
    char sprite = -1;
    short velocity_x = state->velocity_x;
    short velocity_y = state->velocity_y;
    char direction = state->sprite_state & DIRECTION_MASK;
    char jump;

    if (velocity_x > 0 && direction != 3)
        direction = 3;
    else if (velocity_x < 0 && direction != 2)
        direction = 2;
    else if (velocity_y > 0 && direction != 1)
        direction = 1;
    else if (velocity_y < 0 && direction != 0)
        direction = 0;
    jump = P1_PROCESSED_INPUT[player] & 0x40;
    if (jump && terrain <= TERRAIN_WALKABLE_MAX && (state->animation_group_mask & PLAYER_GROUP_SKATES))
        sprite = direction + PLAYER_ANIM_SKATES_CROUCH_UP;
    else if (!jump && (state->animation_group_mask & PLAYER_GROUP_SKATES_CROUCH)) {
        state->velocity_height = 0x400;
        state->acceleration_height = -64;
        sprite = direction + PLAYER_ANIM_SKATES_AIR_UP;
    } else if (((terrain == TERRAIN_WATER || terrain == TERRAIN_VOID) &&
                !(state->animation_group_mask & (PLAYER_GROUP_SKATES_WATER | PLAYER_GROUP_SKATES_AIR))) ||
               (terrain != TERRAIN_WATER && (state->animation_group_mask & PLAYER_GROUP_SKATES_WATER))) {
        state->velocity_height = 0x200;
        state->acceleration_height = -64;
        sprite = direction + PLAYER_ANIM_SKATES_AIR_UP;
    } else if ((state->animation_group_mask & PLAYER_GROUP_SKATES_AIR) && state->height <= 0) {
        if (terrain <= TERRAIN_WALKABLE_MAX)
            sprite = direction + PLAYER_ANIM_SKATES_UP;
        else if (terrain == TERRAIN_WATER)
            sprite = direction + PLAYER_ANIM_SKATES_WATER_UP;
        else {
            StopPlayer(state);
            state->acceleration_height = -32;
            sprite = (state->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
            gPlayerLandingCounterSeed[player] = 2;
        }
    } else if (direction != (state->sprite_state & (unsigned long)3))
        /* The unsigned long mask reproduces the ROM's register order; a plain 3 keeps the size
         * but swaps scratch registers ([INFERENCE] the original spelling is unknown). */
        sprite = (state->sprite_state & 0xfc) + direction;
    if (sprite != -1)
        SetSprite(state, sprite);
}

/* Resolve directional input against the current facing, accelerate on one
 * axis, and retain the original asymmetric x-then-y speed clamping. */
void PlayerApplyRocketSkatesInput(state, player, changed_direction, input)
struct object_state *state;
short player;
char changed_direction;
unsigned char input;
{
    char up, down, left, right;
    short velocity_x, velocity_y;
    char direction;

    velocity_x = state->velocity_x;
    velocity_y = state->velocity_y;
    if (!(state->animation_group_mask & PLAYER_GROUP_SKATES_AIR) || (state->draw_flags & (ENTITY_BLOCKED_X | ENTITY_BLOCKED_Y_OR_HEIGHT))) {
        up = input & 1;
        down = input & 2;
        left = input & 4;
        right = input & 8;
        direction = state->sprite_state & DIRECTION_MASK;
        if ((up && right) || (up && left) ||
            (down && right) || (down && left)) {
            if (!changed_direction)
                goto clamp_speed;
            if (up && direction == 0)
                up = 0;
            else if (down && direction == 1)
                down = 0;
            else if (left && direction == 2)
                left = 0;
            else if (right && direction == 3)
                right = 0;
        }
        if (!(input & 15)) {
            if (direction == 0)
                up = 1;
            else if (direction == 1)
                down = 1;
            else if (direction == 2)
                left = 1;
            else if (direction == 3)
                right = 1;
        } else if (up && direction == 1) {
            up = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(left | right))
                right = 1;
        } else if (down && direction == 0) {
            down = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(left | right))
                left = 1;
        } else if (left && direction == 3) {
            left = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(up | down))
                up = 1;
        } else if (right && direction == 2) {
            right = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(up | down))
                down = 1;
        }
        if (left | right) {
            velocity_y = 0;
            state->acceleration_y = 0;
            if (left) {
                state->acceleration_x = -128;
                if (velocity_x > -0x300)
                    velocity_x = -0x300;
            } else {
                state->acceleration_x = 128;
                if (velocity_x < 0x300)
                    velocity_x = 0x300;
            }
        } else {
            velocity_x = 0;
            state->acceleration_x = 0;
            if (up) {
                state->acceleration_y = -128;
                if (velocity_y > -0x240)
                    velocity_y = -0x240;
            } else {
                state->acceleration_y = 128;
                if (velocity_y < 0x240)
                    velocity_y = 0x240;
            }
        }
    }
clamp_speed:
    /* ORIGINAL BUG: An excessive x speed skips the y-speed clamp. */
    if (abs(velocity_x) > 0x600) {
        if (velocity_x > 0)
            velocity_x = 0x600;
        else
            velocity_x = -0x600;
    } else if (abs(velocity_y) > 0x480) {
        if (velocity_y > 0)
            velocity_y = 0x480;
        else
            velocity_y = -0x480;
    }
    state->velocity_x = velocity_x;
    state->velocity_y = velocity_y;
}

/* Handle the active rocket-movement flags, direction-change delay, and
 * collision-flag-driven random turns before updating acceleration and sprite. */
void PlayerUpdateRocketSkatesMotion(state, terrain)
struct object_state *state;
unsigned short terrain;
{
    char changed_direction = 0;
    short player;
    unsigned char input;

    player = state->entity_index;
    if (TOEJAM_FALL_STATE[player] || !(state->animation_group_mask & PLAYER_GROUP_ALL_SKATES) ||
        ELEVATOR_FLAG[player])
        return;
    if ((state->velocity_height < 0 && state->height <= 0) ||
        (state->velocity_height > 0 && !state->acceleration_height && state->height >= 0)) {
        state->height = 0;
        state->velocity_height = 0;
        state->acceleration_height = 0;
        state->draw_flags |= ENTITY_SHADOW;
    }
    if ((P1_PROCESSED_INPUT[player] & 15) != (P1_PREV_PROCESSED_INPUT[player] & 15)) {
        changed_direction = 1;
        gPlayerRocketTurnDelay[player] = 0;
    } else if (gPlayerRocketTurnDelay[player] && gPlayerRocketTurnDelay[player]--)
        return;
    if (gPlayerControlRemapEntities[player])
        input = PlayerRemapDirectionalInput(player);
    else
        input = P1_PROCESSED_INPUT[player];
    if (state->velocity_y && (state->draw_flags & ENTITY_BLOCKED_Y_OR_HEIGHT)) {
        input &= 0xfc;
        changed_direction = 1;
        if (!(input & 12)) {
            if (PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                input |= 1;
            else
                input |= 2;
        }
    }
    if (state->velocity_x && (state->draw_flags & ENTITY_BLOCKED_X)) {
        input &= 0xf3;
        changed_direction = 1;
        if (!(input & 3)) {
            if (PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                input |= 4;
            else
                input |= 8;
        }
    }
    PlayerApplyRocketSkatesInput(state, player, changed_direction, input);
    PlayerUpdateRocketSkatesSprite(state, player, terrain);
}
