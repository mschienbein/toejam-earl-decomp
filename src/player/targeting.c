#include "entities.h"
#include "ram.h"
#include "player.h"
#include "romcalls.h"
/* Consider two object-index slots first, then the two player records. Squared
 * quarter-coordinate distances choose the nearer eligible target. Modes 12
 * and 7 select steering helpers; mode 14 may call func_01b078 first and then
 * steers the ordinary way. */
char func_01aa20(state, maximum_distance, separation, mode)
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
    if (ram_ffde58[0]) {
        target = &gPlayerInfo[ram_ffde58[0]];
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
    if (ram_ffde58[1]) {
        target = &gPlayerInfo[ram_ffde58[1]];
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
        if (state->level == gPlayerInfo[0].level && !ELEVATOR_FLAG[0] &&
            (mode != 7 || !ram_ffe1dc[0])) {
            target = &gPlayerInfo[0];
            if ((mode != 18 || (gPlayerInventory[0][0] != -1 && !state->flag_57)) &&
                func_01ae92(state, target)) {
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
        if (state->level == gPlayerInfo[1].level && !ELEVATOR_FLAG[1] &&
            (mode != 7 || !ram_ffe1dc[1])) {
            target = &gPlayerInfo[1];
            if ((mode != 18 || (gPlayerInventory[1][0] != -1 && !state->flag_58)) &&
                func_01ae92(state, target)) {
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
        return func_01b02e(state, delta_x[selected_player], delta_y[selected_player]);
    if (mode == 7) {
        if (special_target)
            target = &gPlayerInfo[ram_ffde58[selected_player]];
        else
            target = &gPlayerInfo[selected_player];
        return func_01afb8(state, target, delta_x[selected_player], delta_y[selected_player]);
    }
    if (mode == 14 && !special_target)
        func_01b078(state, target);
    return func_01af04(state, delta_x[selected_player], delta_y[selected_player]);
}

/* Return true for the observed sprite categories and status masks. Sprite 2
 * on the first record bypasses the first group of eligibility tests. */
int func_01ae92(state, target)
struct object_state *state, *target;
{
    if ((state->sprite_state != 2 &&
         ((target->flags_52 & 0x400b8L) || target->sprite_state == 0 ||
          target->sprite_state == 4 || (target->flags_52 & 0x20400000L) ||
          target->sprite_state == 76 || target->sprite_state == 78 || target->sprite_state == 77)) ||
        target->sprite_state == 3 || target->sprite_state == 7 ||
        (target->flags_52 & 0x0fb57f44L))
        return 1;
    return 0;
}
