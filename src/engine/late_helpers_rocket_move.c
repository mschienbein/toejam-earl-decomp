#include "entities.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"

/* Move rocketship and exhaust left, wrapping each at x = -60. */
void CutsceneMoveRocketshipPairLeft(distance)
long distance;
{
    struct object_state *object;

    object = &gPlayerInfo[gCutsceneRocketshipEntity];
    object->x -= distance;
    if (object->x < -60)
        object->x = 320;
    object = &gPlayerInfo[gCutsceneRocketshipExhaustEntity];
    --object->x;
    if (object->x < -60)
        object->x = 320;
}