#include "entities.h"
#include "audio_calls.h"
#include "player.h"

/* Follow the vertical delta only outside a two-unit dead zone, while always
 * moving horizontally and returning the corresponding facing state. */
char func_01b02e(state, delta_x, delta_y)
struct object_state *state;
short delta_x, delta_y;
{
    if (delta_y < -2)
        state->velocity_y = -256;
    else if (delta_y > 2)
        state->velocity_y = 256;
    else
        state->velocity_y = 0;
    if (delta_x < 0) {
        state->velocity_x = -256;
        return 6;
    } else {
        state->velocity_x = 256;
        return 7;
    }
}

/* Trigger event 9 once when the target has either observed status bit. */
void func_01b078(state, target)
struct object_state *state, *target;
{
    if (!state->flag_57 && (target->flags_52 & 0x40800L)) {
        func_03e4f8(9);
        state->flag_57 = 1;
    }
}
