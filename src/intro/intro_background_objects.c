#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "random.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Allocate eight drifting objects, summing each frame's tile dimensions.
 * Their y range excludes the open interval 150..170. */
void CutsceneCreateDriftingEntities()
{
    short object_number, piece_number, slot;
    struct object_state *object;
    struct render_frame *frame;

    for (object_number = 0; object_number < 8; object_number++) {
        frame = CutsceneDriftingEntityFrames[object_number];
        if (slot = AllocateEntity()) {
            object = &gPlayerInfo[slot];
            object->sprite_count = frame->count;
            object->tile_count = 0;
            /* Counts are 2,4,1,1,1,1,1,1: four is this consumer's
             * maximum payload bound, not a fixed complete-frame stride. */
            for (piece_number = 0; piece_number < frame->count; piece_number++)
                object->tile_count +=
                    ((struct render_piece (*)[4])(frame + 1))[0][piece_number].width *
                    ((struct render_piece (*)[4])(frame + 1))[0][piece_number].height;
            object->tile = 0;
            object->current_frame = frame;
            object->x = (int)GetRandomNumber() % 320 + (-70);
            object->velocity_x = ((int)GetRandomNumber() & 1) + 1;
            do {
                object->y = (int)GetRandomNumber() % 148 + 60;
            } while (object->y > 150 && object->y < 170);
            object->sequence = (char *)&IntroHeldFrameStreams[object_number];
            object->frames = CutsceneDriftingEntityFrames;
            object->frame_phase = 1;
            object->frame_duration = 1;
            object->frame = -1;
            AllocateEntityViewportSprites(object, 0);
            object->sprite_ready = 0;
        }
        gCutsceneDriftingEntities[object_number] = slot;
    }
}

/* Release and clear each of the eight drifting-object slots. */
void CutsceneReleaseDriftingEntities()
{
    short object_number, slot;

    for (object_number = 0; object_number < 8; object_number++) {
        if (slot = gCutsceneDriftingEntities[object_number])
            FreeEntity(slot);
        gCutsceneDriftingEntities[object_number] = 0;
    }
}

/* Advance each drifting object when its fractional timer exceeds its
 * velocity word, wrapping x beyond 370 back into -70..-41. */
void CutsceneUpdateDriftingEntities(scene)
short scene;
{
    short object_number, slot;
    struct object_state *object;

    for (object_number = 0; object_number < 8; object_number++) {
        if (slot = gCutsceneDriftingEntities[object_number]) {
            object = &gPlayerInfo[slot];
            object->fraction_x++;
            if (object->fraction_x > object->velocity_x) {
                object->x++;
                object->fraction_x = 0;
                if (object->x > 370) {
                    object->x = (int)GetRandomNumber() % 30 + (-70);
                    object->velocity_x = ((int)GetRandomNumber() & 1) + 1;
                    do {
                        object->y = (int)GetRandomNumber() % 148 + 60;
                    } while (object->y > 150 && object->y < 170);
                }
            }
        }
    }
}

/* Allocate four objects with four sprites and forty tiles each. */
void IntroAllocateFlyingEntitySlots()
{
    short object_number, slot;
    struct object_state *object;

    for (object_number = 0; object_number < 4; object_number++) {
        if (slot = AllocateEntity()) {
            object = &gPlayerInfo[slot];
            object->sprite_count = 4;
            object->tile_count = 40;
            object->tile = 0;
        }
        gIntroFlyingEntities[object_number] = slot;
    }
}

/* Release and clear each of the four flying-object slots. */
void IntroReleaseFlyingEntitySlots()
{
    short object_number, slot;

    for (object_number = 0; object_number < 4; object_number++) {
        if (slot = gIntroFlyingEntities[object_number])
            FreeEntity(slot);
        gIntroFlyingEntities[object_number] = 0;
    }
}
