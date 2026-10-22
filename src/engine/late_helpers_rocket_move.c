#include "entities.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"

/* Move rocketship and exhaust left, wrapping each at x = -60. */
void func_028df2(distance)
long distance;
{
    struct object_state *object;

    object = &gPlayerInfo[ram_ffe3de];
    object->x -= distance;
    if (object->x < -60)
        object->x = 320;
    object = &gPlayerInfo[ram_ffe3e0];
    --object->x;
    if (object->x < -60)
        object->x = 320;
}