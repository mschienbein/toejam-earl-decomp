#include "entities.h"
#include "assets_cutscene.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Allocate the rocketship impact object at the supplied position. */
void func_0298b0(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3f2 = (int)func_00d904()) {
        object = &gPlayerInfo[ram_ffe3f2];
        object->tile = 0;
        object->current_frame = Spr_IntroRocketshipImpact[0];
        object->frames = Spr_IntroRocketshipImpact;
        object->tile_count = 8;
        object->sprite_count = 1;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)IntroFrameIndexLists;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Release the impact slot without clearing its stored index. */
void func_029930()
{
    if (ram_ffe3f2)
        func_00d970(ram_ffe3f2);
}

/* Allocate the planet object at the supplied position. */
void func_02994c(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3f4 = (int)func_00d904()) {
        object = &gPlayerInfo[ram_ffe3f4];
        object->tile = 0;
        object->current_frame = &Spr_PlanetFrame00;
        object->frames = Spr_Planet;
        object->tile_count = 72;
        object->sprite_count = 6;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Release the planet slot without clearing its stored index. */
void func_0299cc()
{
    if (ram_ffe3f4)
        func_00d970(ram_ffe3f4);
}

/* Allocate the ending composite at the supplied position. */
void func_0299e8(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3f6 = (int)func_00d904()) {
        object = &gPlayerInfo[ram_ffe3f6];
        object->tile = 0;
        /* ORIGINAL BUG: current_frame receives the frame-table address,
         * not its first frame; dereferencing the table does not match. */
        object->current_frame = CompositeFrames_0389D0;
        object->frames = CompositeFrames_0389D0;
        object->tile_count = 72;
        object->sprite_count = 6;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Release the ending composite slot without clearing its stored index. */
void func_029a68()
{
    if (ram_ffe3f6)
        func_00d970(ram_ffe3f6);
}
