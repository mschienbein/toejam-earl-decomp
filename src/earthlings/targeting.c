#include "player_animation.h"
#include "player_animation_groups.h"
#include "earthling_types.h"
#include "present_types.h"
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
    unsigned short distance[PLAYER_COUNT];
    short delta_x[2], delta_y[2];
    unsigned short selected_player;
    struct object_state *target;

    distance[PLAYER_TOEJAM] = 0x7fff;
    distance[PLAYER_EARL] = 0x7fff;
    separation >>= 2;
    if (state->sprite_state == 2)
        maximum_distance = 0x400;
    else
        maximum_distance <<= 8;
    if (gPlayerDecoyEntities[PLAYER_TOEJAM]) {
        target = &gPlayerInfo[gPlayerDecoyEntities[PLAYER_TOEJAM]];
        if (state->level == target->level) {
            delta_x[PLAYER_TOEJAM] = (target->x - state->x) >> 2;
            if (state->x < target->x)
                delta_x[PLAYER_TOEJAM] -= separation;
            else
                delta_x[PLAYER_TOEJAM] += separation;
            delta_y[PLAYER_TOEJAM] = (target->y - state->y) >> 2;
            if (abs(delta_x[PLAYER_TOEJAM]) < 160 && abs(delta_y[PLAYER_TOEJAM]) < 112)
                distance[PLAYER_TOEJAM] = delta_x[PLAYER_TOEJAM] * delta_x[PLAYER_TOEJAM] + delta_y[PLAYER_TOEJAM] * delta_y[PLAYER_TOEJAM];
        }
        if (distance[PLAYER_TOEJAM] > 0x1800)
            distance[PLAYER_TOEJAM] = 0x7fff;
        else
            maximum_distance = distance[PLAYER_TOEJAM];
    }
    if (gPlayerDecoyEntities[PLAYER_EARL]) {
        target = &gPlayerInfo[gPlayerDecoyEntities[PLAYER_EARL]];
        if (state->level == target->level) {
            delta_x[PLAYER_EARL] = (target->x - state->x) >> 2;
            if (state->x < target->x)
                delta_x[PLAYER_EARL] -= separation;
            else
                delta_x[PLAYER_EARL] += separation;
            delta_y[PLAYER_EARL] = (target->y - state->y) >> 2;
            if (abs(delta_x[PLAYER_EARL]) < 160 && abs(delta_y[PLAYER_EARL]) < 112)
                distance[PLAYER_EARL] = delta_x[PLAYER_EARL] * delta_x[PLAYER_EARL] + delta_y[PLAYER_EARL] * delta_y[PLAYER_EARL];
        }
        if (distance[PLAYER_EARL] > 0x1800)
            distance[PLAYER_EARL] = 0x7fff;
        else
            maximum_distance = distance[PLAYER_EARL];
    }
    if (distance[PLAYER_TOEJAM] == 0x7fff && distance[PLAYER_EARL] == 0x7fff) {
        special_target = 0;
        if (state->level == gPlayerInfo[PLAYER_TOEJAM].level && !gPlayerElevatorFlags[PLAYER_TOEJAM] &&
            (mode != EARTHLING_CUPID || !gPlayerControlRemapEntities[PLAYER_TOEJAM])) {
            target = &gPlayerInfo[PLAYER_TOEJAM];
            if ((mode != EARTHLING_MOLE || (gPlayerInventory[PLAYER_TOEJAM][0] != PRESENT_NONE && !state->attachment_x_or_latch)) &&
                EarthlingCanTargetPlayerState(state, target)) {
                delta_x[PLAYER_TOEJAM] = (target->x - state->x) >> 2;
                if (state->x < target->x)
                    delta_x[PLAYER_TOEJAM] -= separation;
                else
                    delta_x[PLAYER_TOEJAM] += separation;
                delta_y[PLAYER_TOEJAM] = (target->y - state->y) >> 2;
                if (abs(delta_x[PLAYER_TOEJAM]) < 160 && abs(delta_y[PLAYER_TOEJAM]) < 112)
                    distance[PLAYER_TOEJAM] = delta_x[PLAYER_TOEJAM] * delta_x[PLAYER_TOEJAM] + delta_y[PLAYER_TOEJAM] * delta_y[PLAYER_TOEJAM];
            }
        }
        if (state->level == gPlayerInfo[PLAYER_EARL].level && !gPlayerElevatorFlags[PLAYER_EARL] &&
            (mode != EARTHLING_CUPID || !gPlayerControlRemapEntities[PLAYER_EARL])) {
            target = &gPlayerInfo[PLAYER_EARL];
            if ((mode != EARTHLING_MOLE || (gPlayerInventory[PLAYER_EARL][0] != PRESENT_NONE && !state->attachment_y_or_latch)) &&
                EarthlingCanTargetPlayerState(state, target)) {
                delta_x[PLAYER_EARL] = (target->x - state->x) >> 2;
                if (state->x < target->x)
                    delta_x[PLAYER_EARL] -= separation;
                else
                    delta_x[PLAYER_EARL] += separation;
                delta_y[PLAYER_EARL] = (target->y - state->y) >> 2;
                if (abs(delta_x[PLAYER_EARL]) < 160 && abs(delta_y[PLAYER_EARL]) < 112)
                    distance[PLAYER_EARL] = delta_x[PLAYER_EARL] * delta_x[PLAYER_EARL] + delta_y[PLAYER_EARL] * delta_y[PLAYER_EARL];
            }
        }
    }
    if (maximum_distance < distance[PLAYER_TOEJAM] && maximum_distance < distance[PLAYER_EARL])
        return -1;
    if (distance[PLAYER_TOEJAM] < distance[PLAYER_EARL])
        selected_player = PLAYER_TOEJAM;
    else
        selected_player = PLAYER_EARL;
    if (mode == EARTHLING_NERD_HERD)
        return EntitySteerWithVerticalDeadZone(state, delta_x[selected_player], delta_y[selected_player]);
    if (mode == EARTHLING_CUPID) {
        if (special_target)
            target = &gPlayerInfo[gPlayerDecoyEntities[selected_player]];
        else
            target = &gPlayerInfo[selected_player];
        return EntitySteerWithHorizontalDrift(state, target, delta_x[selected_player], delta_y[selected_player]);
    }
    if (mode == EARTHLING_SHARK && !special_target)
        EntityTriggerTargetStatusEventOnce(state, target);
    return EntitySetVelocityTowardDelta(state, delta_x[selected_player], delta_y[selected_player]);
}

/* Return true for the observed sprite categories and status masks. Sprite 2
 * on the first record bypasses the first group of eligibility tests. */
int EarthlingCanTargetPlayerState(state, target)
struct object_state *state, *target;
{
    if ((state->sprite_state != 2 &&
         ((target->animation_group_mask & PLAYER_GROUP_TARGET_NON_ATTACK) || target->sprite_state == PLAYER_ANIM_IDLE ||
          target->sprite_state == PLAYER_ANIM_ICARUS_IDLE || (target->animation_group_mask & (long)(PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) ||
          target->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE || target->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY || target->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE)) ||
        target->sprite_state == PLAYER_ANIM_SWIM_IDLE || target->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
        (target->animation_group_mask & PLAYER_GROUP_TARGET_ACTIVE_MOTION))
        return 1;
    return 0;
}
