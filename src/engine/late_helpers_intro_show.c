#include "entities.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"

/* Clear the hidden flag on each of the ten intro objects. */
void IntroRevealFrameEntities()
{
    short index, object_number;
    struct object_state *object;

    for (index = 0; index < 10; ++index) {
        object_number = gIntroComponentEntities[index];
        object = &gPlayerInfo[object_number];
        object->flags_4d &= ~64;
    }
}