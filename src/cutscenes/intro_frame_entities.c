#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "assets_scenes.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* The bounded payload view uses the entity's twelve sprite slots as a
 * capacity, not a fixed extent for every frame after its four-byte prefix. */

/* Initialize ten intro objects from a frame table and sum each frame's tiles. */
void IntroCreateHiddenFrameEntities()
{
    short index, object_number, piece_number;
    struct render_frame **frames;
    struct render_frame *frame;
    struct object_state *object;

    frames = IntroAndShipAnimationPointers[1];
    for (index = 0; index < 10; ++index) {
        frame = frames[index];
        if (object_number = AllocateEntity()) {
            object = &gPlayerInfo[object_number];
            object->sprite_count = frame->piece_count;
            object->tile_count = 0;
            for (piece_number = 0; piece_number < frame->piece_count; ++piece_number)
                object->tile_count += ((struct render_piece (*)[12])(frame + 1))[0][piece_number].width *
                                      ((struct render_piece (*)[12])(frame + 1))[0][piece_number].height;
            object->current_frame = frame;
            object->x = 170;
            object->y = 160;
            object->tile |= VDP_TILE_PRIORITY;
            object->behavior_flags |= ENTITY_HIDDEN;
            AllocateEntityViewportSprites(object, 0);
            object->sprite_ready = 0;
            object->sequence = (char *)&IntroHeldFrameStreams[index];
            object->frames = frames;
            object->frame_phase = 1;
            object->frame_duration = 1;
            object->frame = -1;
        }
        gIntroComponentEntities[index] = object_number;
    }
}