#include "entities.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Report whether either of the two effect slots is available. */
int func_01bdd0()
{
    if (!ram_ffe1be[0] || !ram_ffe1be[1])
        return 1;
    else
        return 0;
}

/* Claim the first available effect slot and start its animation. */
int func_01bde8(x, y, level)
short x, y, level;
{
    short effect_slot;
    struct object_state *effect;
    struct timed_animation *animation;

    if (!ram_ffe1be[0])
        effect_slot = 0;
    else if (!ram_ffe1be[1])
        effect_slot = 1;
    else
        return 1;
    if (ram_ffe1be[effect_slot] = (int)func_00d904()) {
        effect = &gPlayerInfo[ram_ffe1be[effect_slot]];
        effect->x = x;
        effect->y = y;
        effect->height = 5;
        effect->level = level;
        effect->width = 32;
        effect->height_pixels = 32;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        effect->tile |= 0x8000;
        animation = &dat_0af036;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame_duration = 1;
        effect->frame = -1;
        PlaySound(14, 50);
        return 0;
    }
    return 1;
}

/* Release a slot after its object's animation has stopped. */
void func_01bea6(effect_slot)
short effect_slot;
{
    if (ram_ffe1be[effect_slot] &&
        !gPlayerInfo[ram_ffe1be[effect_slot]].frame_duration) {
        func_00d970(ram_ffe1be[effect_slot]);
        ram_ffe1be[effect_slot] = 0;
    }
}
