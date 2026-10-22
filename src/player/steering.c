#include "entities.h"
#include "player.h"

/* The first three words are position; the next two are horizontal motion.
 * [INFERENCE] The returned values 4..7 select facing directions. */

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
