#include "entities.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "random.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Move the first flying-object slot ten pixels right. A nonzero restart
 * selects one of the held-frame variants for the requested scene. */
void IntroUpdateRightwardFlyingEntity(restart, scene)
short restart, scene;
{
    struct object_state *object;

    if (!scene)
        return;
    object = &gPlayerInfo[gIntroFlyingEntities[0]];
    if (restart) {
        if (object->draw_flags & 3)
            ReleaseEntityViewportSprites(object, 0);
        if (scene == 1) {
            object->y = 94;
            object->sequence = (char *)&IntroHeldFrameStreams[(int)GetRandomNumber(0) % 3];
            object->height = 0;
        } else if (scene == 2) {
            object->y = 192;
            object->height = 80;
            object->sequence = (char *)&IntroHeldFrameStreams[((int)GetRandomNumber() & 1) + 3];
        }
        object->frames = IntroFlyingEntityFrames;
        object->frame_phase = 6;
        object->frame_duration = 1;
        object->frame = -1;
        object->sprite_ready = 0;
        object->x = 96;
        AllocateEntityViewportSprites(object, 0);
    }
    object->x += 10;
    if (object->x > 232 && (object->draw_flags & 3))
        ReleaseEntityViewportSprites(object, 0);
}

/* Update the four flying-object slots, respawning objects outside the
 * observed x/height bounds and restarting their animation at zero delay. */
void IntroUpdateFlyingEntityMotion(scene)
short scene;
{
    short object_number, slot, spawn_x, drift_x, variant;
    struct object_state *object;

    switch (scene) {
    case 0:
        for (object_number = 0; object_number < 4; object_number++) {
            if (slot = gIntroFlyingEntities[object_number])
                ReleaseEntityViewportSprites(&gPlayerInfo[slot], 0);
        }
        return;
    case 2:
        spawn_x = 248;
        drift_x = -12;
        break;
    case 3:
        spawn_x = 88;
        drift_x = 12;
        break;
    case 1:
    case 4:
    default:
        spawn_x = 168;
        drift_x = 0;
        break;
    }
    for (object_number = 0; object_number < 4; object_number++) {
        if (slot = gIntroFlyingEntities[object_number]) {
            object = &gPlayerInfo[slot];
            if (object->x < 50 || object->x > 270 ||
                object->height > -18 || object->height < -198) {
                if (object->draw_flags & ENTITY_PRIMARY_VIEW)
                    ReleaseEntityViewportSprites(object, 0);
                object->x = spawn_x;
                object->y = 0;
                object->height = -100;
                if (scene == 4) {
                    object->sequence = IntroFlyingSceneFourAnimation;
                    object->frames = IntroFlyingEntityFrames;
                    object->velocity_x = 0;
                    object->velocity_y = 0;
                    object->velocity_height = -1;
                } else {
                    object->velocity_x = (int)GetRandomNumber() % 4;
                    object->velocity_y = 4 - object->velocity_x;
                    if ((int)GetRandomNumber() & 1)
                        object->velocity_x = -object->velocity_x;
                    if ((int)GetRandomNumber() & 1)
                        object->velocity_y = -object->velocity_y;
                    object->velocity_height = -(int)GetRandomNumber() % 12;
                    variant = (int)GetRandomNumber() & 1;
                    /* The mask only produces 0/1, but comparisons against
                     * 2/3 and their alternate frame table are in the ROM. */
                    if (variant == 0) {
                        object->sequence = IntroFlyingForwardFramesAnimation;
                        object->frames = IntroFlyingEntityFrames;
                    } else if (variant == 1) {
                        object->sequence = IntroFlyingFlippedFramesAnimation;
                        object->frames = IntroFlyingEntityFrames;
                    } else if (variant == 2) {
                        object->sequence = IntroFlyingFlippedFramesAnimation;
                        object->frames = IntroFlyingAlternateEntityFrames;
                    } else if (variant == 3) {
                        object->sequence = IntroFlyingForwardFramesAnimation;
                        object->frames = IntroFlyingAlternateEntityFrames;
                    }
                }
                object->current_frame = object->frames[0];
                object->frame_phase = 5;
                object->frame_duration = 1;
                object->frame = -1;
                object->sprite_ready = 0;
            } else {
                object->y++;
                object->height++;
            }
            object->x += object->velocity_x + drift_x;
            object->height += object->velocity_y;
            object->velocity_height++;
            if (object->velocity_height == 0) {
                AllocateEntityViewportSprites(object, 0);
                object->sprite_ready = 0;
            }
        }
    }
}
