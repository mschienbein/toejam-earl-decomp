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
    char sprite = PLAYER_ANIM_UNCHANGED;
    short velocity_x = state->velocity_x;
    short velocity_y = state->velocity_y;
    char direction = state->sprite_state & DIRECTION_MASK;
    char jump;

    if (velocity_x > 0 && direction != DIRECTION_RIGHT)
        direction = DIRECTION_RIGHT;
    else if (velocity_x < 0 && direction != DIRECTION_LEFT)
        direction = DIRECTION_LEFT;
    else if (velocity_y > 0 && direction != DIRECTION_DOWN)
        direction = DIRECTION_DOWN;
    else if (velocity_y < 0 && direction != DIRECTION_UP)
        direction = DIRECTION_UP;
    jump = gPlayerProcessedInput[player] & INPUT_BUTTON_A;
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
    } else if (direction != (state->sprite_state & (unsigned long)DIRECTION_MASK))
        /* MATCHING QUALIFICATION: the unsigned-long direction mask retains
         * the observed scratch-register order; a plain int mask keeps the
         * size but swaps scratch registers. Historical spelling is unknown [INFERENCE]. */
        sprite = (state->sprite_state & PLAYER_ANIM_FAMILY_MASK) + direction;
    if (sprite != PLAYER_ANIM_UNCHANGED)
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
        up = input & INPUT_UP;
        down = input & INPUT_DOWN;
        left = input & INPUT_LEFT;
        right = input & INPUT_RIGHT;
        direction = state->sprite_state & DIRECTION_MASK;
        if ((up && right) || (up && left) ||
            (down && right) || (down && left)) {
            if (!changed_direction)
                goto clamp_speed;
            if (up && direction == DIRECTION_UP)
                up = 0;
            else if (down && direction == DIRECTION_DOWN)
                down = 0;
            else if (left && direction == DIRECTION_LEFT)
                left = 0;
            else if (right && direction == DIRECTION_RIGHT)
                right = 0;
        }
        if (!(input & INPUT_DIRECTION_MASK)) {
            if (direction == DIRECTION_UP)
                up = 1;
            else if (direction == DIRECTION_DOWN)
                down = 1;
            else if (direction == DIRECTION_LEFT)
                left = 1;
            else if (direction == DIRECTION_RIGHT)
                right = 1;
        } else if (up && direction == DIRECTION_DOWN) {
            up = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(left | right))
                right = 1;
        } else if (down && direction == DIRECTION_UP) {
            down = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(left | right))
                left = 1;
        } else if (left && direction == DIRECTION_RIGHT) {
            left = 0;
            gPlayerRocketTurnDelay[player] = 3;
            if (!(up | down))
                up = 1;
        } else if (right && direction == DIRECTION_LEFT) {
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
    if (gPlayerFallStates[player] || !(state->animation_group_mask & PLAYER_GROUP_ALL_SKATES) ||
        gPlayerElevatorFlags[player])
        return;
    if ((state->velocity_height < 0 && state->height <= 0) ||
        (state->velocity_height > 0 && !state->acceleration_height && state->height >= 0)) {
        state->height = 0;
        state->velocity_height = 0;
        state->acceleration_height = 0;
        state->draw_flags |= ENTITY_SHADOW;
    }
    if ((gPlayerProcessedInput[player] & INPUT_DIRECTION_MASK) != (gPlayerPreviousProcessedInput[player] & INPUT_DIRECTION_MASK)) {
        changed_direction = 1;
        gPlayerRocketTurnDelay[player] = 0;
    } else if (gPlayerRocketTurnDelay[player] && gPlayerRocketTurnDelay[player]--)
        return;
    if (gPlayerControlRemapEntities[player])
        input = PlayerRemapDirectionalInput(player);
    else
        input = gPlayerProcessedInput[player];
    if (state->velocity_y && (state->draw_flags & ENTITY_BLOCKED_Y_OR_HEIGHT)) {
        input &= (INPUT_BUTTON_MASK | INPUT_HORIZONTAL_MASK);
        changed_direction = 1;
        if (!(input & INPUT_HORIZONTAL_MASK)) {
            if (gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                input |= INPUT_UP;
            else
                input |= INPUT_DOWN;
        }
    }
    if (state->velocity_x && (state->draw_flags & ENTITY_BLOCKED_X)) {
        input &= (INPUT_BUTTON_MASK | INPUT_VERTICAL_MASK);
        changed_direction = 1;
        if (!(input & INPUT_VERTICAL_MASK)) {
            if (gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                input |= INPUT_LEFT;
            else
                input |= INPUT_RIGHT;
        }
    }
    PlayerApplyRocketSkatesInput(state, player, changed_direction, input);
    PlayerUpdateRocketSkatesSprite(state, player, terrain);
}
