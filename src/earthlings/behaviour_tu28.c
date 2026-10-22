#include "entities.h"
#include "earthlings.h"
#include "ram.h"
#include "assets.h"
#include "earthling_calls.h"
#include "player.h"
#include "present_calls.h"
#include "romcalls.h"
/* Select the next directional animation for earthling category 19. */
char func_019d3e(earthling, object, next_state)
struct earthling_state *earthling;
struct object_state *object;
short next_state;
{
    short animation;

    animation = earthling->state & 252;
    if (animation == 4) {
        if (earthling->value_0a >= 128)
            next_state += 4;
    } else if (animation == 8) {
        if (!object->frame_duration) {
            if (earthling->value_0a >= 128)
                next_state += 8;
            else
                PlaySoundEffect((long)Sfx_IceCreamHonk, 50, 0);
        } else
            next_state += 4;
    } else if (animation == 12) {
        if (earthling->value_0a >= 80 ||
            func_00dd1e(object, &gPlayerInfo[0], 96, 48) ||
            func_00dd1e(object, &gEarlPositionX, 96, 48))
            next_state += 8;
        else
            next_state += 4;
    }
    if (animation != (next_state & 252))
        object->frame = -1;
    return next_state;
}

/* Update category 19's animation independently of its movement timer. */
void func_019e12(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    short next_state = -1;
    short animation;

    animation = earthling->state & 252;
    if (animation == 4) {
        if (earthling->value_0a >= 128)
            next_state = earthling->state + 4;
    } else if (animation == 8) {
        if (!object->frame_duration) {
            if (earthling->value_0a >= 128)
                next_state = earthling->state + 4;
            else {
                next_state = earthling->state - 4;
                PlaySoundEffect((long)Sfx_IceCreamHonk, 50, 0);
            }
        }
    } else if (animation == 12) {
        if (earthling->value_0a < 80 &&
            !func_00dd1e(object, &gPlayerInfo[0], 128, 64) &&
            !func_00dd1e(object, &gEarlPositionX, 128, 64))
            next_state = earthling->state - 4;
    }
    if (next_state >= 0) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
        if ((next_state & 252) == 4)
            object->flags_4d |= 1;
        else
            object->flags_4d &= ~1;
    }
}

/* Reconsider a live earthling's movement after its timer expires.
 * [INFERENCE] The first state byte indexes the 38-byte definition records;
 * target/turn/move names describe the observed helper calls and ranges. */
void func_019f0e(earthling)
struct earthling_state *earthling;
{
    short next_state = -1, separation = 0;
    struct object_state *object;
    struct earthling_definition *definition;
    unsigned char minimum, maximum;

    if (earthling->kind == 7 && earthling->state == 3) {
        func_01b746(earthling);
        return;
    }
    if (earthling->object) {
        object = &gPlayerInfo[earthling->object];
        definition = &EARTHLING_DATA[earthling->kind];
        if (!object->sprite_state && earthling->state && earthling->state != 2) {
            earthling->timer = 0;
            earthling->state_delay = definition->state_delay;
        }
    } else
        return;
    if (--earthling->timer < 0) {
        object->velocity_x = 0;
        object->velocity_y = 0;
        if (!earthling->state_delay) {
            if (definition->target_chance >
                PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                (ram_ff91e0 = 0) : ++ram_ff91e0] % 100) {
                if (earthling->kind == 7)
                    separation = 85;
                next_state = func_01aa20(object, definition->target_distance,
                                        separation, earthling->kind);
                if (next_state != -1) {
                    minimum = definition->target_time_min;
                    maximum = definition->target_time_max;
                    earthling->timer = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                        (ram_ff91e0 = 0) : ++ram_ff91e0] %
                        (maximum - minimum + 1) + minimum;
                }
                if ((earthling->kind == 6 || earthling->kind == 22) &&
                    earthling->state != next_state &&
                    earthling->state >= 4 && next_state >= 4) {
                    next_state = 0;
                    earthling->timer = 2;
                }
            }
        } else
            --earthling->state_delay;
        if (earthling->kind == 19 && next_state == 0)
            next_state = -1;
        if (earthling->kind == 7 && (earthling->state & 252) == 8)
            next_state = -1;
        else if (next_state == -1 && earthling->state != 2) {
            if (definition->turn_chance >
                PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                (ram_ff91e0 = 0) : ++ram_ff91e0] % 100) {
                next_state = func_0196b0(earthling);
                minimum = definition->turn_time_min;
                maximum = definition->turn_time_max;
                earthling->timer = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                    (ram_ff91e0 = 0) : ++ram_ff91e0] %
                    (maximum - minimum + 1) + minimum;
            } else {
                next_state = (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                    (ram_ff91e0 = 0) : ++ram_ff91e0] & 3) + 4;
                minimum = definition->move_time_min;
                maximum = definition->move_time_max;
                earthling->timer = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                    (ram_ff91e0 = 0) : ++ram_ff91e0] %
                    (maximum - minimum + 1) + minimum;
            }
        }
        if (!object->velocity_x && !object->velocity_y) {
            switch (next_state) {
            case 4:
                object->velocity_y = -320;
                break;
            case 5:
                object->velocity_y = 320;
                break;
            case 6:
                object->velocity_x = -320;
                break;
            case 7:
                object->velocity_x = 320;
                break;
            }
        }
        if (earthling->kind == 19 && next_state != -1)
            next_state = func_019d3e(earthling, object, next_state);
        if (next_state != -1 && earthling->state != next_state) {
            SetEarthlingState(earthling, next_state);
            if (earthling->kind == 7 && earthling->value_0a &&
                next_state == 0 && (object->draw_flags & 6)) {
                PlaySoundEffect((long)Sfx_CupidSing, 40, 0);
                earthling->value_0a = 0;
            } else if (earthling->kind == 19) {
                if ((next_state & 252) == 4)
                    object->flags_4d |= 1;
                else
                    object->flags_4d &= ~1;
            }
        }
        if (earthling->timer <= 0)
            earthling->timer = 1;
    }
    if (earthling->kind == 19 &&
        (next_state == -1 || earthling->state != next_state))
        func_019e12(earthling, object);
    if (earthling->kind == 7 && (earthling->state & 252) == 8)
        func_01d288(earthling, object);
}
