#include "engine_calls.h"
#include "ram_speech.h"
#include "ram_sprites.h"

/* Test whether an ordinary object is held by one of the four effect slots. */
int IsSpeechBubbleEntity(object_number)
short object_number;
{
    short slot;

    if (object_number >= DYNAMIC_ENTITY_FIRST_SLOT) {
        for (slot = 0; slot < 4; ++slot)
            if (gSpeechBubbleEntities[slot] == object_number)
                return 1;
    }
    return 0;
}
