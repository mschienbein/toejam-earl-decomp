#define TJE_LOCAL_FUNC_00D904

#include "entities.h"
#include "sprites.h"
#include "assets_cutscene.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int func_00d904();

/* Select ending-character sequences according to which players have lives. */
void func_028844(x, y)
unsigned short x, y;
{
    short second_active, first_active;
    struct object_state *object;
    struct render_frame **frames;

    second_active = !(gPlayer2Lives < 0);
    first_active = !(gPlayerLives[0] < 0);
    if (first_active) {
        if (ram_ffe3aa = func_00d904()) {
            object = &gPlayerInfo[ram_ffe3aa];
            frames = CompositeFrames_03A3AA;
            object->sprite_count = 3;
            object->tile_count = 48;
            object->tile = 0x8000;
            object->frames = frames;
            if (second_active)
                object->sequence = dat_03855d;
            else
                object->sequence = dat_0384e6;
            object->x = x;
            object->y = y + 100;
            object->height = 100;
            object->sprite_ready = 0;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            func_00d776(object, 0);
        }
    }
    if (second_active) {
        if (ram_ffe3ac = func_00d904()) {
            object = &gPlayerInfo[ram_ffe3ac];
            frames = CompositeFrames_03A33A;
            object->sprite_count = 3;
            object->tile_count = 48;
            object->tile = 0x8000;
            object->frames = frames;
            if (first_active)
                object->sequence = dat_038521;
            else
                object->sequence = EndingFrameIndexStreams;
            object->x = x;
            object->y = y + 100;
            object->height = 100;
            object->sprite_ready = 0;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            func_00d776(object, 0);
        }
    }
}