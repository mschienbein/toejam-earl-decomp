#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Position both rocket objects and select their corresponding frame streams. */
void func_028c8a(animation, x, y)
short animation, x, y;
{
    struct object_state *object;

    object = &gPlayerInfo[ram_ffe3de];
    if (object->draw_flags & 2)
        OpeningPresents7(object, 0);
    object->x = x;
    object->y = y;
    object->sprite_ready = 0;
    object->sequence = (char *)&IntroHeldFrameStreams[animation];
    object->frame_duration = 1;
    object->frame = -1;
    func_00d776(object, 0);
    object = &gPlayerInfo[ram_ffe3e0];
    if (object->draw_flags & 2)
        OpeningPresents7(object, 0);
    object->x = x;
    object->y = y;
    object->sprite_ready = 0;
    object->sequence = IntroAnimationPointers[animation];
    object->frame_duration = 1;
    object->frame = -1;
    func_00d776(object, 0);
}