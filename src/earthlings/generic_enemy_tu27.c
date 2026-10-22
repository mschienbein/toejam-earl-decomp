#include "revision.h"

/* Canonical object_state fields from accepted src/player/mole.c. */
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer;
    char reserved_6[4];
    short terrain;
    char reserved_c[2], state_delay;
};
struct earthling_definition {
    char reserved_00[21], target_chance;
    unsigned char target_distance;
    char turn_chance;
    unsigned char movement_flags;
    char reserved_19[4];
    unsigned char target_time_min, target_time_max;
    unsigned char turn_time_min, turn_time_max;
    unsigned char move_time_min, move_time_max;
    char state_delay;
    char reserved_24[2];
};
extern struct object_state gPlayerInfo[40];
extern struct earthling_definition EARTHLING_DATA[24];
extern unsigned short PREGEN_RANDOMS[], ram_ff91e0;
extern short TOEJAM_SPEECH_COUNTER;
extern char Sfx_DentistLaugh[];
extern void PlaySoundEffect(), func_01d2cc(), SetEarthlingState(), func_02234c();
/* ORIGINAL BUG: these callers consume word results from func_01aa20 and
 * func_0196b0, although both definitions return char and sign-extend to long.
 * [INFERENCE] The original caller relied on implicit int declarations. */
extern int func_01aa20(), func_0196b0();
extern short GenerateSpeech();

#define NEXT_RANDOM PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0]

/* The reserved byte at +0 is the earthling kind. The common prefix keeps
 * the accepted earthling_state spelling while the shared layout is consolidated. */
int func_01975c(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    if (earthling->reserved == 9 && (earthling->state & 252) != 4)
        return 1;
    if (earthling->reserved == 17 && earthling->state_delay) {
        earthling->state_delay = 0;
        return 1;
    }
    if (!earthling->object)
        return 1;
    if (earthling->reserved == 18) {
        if (earthling->state <= 1) {
            object->flags_4d |= 1;
            object->draw_flags &= 127;
            if (object->frame_duration)
                return 1;
            earthling->timer = 0;
        } else if (earthling->state == 3) {
            object->flags_4d |= 1;
            if (--earthling->timer > 0)
                return 1;
        }
    }
    if ((earthling->state & 252) == 8) {
        if (earthling->reserved == 4 || earthling->reserved == 21) {
            if (!TOEJAM_SPEECH_COUNTER && object->frame_duration > 3 &&
                earthling->terrain) {
                PlaySoundEffect((long)Sfx_DentistLaugh, 50, 0);
                earthling->terrain = 0;
            }
            if (object->frame_duration)
                return 1;
            earthling->timer = 0;
        }
        if (earthling->reserved == 15) {
            func_01d2cc(earthling, object);
            return 1;
        }
        if (earthling->reserved == 16) {
            if (object->frame_duration)
                return 1;
            earthling->timer = 0;
        }
    }
    return 0;
}

/* The switch arms at 019c86 and 019c8e and the common tail at 019c96
 * belong to this frame, not to separately callable functions. */
void AI_GenericEnemyThink(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    struct earthling_definition *definition;
    short next_state = -1, search_bias = 0;
    unsigned char minimum_time, maximum_time;

    object = &gPlayerInfo[earthling->object];
    if (func_01975c(earthling, object))
        return;
    definition = &EARTHLING_DATA[earthling->reserved];
    if (!object->sprite && earthling->state && earthling->state != 2) {
        earthling->timer = 0;
        earthling->state_delay = definition->state_delay;
        object->sprite = earthling->state;
    }
    if (--earthling->timer < 0) {
        object->velocity_x = 0;
        object->velocity_y = 0;
        if (!earthling->state_delay) {
            if (NEXT_RANDOM % 100 < definition->target_chance) {
                if (earthling->reserved == 15)
                    search_bias = 100;
                next_state = func_01aa20(object, definition->target_distance,
                                         search_bias, earthling->reserved);
                if (next_state != -1) {
                    minimum_time = definition->target_time_min;
                    maximum_time = definition->target_time_max;
                    earthling->timer = NEXT_RANDOM %
                        (maximum_time - minimum_time + 1) + minimum_time;
                }
                if (earthling->reserved == 15 && earthling->state != next_state &&
                    (earthling->state & 4) && (next_state & 4)) {
                    next_state = 0;
                    earthling->timer = 2;
                }
            }
        } else
            --earthling->state_delay;
        if (next_state == -1 && earthling->state != 2) {
            if (NEXT_RANDOM % 100 < definition->turn_chance) {
                next_state = func_0196b0(earthling);
                if (earthling->reserved == 3 && next_state == 0 &&
                    (NEXT_RANDOM & 1))
                    GenerateSpeech(earthling->object, "shut up!!", 1);
                if (earthling->state != next_state)
                    object->frame = -1;
                else
                    next_state = -1;
                minimum_time = definition->turn_time_min;
                maximum_time = definition->turn_time_max;
                earthling->timer = NEXT_RANDOM %
                    (maximum_time - minimum_time + 1) + minimum_time;
            } else {
                next_state = (NEXT_RANDOM & 3) + 4;
                minimum_time = definition->move_time_min;
                maximum_time = definition->move_time_max;
                earthling->timer = NEXT_RANDOM %
                    (maximum_time - minimum_time + 1) + minimum_time;
            }
        }
        if ((definition->movement_flags & 2) && next_state >= 4 &&
            earthling->state != next_state &&
            (next_state & 2) == (earthling->state & 2)) {
            next_state = (next_state ^ 2) & 254;
            next_state |= (char)(NEXT_RANDOM & 1);
            object->velocity_x = 0;
            object->velocity_y = 0;
        }
        if (!object->velocity_x && !object->velocity_y) {
            switch (next_state) {
            case 4: object->velocity_y = -320; break;
            case 5: object->velocity_y = 320; break;
            case 6: object->velocity_x = -320; break;
            case 7: object->velocity_x = 320; break;
            }
        }
        if (next_state != -1 && earthling->state != next_state) {
            SetEarthlingState(earthling, next_state);
            if (earthling->reserved == 4 || earthling->reserved == 21)
                object->frame = -1;
            if (earthling->reserved == 18) {
                if (earthling->state > 1)
                    object->draw_flags |= 128;
                else
                    object->draw_flags &= 127;
            }
        }
        if (earthling->timer <= 0)
            earthling->timer = 1;
    }
    if (earthling->reserved == 18 && (earthling->state & 252) == 4) {
        func_02234c(earthling, object);
        object->flags_4d &= 254;
    }
}
