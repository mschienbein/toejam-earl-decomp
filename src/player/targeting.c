#include "player_animation.h"
#include "player_animation_groups.h"
#include "earthling_types.h"
#include "entities.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "random.h"

/* Consider two object-index slots first, then the two player records. Squared
 * quarter-coordinate distances choose the nearer eligible target. Modes 12
 * and 7 select steering helpers; mode 14 may call EntityTriggerTargetStatusEventOnce first and then
 * steers the ordinary way. */
char EarthlingSteerTowardNearestTarget(state, maximum_distance, separation, mode)
struct object_state *state;
unsigned short maximum_distance;
short separation;
char mode;
{
    char special_target = 1;
    unsigned short distance[2];
    short delta_x[2], delta_y[2];
    unsigned short selected_player;
    struct object_state *target;

    distance[0] = 0x7fff;
    distance[1] = 0x7fff;
    separation >>= 2;
    if (state->sprite_state == 2)
        maximum_distance = 0x400;
    else
        maximum_distance <<= 8;
    if (gPlayerDecoyEntities[0]) {
        target = &gPlayerInfo[gPlayerDecoyEntities[0]];
        if (state->level == target->level) {
            delta_x[0] = (target->x - state->x) >> 2;
            if (state->x < target->x)
                delta_x[0] -= separation;
            else
                delta_x[0] += separation;
            delta_y[0] = (target->y - state->y) >> 2;
            if (abs(delta_x[0]) < 160 && abs(delta_y[0]) < 112)
                distance[0] = delta_x[0] * delta_x[0] + delta_y[0] * delta_y[0];
        }
        if (distance[0] > 0x1800)
            distance[0] = 0x7fff;
        else
            maximum_distance = distance[0];
    }
    if (gPlayerDecoyEntities[1]) {
        target = &gPlayerInfo[gPlayerDecoyEntities[1]];
        if (state->level == target->level) {
            delta_x[1] = (target->x - state->x) >> 2;
            if (state->x < target->x)
                delta_x[1] -= separation;
            else
                delta_x[1] += separation;
            delta_y[1] = (target->y - state->y) >> 2;
            if (abs(delta_x[1]) < 160 && abs(delta_y[1]) < 112)
                distance[1] = delta_x[1] * delta_x[1] + delta_y[1] * delta_y[1];
        }
        if (distance[1] > 0x1800)
            distance[1] = 0x7fff;
        else
            maximum_distance = distance[1];
    }
    if (distance[0] == 0x7fff && distance[1] == 0x7fff) {
        special_target = 0;
        if (state->level == gPlayerInfo[PLAYER_TOEJAM].level && !ELEVATOR_FLAG[PLAYER_TOEJAM] &&
            (mode != 7 || !gPlayerControlRemapEntities[0])) {
            target = &gPlayerInfo[PLAYER_TOEJAM];
            if ((mode != EARTHLING_MOLE || (gPlayerInventory[PLAYER_TOEJAM][0] != -1 && !state->attachment_x_or_latch)) &&
                EarthlingCanTargetPlayerState(state, target)) {
                delta_x[0] = (target->x - state->x) >> 2;
                if (state->x < target->x)
                    delta_x[0] -= separation;
                else
                    delta_x[0] += separation;
                delta_y[0] = (target->y - state->y) >> 2;
                if (abs(delta_x[0]) < 160 && abs(delta_y[0]) < 112)
                    distance[0] = delta_x[0] * delta_x[0] + delta_y[0] * delta_y[0];
            }
        }
        if (state->level == gPlayerInfo[PLAYER_EARL].level && !ELEVATOR_FLAG[PLAYER_EARL] &&
            (mode != 7 || !gPlayerControlRemapEntities[1])) {
            target = &gPlayerInfo[PLAYER_EARL];
            if ((mode != EARTHLING_MOLE || (gPlayerInventory[PLAYER_EARL][0] != -1 && !state->attachment_y_or_latch)) &&
                EarthlingCanTargetPlayerState(state, target)) {
                delta_x[1] = (target->x - state->x) >> 2;
                if (state->x < target->x)
                    delta_x[1] -= separation;
                else
                    delta_x[1] += separation;
                delta_y[1] = (target->y - state->y) >> 2;
                if (abs(delta_x[1]) < 160 && abs(delta_y[1]) < 112)
                    distance[1] = delta_x[1] * delta_x[1] + delta_y[1] * delta_y[1];
            }
        }
    }
    if (maximum_distance < distance[0] && maximum_distance < distance[1])
        return -1;
    if (distance[0] < distance[1])
        selected_player = 0;
    else
        selected_player = 1;
    if (mode == 12)
        return EntitySteerWithVerticalDeadZone(state, delta_x[selected_player], delta_y[selected_player]);
    if (mode == 7) {
        if (special_target)
            target = &gPlayerInfo[gPlayerDecoyEntities[selected_player]];
        else
            target = &gPlayerInfo[selected_player];
        return EntitySteerWithHorizontalDrift(state, target, delta_x[selected_player], delta_y[selected_player]);
    }
    if (mode == 14 && !special_target)
        EntityTriggerTargetStatusEventOnce(state, target);
    return EntitySetVelocityTowardDelta(state, delta_x[selected_player], delta_y[selected_player]);
}

/* Return true for the observed sprite categories and status masks. Sprite 2
 * on the first record bypasses the first group of eligibility tests. */
int EarthlingCanTargetPlayerState(state, target)
struct object_state *state, *target;
{
    if ((state->sprite_state != 2 &&
         ((target->animation_group_mask & 0x400b8L) || target->sprite_state == PLAYER_ANIM_IDLE ||
          target->sprite_state == PLAYER_ANIM_ICARUS_IDLE || (target->animation_group_mask & (long)(PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) ||
          target->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE || target->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY || target->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE)) ||
        target->sprite_state == PLAYER_ANIM_SWIM_IDLE || target->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
        (target->animation_group_mask & 0x0fb57f44L))
        return 1;
    return 0;
}
