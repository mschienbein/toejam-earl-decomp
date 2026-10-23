#include "player_animation.h"
#include "player_animation_groups.h"
#include "terrain.h"
#include "entities.h"
#include "player.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Stop horizontal movement, raise a submerged player, and select the
 * directional rocket-skates sprite for the current terrain and height. */
void OpenRocketSkates(state, player, terrain)
struct object_state *state;
short player;
unsigned short terrain;
{
    char sprite = DIRECTION_DOWN;

    StopPlayer(state);
    if (state->height > 0)
        state->acceleration_height = -32;
    else if ((state->animation_group_mask & PLAYER_GROUP_SWIM) || state->sprite_state == PLAYER_ANIM_SWIM_IDLE || state->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE) {
        state->height = -16;
        state->velocity_height = 256;
        state->acceleration_height = 0;
    }
    if (state->animation_group_mask & PLAYER_GROUP_ROCKET_FACING_MOTION)
        sprite = state->sprite_state & DIRECTION_MASK;
    if (terrain == TERRAIN_VOID || state->height > 0)
        sprite += PLAYER_ANIM_SKATES_AIR_UP;
    else if (terrain == TERRAIN_WATER)
        sprite += PLAYER_ANIM_SKATES_WATER_UP;
    else
        sprite += PLAYER_ANIM_SKATES_UP;
    SetSprite(state, sprite);
    state->animation_motion_axes = ANIMATION_MOTION_NONE;
    gPlayerPreviousProcessedInput[player] &= INPUT_BUTTON_MASK;
    if (state->height >= 0)
        state->draw_flags |= ENTITY_SHADOW;
}
