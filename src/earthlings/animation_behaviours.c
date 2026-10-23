#include "earthling_types.h"
#include "entities.h"
#include "earthlings.h"
#include "assets_earthlings.h"
#include "earthling_calls.h"
#include "player.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "sprite_calls.h"

#define NEXT_RANDOM gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex]

/* Reverse blocked horizontal motion, probabilistically acquire a target,
 * otherwise continue a timed horizontal wander with a random vertical drift. */
void EarthlingThinkHorizontalWander(earthling)
struct earthling_state *earthling;
{
    short next_state = EARTHLING_STATE_UNCHANGED;
    struct object_state *object;
    struct earthling_definition *definition;
    short previous_velocity_x, previous_velocity_y;
    unsigned char minimum_time, maximum_time;

    if (earthling->object) {
        object = &gPlayerInfo[earthling->object];
        definition = &EarthlingDefinitions[earthling->kind];
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
                next_state = EarthlingSteerTowardNearestTarget(object, definition->target_distance, 0,
                                        earthling->kind);
                if (next_state != EARTHLING_STATE_UNCHANGED) {
                    if (earthling->state != 2 && (object->draw_flags & ENTITY_EITHER_VIEW) &&
                        ((earthling->state ^ next_state) & 1)) {
                        next_state = EARTHLING_STATE_UNCHANGED;
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
        if (next_state == EARTHLING_STATE_UNCHANGED && earthling->state != 2) {
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
        if (next_state != EARTHLING_STATE_UNCHANGED && earthling->state != next_state)
            SetEarthlingState(earthling, next_state);
        if (earthling->timer <= 0)
            earthling->timer = 1;
    }
}

/* Clear the one-update delay, then reverse or step between the observed
 * facing states when the animation ends or the current sprite is blocked. */
void EarthlingAdvanceHorizontalTurnAnimation(earthling)
struct earthling_state *earthling;
{
    short next_state = EARTHLING_STATE_UNCHANGED;
    struct object_state *object;

    if (!earthling->object)
        return;
    if (earthling->state_delay) {
        earthling->state_delay = 0;
        return;
    }
    object = &gPlayerInfo[earthling->object];
    if (!object->sprite_state && earthling->state >= 6)
        next_state = EarthlingSelectTurnState(earthling);
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
    if (next_state != EARTHLING_STATE_UNCHANGED && earthling->state != next_state) {
        SetEarthlingState(earthling, next_state);
        object->frame = SPRITE_FRAME_BEFORE_FIRST;
    }
}

/* Reverse a moving earthling when the two menu flags permit it, and restart
 * its animation whenever the selected state changes. */
void EarthlingThinkHorizontalReversal(earthling)
struct earthling_state *earthling;
{
    short next_state = EARTHLING_STATE_UNCHANGED;
    struct object_state *object;

    if (!earthling->object)
        return;
    if (earthling->state_delay) {
        earthling->state_delay = 0;
        return;
    }
    object = &gPlayerInfo[earthling->object];
    if (!object->sprite_state && earthling->state >= 2)
        next_state = EarthlingSelectTurnState(earthling);
    else if (earthling->state <= 1 && gPlayerInventoryOpen[PLAYER_TOEJAM] != INVENTORY_IDENTIFY && gPlayer2InventoryOpen != INVENTORY_IDENTIFY) {
        if (object->velocity_x > 0) {
            next_state = 2;
            object->velocity_x = -320;
        } else {
            next_state = 3;
            object->velocity_x = 320;
        }
    }
    if (next_state != EARTHLING_STATE_UNCHANGED && earthling->state != next_state) {
        SetEarthlingState(earthling, next_state);
        object->frame = SPRITE_FRAME_BEFORE_FIRST;
    }
}

/* Acquire the whirlwind target mode at timer expiry. State 3 waits for animation and
 * status-bit transitions; other states expire a signed byte phase timer. */
void EarthlingThinkTargetedResetCycle(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    struct earthling_definition *definition;
    short next_state;
    unsigned char minimum_time, maximum_time;

    if (!earthling->object)
        return;
    object = &gPlayerInfo[earthling->object];
    definition = &EarthlingDefinitions[earthling->kind];
    if (earthling->state == 2) {
        if (!object->frame_duration)
            EarthlingRestoreSavedPosition(earthling);
        return;
    }
    if (--earthling->timer < 0) {
        object->velocity_x = 0;
        object->velocity_y = 0;
        next_state = EarthlingSteerTowardNearestTarget(object, definition->target_distance, 0, EARTHLING_WHIRLWIND);
        if (next_state != EARTHLING_STATE_UNCHANGED) {
            minimum_time = definition->target_time_min;
            maximum_time = definition->target_time_max;
            earthling->timer = NEXT_RANDOM % (maximum_time - minimum_time + 1) + minimum_time;
        }
        if (earthling->state == 3) {
            if (object->behavior_flags & ENTITY_HIDDEN) {
                if (next_state != EARTHLING_STATE_UNCHANGED) {
                    object->frame = SPRITE_FRAME_BEFORE_FIRST;
                    object->frame_duration = 1;
                    object->sprite_ready = 0;
                    object->behavior_flags &= ~ENTITY_HIDDEN;
                }
                earthling->timer = 3;
            } else if (!object->frame_duration) {
                SetEarthlingState(earthling, 4);
                earthling->timer = 0;
                earthling->health_visibility[0] = 7;
            } else
                earthling->timer = 1;
        } else if (--earthling->health_visibility[0] < 0 || next_state == EARTHLING_STATE_UNCHANGED) {
            SetEarthlingState(earthling, 2);
            object->frame = SPRITE_FRAME_BEFORE_FIRST;
        }
    }
}
