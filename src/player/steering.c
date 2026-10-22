#include "entities.h"
#include "player.h"
#include "random.h"

char func_01af04(state, delta_x, delta_y)
struct object_state *state;
short delta_x, delta_y;
{
    char direction;

    direction = 0;
    state->velocity_y = 0;
    state->velocity_x = 0;
    if (delta_y < 0) {
        state->velocity_y = -256;
        delta_y = -delta_y;
    } else if (delta_y > 0)
        state->velocity_y = 256;
    if (delta_x < 0) {
        state->velocity_x = -256;
        delta_x = -delta_x;
    } else if (delta_x > 0)
        state->velocity_x = 256;
    if (state->velocity_x && state->velocity_y) {
        if (delta_y > 5 && delta_x < 20)
            state->velocity_x = 0;
        else if (delta_x > 20 && delta_y < 5)
            state->velocity_y = 0;
    }
    if (state->velocity_y < 0)
        direction = 4;
    else if (state->velocity_y > 0)
        direction = 5;
    if (state->velocity_x < 0)
        direction = 6;
    else if (state->velocity_x > 0)
        direction = 7;
    return direction;
}

char func_01afb8(state, target, delta_x, delta_y)
struct object_state *state, *target;
short delta_x, delta_y;
{
    short previous_x;
    char facing;

    previous_x = state->velocity_x;
    /* QUALIFICATION: retain the discarded facing result in a named local.
     * MCC68K then flushes argument cleanup before the following velocity test;
     * ignoring the return emits the same two instructions in reverse order. */
    facing = func_01af04(state, delta_x, delta_y);
    if (!state->velocity_x && state->velocity_y) {
        if (previous_x)
            state->velocity_x = previous_x;
        else if (GetRandomNumber() & 1)
            state->velocity_x = 256;
        else
            state->velocity_x = -256;
    }
    return (char)(state->x > target->x ? 6 : 7);
}
