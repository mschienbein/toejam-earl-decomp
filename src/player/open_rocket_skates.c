#include "entities.h"
#include "ram.h"
#include "player.h"
#include "sprite_calls.h"
/* Stop horizontal movement, raise a submerged player, and select the
 * directional rocket-skates sprite for the current terrain and height. */
void OpenRocketSkates(state, player, terrain)
struct object_state *state;
short player;
unsigned short terrain;
{
    char sprite = 1;

    StopPlayer(state);
    if (state->height > 0)
        state->acceleration_height = -32;
    else if ((state->flags_52 & 0x800) || state->sprite_state == 3 || state->sprite_state == 7) {
        state->height = -16;
        state->velocity_height = 256;
        state->acceleration_height = 0;
    }
    if (state->flags_52 & 0x07b5fffc)
        sprite = state->sprite_state & 3;
    if (terrain == 0x90 || state->height > 0)
        sprite += 104;
    else if (terrain == 0x80)
        sprite += 100;
    else
        sprite += 92;
    SetSprite(state, sprite);
    state->mode_15 = 0;
    P1_PREV_PROCESSED_INPUT[player] &= 0xf0;
    if (state->height >= 0)
        state->draw_flags |= 0x40;
}
