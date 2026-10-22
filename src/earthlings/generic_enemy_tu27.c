#include "entities.h"
#include "earthlings.h"
#include "revision.h"
#include "assets_audio.h"
#include "assets_earthlings.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"

#define NEXT_RANDOM PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0]

/* The byte at +0 is the earthling kind. */
int func_01975c(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    if (earthling->kind == 9 && (earthling->state & 252) != 4)
        return 1;
    if (earthling->kind == 17 && earthling->state_delay) {
        earthling->state_delay = 0;
        return 1;
    }
    if (!earthling->object)
        return 1;
    if (earthling->kind == 18) {
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
        if (earthling->kind == 4 || earthling->kind == 21) {
            if (!TOEJAM_SPEECH_COUNTER && object->frame_duration > 3 &&
                earthling->value_0a) {
                PlaySoundEffect((long)Sfx_DentistLaugh, 50, 0);
                earthling->value_0a = 0;
            }
            if (object->frame_duration)
                return 1;
            earthling->timer = 0;
        }
        if (earthling->kind == 15) {
            func_01d2cc(earthling, object);
            return 1;
        }
        if (earthling->kind == 16) {
            if (object->frame_duration)
                return 1;
            earthling->timer = 0;
        }
    }
    return 0;
}

/* The merged switch-case entries at 019c86, 019c8e, 019c96 and 019c9e
 * join at the common tail 019ca4; they are not separately callable functions. */
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
    definition = &EARTHLING_DATA[earthling->kind];
    if (!object->sprite_state && earthling->state && earthling->state != 2) {
        earthling->timer = 0;
        earthling->state_delay = definition->state_delay;
        object->sprite_state = earthling->state;
    }
    if (--earthling->timer < 0) {
        object->velocity_x = 0;
        object->velocity_y = 0;
        if (!earthling->state_delay) {
            if (NEXT_RANDOM % 100 < definition->target_chance) {
                if (earthling->kind == 15)
                    search_bias = 100;
                next_state = func_01aa20(object, definition->target_distance,
                                         search_bias, earthling->kind);
                if (next_state != -1) {
                    minimum_time = definition->target_time_min;
                    maximum_time = definition->target_time_max;
                    earthling->timer = NEXT_RANDOM %
                        (maximum_time - minimum_time + 1) + minimum_time;
                }
                if (earthling->kind == 15 && earthling->state != next_state &&
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
                if (earthling->kind == 3 && next_state == 0 &&
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
        if ((definition->flags_18 & 2) && next_state >= 4 &&
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
            if (earthling->kind == 4 || earthling->kind == 21)
                object->frame = -1;
            if (earthling->kind == 18) {
                if (earthling->state > 1)
                    object->draw_flags |= 128;
                else
                    object->draw_flags &= 127;
            }
        }
        if (earthling->timer <= 0)
            earthling->timer = 1;
    }
    if (earthling->kind == 18 && (earthling->state & 252) == 4) {
        func_02234c(earthling, object);
        object->flags_4d &= 254;
    }
}
