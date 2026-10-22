#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Position both rocket objects and select their corresponding frame streams. */
void CutsceneSetRocketshipPairPose(animation, x, y)
short animation, x, y;
{
    struct object_state *object;

    object = &gPlayerInfo[gCutsceneRocketshipEntity];
    if (object->draw_flags & ENTITY_PRIMARY_VIEW)
        ReleaseEntityViewportSprites(object, 0);
    object->x = x;
    object->y = y;
    object->sprite_ready = 0;
    object->sequence = (char *)&IntroHeldFrameStreams[animation];
    object->frame_duration = 1;
    object->frame = -1;
    AllocateEntityViewportSprites(object, 0);
    object = &gPlayerInfo[gCutsceneRocketshipExhaustEntity];
    if (object->draw_flags & ENTITY_PRIMARY_VIEW)
        ReleaseEntityViewportSprites(object, 0);
    object->x = x;
    object->y = y;
    object->sprite_ready = 0;
    object->sequence = IntroAnimationPointers[animation];
    object->frame_duration = 1;
    object->frame = -1;
    AllocateEntityViewportSprites(object, 0);
}