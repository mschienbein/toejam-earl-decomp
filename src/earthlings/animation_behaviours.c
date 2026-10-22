#include "entities.h"
#include "earthlings.h"
#include "ram.h"
#include "assets.h"
#include "earthling_calls.h"
#include "player.h"
#define NEXT_RANDOM PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0]

/* Reverse blocked horizontal motion, probabilistically acquire a target,
 * otherwise continue a timed horizontal wander with a random vertical drift. */
void func_01a3a4(earthling)
struct earthling_state *earthling;
{
    short next_state = -1;
    struct object_state *object;
    struct earthling_definition *definition;
    short previous_velocity_x, previous_velocity_y;
    unsigned char minimum_time, maximum_time;

    if (earthling->object) {
        object = &gPlayerInfo[earthling->object];
        definition = &EARTHLING_DATA[earthling->kind];
        if (!object->sprite_state && earthling->state && earthling->state != 2) {
            object->velocity_x = -object->velocity_x;
            if (NEXT_RANDOM & 1)
                object->velocity_y = -object->velocity_y;
            next_state = earthling->state ^ 1;
            earthling->timer = 3;
            earthling->state_delay = definition->state_delay;
            SetEarthlingState(earthling, next_state);
        }
    } else
        return;
    if (--earthling->timer < 0) {
        if (!earthling->state_delay) {
            if (definition->target_chance > NEXT_RANDOM % 100) {
                previous_velocity_x = object->velocity_x;
                previous_velocity_y = object->velocity_y;
                next_state = func_01aa20(object, definition->target_distance, 0,
                                        earthling->kind);
                if (next_state != -1) {
                    if (earthling->state != 2 && (object->draw_flags & 6) &&
                        ((earthling->state ^ next_state) & 1)) {
                        next_state = -1;
                        object->velocity_x = previous_velocity_x;
                        object->velocity_y = previous_velocity_y;
                    }
                    minimum_time = definition->target_time_min;
                    maximum_time = definition->target_time_max;
                    earthling->timer = NEXT_RANDOM % (maximum_time - minimum_time + 1) + minimum_time;
                }
            }
        } else
            --earthling->state_delay;
        if (next_state == -1 && earthling->state != 2) {
            if (!earthling->state) {
                next_state = (NEXT_RANDOM & 1) + 6;
                if (next_state == 6)
                    object->velocity_x = -320;
                else
                    object->velocity_x = 320;
            }
            if (NEXT_RANDOM % 100 < 33)
                object->velocity_y = 0;
            else if (NEXT_RANDOM % 100 < 50)
                object->velocity_y = -320;
            else
                object->velocity_y = 320;
            minimum_time = definition->move_time_min;
            maximum_time = definition->move_time_max;
            earthling->timer = NEXT_RANDOM % (maximum_time - minimum_time + 1) + minimum_time;
        }
        if (next_state != -1 && earthling->state != next_state)
            SetEarthlingState(earthling, next_state);
        if (earthling->timer <= 0)
            earthling->timer = 1;
    }
}

/* Clear the one-update delay, then reverse or step between the observed
 * facing states when the animation ends or the current sprite is blocked. */
void func_01a722(earthling)
struct earthling_state *earthling;
{
    short next_state = -1;
    struct object_state *object;

    if (!earthling->object)
        return;
    if (earthling->state_delay) {
        earthling->state_delay = 0;
        return;
    }
    object = &gPlayerInfo[earthling->object];
    if (!object->sprite_state && earthling->state >= 6)
        next_state = func_0196b0(earthling);
    else if (!object->frame_duration) {
        if (earthling->state == 2)
            next_state = 3;
        else if (earthling->state <= 1 || earthling->state == 3) {
            if (object->velocity_x > 0) {
                next_state = 4;
                object->velocity_x = -320;
            } else {
                next_state = 5;
                object->velocity_x = 320;
            }
        } else if (earthling->state == 4) {
            next_state = 6;
            object->velocity_x = -320;
        } else if (earthling->state == 5) {
            next_state = 7;
            object->velocity_x = 320;
        }
    }
    if (next_state != -1 && earthling->state != next_state) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* Reverse a moving earthling when the two menu flags permit it, and restart
 * its animation whenever the selected state changes. */
void func_01a7f0(earthling)
struct earthling_state *earthling;
{
    short next_state = -1;
    struct object_state *object;

    if (!earthling->object)
        return;
    if (earthling->state_delay) {
        earthling->state_delay = 0;
        return;
    }
    object = &gPlayerInfo[earthling->object];
    if (!object->sprite_state && earthling->state >= 2)
        next_state = func_0196b0(earthling);
    else if (earthling->state <= 1 && P1_MENU_OPEN[0] != 3 && HBLANK_FLAG_1 != 3) {
        if (object->velocity_x > 0) {
            next_state = 2;
            object->velocity_x = -320;
        } else {
            next_state = 3;
            object->velocity_x = 320;
        }
    }
    if (next_state != -1 && earthling->state != next_state) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* Acquire target mode 8 at timer expiry. State 3 waits for animation and
 * status-bit transitions; other states expire a signed byte phase timer. */
void func_01a8a0(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    struct earthling_definition *definition;
    short next_state;
    unsigned char minimum_time, maximum_time;

    if (!earthling->object)
        return;
    object = &gPlayerInfo[earthling->object];
    definition = &EARTHLING_DATA[earthling->kind];
    if (earthling->state == 2) {
        if (!object->frame_duration)
            func_0180fe(earthling);
        return;
    }
    if (--earthling->timer < 0) {
        object->velocity_x = 0;
        object->velocity_y = 0;
        next_state = func_01aa20(object, definition->target_distance, 0, 8);
        if (next_state != -1) {
            minimum_time = definition->target_time_min;
            maximum_time = definition->target_time_max;
            earthling->timer = NEXT_RANDOM % (maximum_time - minimum_time + 1) + minimum_time;
        }
        if (earthling->state == 3) {
            if (object->flags_4d & 0x40) {
                if (next_state != -1) {
                    object->frame = -1;
                    object->frame_duration = 1;
                    object->sprite_ready = 0;
                    object->flags_4d &= 0xbf;
                }
                earthling->timer = 3;
            } else if (!object->frame_duration) {
                SetEarthlingState(earthling, 4);
                earthling->timer = 0;
                earthling->reserved_10[0] = 7;
            } else
                earthling->timer = 1;
        } else if (--earthling->reserved_10[0] < 0 || next_state == -1) {
            SetEarthlingState(earthling, 2);
            object->frame = -1;
        }
    }
}
