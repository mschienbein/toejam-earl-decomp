#include "intro_object_calls.h"
#include "ram_intro.h"
#include "sprite_calls.h"

/* Release exhaust before the rocketship object. */
void CutsceneReleaseRocketshipPair()
{
    if (gCutsceneRocketshipExhaustEntity)
        FreeEntity(gCutsceneRocketshipExhaustEntity);
    gCutsceneRocketshipExhaustEntity = 0;
    if (gCutsceneRocketshipEntity)
        FreeEntity(gCutsceneRocketshipEntity);
    gCutsceneRocketshipEntity = 0;
}