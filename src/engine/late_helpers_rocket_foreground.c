#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Set a foreground rocketship frame and detach the exhaust's active sprites. */
void func_028d54(animation, x, y)
short animation, x, y;
{
    struct object_state *object;

    object = &gPlayerInfo[ram_ffe3de];
    if (object->draw_flags & 2)
        OpeningPresents7(object, 0);
    object->x = x;
    object->y = y;
    object->sprite_ready = 0;
    object->tile |= 0x8000;
    object->sequence = (char *)&IntroHeldFrameStreams[animation];
    object->frame_duration = 1;
    object->frame = -1;
    func_00d776(object, 0);
    object = &gPlayerInfo[ram_ffe3e0];
    if (object->draw_flags & 2)
        OpeningPresents7(object, 0);
}