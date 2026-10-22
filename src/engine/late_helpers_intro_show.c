#include "entities.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"

/* Clear the hidden flag on each of the ten intro objects. */
void func_028af0()
{
    short index, object_number;
    struct object_state *object;

    for (index = 0; index < 10; ++index) {
        object_number = ram_ffe3ce[index];
        object = &gPlayerInfo[object_number];
        object->flags_4d &= ~64;
    }
}