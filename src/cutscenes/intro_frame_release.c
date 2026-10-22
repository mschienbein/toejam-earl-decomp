#include "core_calls.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Release ten intro objects, then refresh the sprite state. */
void IntroReleaseFrameEntitiesAndRefresh()
{
    short index, object_number;

    for (index = 0; index < 10; ++index) {
        if (object_number = gIntroComponentEntities[index])
            FreeEntity(object_number);
        gIntroComponentEntities[index] = 0;
    }
    gSpriteGraphicsUploadPending = 0;
    UpdateAndQueueEntitySprites();
    WaitForNextVBlank(1);
    UploadQueuedSpriteGraphics(0, 0);
}