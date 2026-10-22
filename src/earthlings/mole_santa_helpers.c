#define TJE_LOCAL_FUNC_00D904

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "earthling_calls.h"
#include "ram_earthlings.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: a short declaration adds MOVE.W D0,D0 before the byte store;
 * the observed caller uses the allocator's low byte without that conversion. */
extern int func_00d904();

/* Allocate a six-slot effect and copy its position and level from the mole. */
void func_02234c(earthling, mole)
struct earthling_state *earthling;
struct object_state *mole;
{
    short slot;
    struct object_state *effect;
    struct animation_descriptor *animation;

    for (slot = 0; slot < 6; ++slot) {
        if (!ram_ffe244[slot])
            break;
    }
    if (slot == 6)
        return;
    if (ram_ffe244[slot] = func_00d904()) {
        effect = &gPlayerInfo[ram_ffe244[slot]];
        effect->x = mole->x;
        effect->y = mole->y;
        effect->level = mole->level;
        effect->width = 16;
        effect->height_pixels = 8;
        effect->sprite_count = 1;
        effect->tile_count = 8;
        effect->tile = 0x8000;
        animation = &AnimationDescriptors_0AF094[0];
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = 6;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->draw_flags |= 128;
    }
}

/* Release completed mole effects and presents whose height reaches zero. */
void func_0223f6()
{
    short slot, object_number;

    for (slot = 0; slot < 6; ++slot) {
        if ((object_number = ram_ffe244[slot]) &&
            !gPlayerInfo[object_number].frame_duration) {
            func_00d970(object_number);
            ram_ffe244[slot] = 0;
        }
    }
    for (slot = 0; slot <= 1; ++slot) {
        if ((object_number = ram_ffe24a[slot]) &&
            gPlayerInfo[object_number].height <= 0) {
            func_00d970(object_number);
            ram_ffe24a[slot] = 0;
        }
    }
}
