#include "entities.h"
#include "ram_player.h"
#include "ram_intro.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "sprite_calls.h"

/* Set up the parent collision object and three attached objects. */
void IntroCreateCollisionSceneEntities()
{
    unsigned short index;
    struct object_state *parent, *object;

    if (gIntroCollisionParentEntity = (int)AllocateEntity()) {
        parent = &gPlayerInfo[gIntroCollisionParentEntity];
        parent->tile |= 0x8000;
        parent->current_frame = Spr_IntroCollision[0];
        parent->frames = Spr_IntroCollision;
        parent->tile_count = 160;
        parent->sprite_count = 12;
        parent->x = 16;
        parent->y = 97;
        parent->height = 1;
        parent->sequence = (char *)&IntroHeldFrameStreams[1];
        parent->frame_duration = 1;
        parent->frame = -1;
        parent->frame_phase = 6;
        parent->sprite_ready = 0;
        AllocateEntityViewportSprites(parent, 0);
    }
    for (index = 0; index < 3; ++index) {
        if (gIntroCollisionAttachedEntities[index] = (int)AllocateEntity()) {
            object = &gPlayerInfo[gIntroCollisionAttachedEntities[index]];
            object->tile |= 0x8000;
            object->current_frame = Spr_IntroCollision[0];
            object->frames = Spr_IntroCollision;
            object->tile_count = 160;
            object->sprite_count = 12;
            if (index == 0) {
                object->x = 16;
                object->y = 208;
            } else if (index == 1) {
                object->x = 176;
                object->y = 96;
            } else {
                object->x = 176;
                object->y = 208;
            }
            ++object->y;
            object->height = 1;
            object->sequence = (char *)&IntroHeldFrameStreams[1];
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            object->flags_4d |= 4;
            /* ORIGINAL BUG: parent is uninitialized if its allocation failed
             * but an attached object's allocation succeeds. */
            object->tile = parent->tile;
            object->parent = gIntroCollisionParentEntity;
            object->sprite_ready = 0;
            AllocateEntityViewportSprites(object, 0);
        }
    }
}

/* Release the collision object and the two three-object lists. */
void IntroReleaseCollisionSceneEntities()
{
    unsigned short index;

    if (gIntroCollisionParentEntity)
        FreeEntity(gIntroCollisionParentEntity);
    gIntroCollisionParentEntity = 0;
    for (index = 0; index < 3; ++index) {
        if (gIntroCollisionAuxiliaryEntities[index])
            FreeEntity(gIntroCollisionAuxiliaryEntities[index]);
        gIntroCollisionAuxiliaryEntities[index] = 0;
        if (gIntroCollisionAttachedEntities[index])
            FreeEntity(gIntroCollisionAttachedEntities[index]);
        gIntroCollisionAttachedEntities[index] = 0;
    }
}
