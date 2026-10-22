#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "core_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "ram_system.h"
#include "reset_calls.h"
#include "sprite_calls.h"

/* Twelve is the entity's sprite-list capacity, not a claimed fixed frame
 * extent. Each frame's four-byte prefix is followed by its counted pieces;
 * the bounded payload view retains the existing word-index arithmetic. */

/* Allocate eight actor records and total each selected frame's tile area. */
void IntroAllocateLargePlayerParts()
{
    short index, object_number, piece;
    struct render_frame *frame;
    struct object_state *entity;

    for (index = 0; index < 8; ++index) {
        frame = Spr_TitleBigPlayers[IntroLargePlayerPartInitialFrameIndices[index]];
        if (object_number = (int)AllocateEntity()) {
            entity = &gPlayerInfo[object_number];
            entity->sprite_count = frame->count;
            entity->tile_count = 0;
            for (piece = 0; piece < frame->count; ++piece)
                entity->tile_count += ((struct render_piece (*)[12])(frame + 1))[0][piece].width *
                                      ((struct render_piece (*)[12])(frame + 1))[0][piece].height;
            entity->tile = VDP_TILE_PRIORITY;
            entity->current_frame = frame;
        }
        gIntroLargePlayerPartEntities[index] = object_number;
    }
}

/* Release the eight actor records and clear every selector slot. */
void IntroReleaseLargePlayerParts()
{
    short index, object_number;

    for (index = 0; index < 8; ++index) {
        if (object_number = gIntroLargePlayerPartEntities[index])
            FreeEntity(object_number);
        gIntroLargePlayerPartEntities[index] = 0;
    }
}

/* Position an actor and select its held-frame animation stream. */
void IntroPositionLargePlayerPart(index, x, y)
short index, x, y;
{
    short object_number;
    struct render_frame *frame;
    struct object_state *entity;

    if (index >= 8)
        return;
    /* MATCHING QUALIFICATION: the ROM loads this table entry and never uses it;
     * the assigned-but-unused local reproduces that read (dropping it, or
     * discarding it through a void cast, changes the code in both revisions).
     * The historical spelling is unknown; no gameplay effect. */
    frame = Spr_TitleBigPlayers[index];
    object_number = gIntroLargePlayerPartEntities[index];
    entity = &gPlayerInfo[object_number];
    entity->x = x;
    entity->y = y;
    if (index == 3) {
        entity->y += 5;
        entity->height = 5;
    }
    entity->sequence = (char *)&IntroLargePlayerPartHeldFrameStreams[index];
    entity->frames = Spr_TitleBigPlayers;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->frame = -1;
    AllocateEntityViewportSprites(entity, 0);
    entity->sprite_ready = 0;
}

/* Replace one actor's sequence and restart its frame timer. */
void IntroSetLargePlayerPartSequence(index, sequence, period)
short index;
char *sequence;
char period;
{
    struct object_state *entity;

    entity = &gPlayerInfo[gIntroLargePlayerPartEntities[index]];
    entity->sequence = sequence;
    entity->frame_phase = period;
    entity->frame_duration = 1;
    entity->frame = -1;
}

/* Release the primary viewport state of an allocated actor. */
void IntroReleaseLargePlayerPartSprites(index)
short index;
{
    struct object_state *entity;

    if (index >= 8)
        return;
    entity = &gPlayerInfo[gIntroLargePlayerPartEntities[index]];
    if (!(entity->draw_flags & ENTITY_ACTIVE))
        return;
    ReleaseEntityViewportSprites(entity, 0);
}

/* Place the seven visible actors in selector order. */
void IntroPositionLargePlayerParts()
{
    IntroPositionLargePlayerPart(0, 148, 112);
    IntroPositionLargePlayerPart(4, 148, 112);
    IntroPositionLargePlayerPart(1, 148, 112);
    IntroPositionLargePlayerPart(2, 148, 112);
    IntroPositionLargePlayerPart(5, 148, 112);
    IntroPositionLargePlayerPart(6, 148, 112);
    IntroPositionLargePlayerPart(7, 148, 112);
}

/* Keep the selector order: the repeated target is hoisted into a preserved
 * address register, and the original combines the argument stack cleanups. */
void IntroReleaseLargePlayerSpritesAndRefresh()
{
    IntroReleaseLargePlayerPartSprites(0);
    IntroReleaseLargePlayerPartSprites(4);
    IntroReleaseLargePlayerPartSprites(1);
    IntroReleaseLargePlayerPartSprites(2);
    IntroReleaseLargePlayerPartSprites(5);
    IntroReleaseLargePlayerPartSprites(6);
    IntroReleaseLargePlayerPartSprites(7);
    VBLANK_FLAG_2 = 0;
    UpdateAndQueueEntitySprites();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
}

/* Allocate the three driving-scene actors and total their frame tile areas. */
void IntroAllocateDrivingParts()
{
    short index, object_number, piece;
    struct render_frame *frame;
    struct object_state *entity;

    for (index = 0; index < 3; ++index) {
        frame = Spr_IntroEarlDriving[IntroDrivingInitialFrameIndices[index]];
        if (object_number = (int)AllocateEntity()) {
            entity = &gPlayerInfo[object_number];
            entity->sprite_count = frame->count;
            entity->tile_count = 0;
            for (piece = 0; piece < frame->count; ++piece)
                entity->tile_count += ((struct render_piece (*)[12])(frame + 1))[0][piece].width *
                                      ((struct render_piece (*)[12])(frame + 1))[0][piece].height;
            entity->tile = VDP_TILE_PRIORITY;
            entity->current_frame = frame;
        }
        gIntroDrivingPartEntities[index] = object_number;
    }
}

/* Release all driving-scene actor records and clear their selector slots. */
void IntroReleaseDrivingParts()
{
    short index, object_number;

    for (index = 0; index < 3; ++index) {
        if (object_number = gIntroDrivingPartEntities[index])
            FreeEntity(object_number);
        gIntroDrivingPartEntities[index] = 0;
    }
}

/* Position a driving-scene actor and select its held-frame stream. */
void IntroPositionDrivingPart(index, x, y)
short index, x, y;
{
    short object_number;
    struct render_frame *frame;
    struct object_state *entity;

    if (index >= 3)
        return;
    /* MATCHING QUALIFICATION: the ROM loads this table entry and never uses it;
     * the assigned-but-unused local reproduces that read (see IntroPositionLargePlayerPart). */
    frame = Spr_IntroEarlDriving[index];
    object_number = gIntroDrivingPartEntities[index];
    entity = &gPlayerInfo[object_number];
    entity->x = x;
    entity->y = y;
    if (index == 1) {
        entity->y += 100;
        entity->height += 100;
    } else if (index == 2) {
        entity->y += 101;
        entity->height += 101;
    }
    entity->sequence = (char *)&IntroDrivingPartHeldFrameStreams[index];
    entity->frames = Spr_IntroEarlDriving;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->frame = -1;
    AllocateEntityViewportSprites(entity, 0);
    entity->sprite_ready = 0;
}

/* Replace a driving actor's sequence and restart its frame timer. */
void IntroSetDrivingPartSequence(index, sequence, period)
short index;
char *sequence;
char period;
{
    struct object_state *entity;

    entity = &gPlayerInfo[gIntroDrivingPartEntities[index]];
    entity->sequence = sequence;
    entity->frame_phase = period;
    entity->frame_duration = 1;
    entity->frame = -1;
}

/* Release the primary viewport state when either low flag bit is set. */
void IntroReleaseDrivingPartSprites(index)
short index;
{
    struct object_state *entity;

    if (index >= 3)
        return;
    entity = &gPlayerInfo[gIntroDrivingPartEntities[index]];
    if (!(entity->draw_flags & 3))
        return;
    ReleaseEntityViewportSprites(entity, 0);
}

/* Create and initialize the asteroid actor used in the driving scene. */
void IntroCreateAsteroidEntity()
{
    struct object_state *entity;

    if (gIntroAsteroidEntity = (int)AllocateEntity()) {
        entity = &gPlayerInfo[gIntroAsteroidEntity];
        entity->tile = 0;
        entity->current_frame = Spr_IntroAsteroid[0];
        entity->frames = Spr_IntroAsteroid;
        entity->tile_count = 160;
        entity->sprite_count = 12;
        entity->x = 104;
        entity->y = 151;
        entity->sprite_ready = 0;
        entity->sequence = IntroAsteroidLoopAnimation;
        entity->frame_duration = 1;
        entity->frame = -1;
        entity->frame_phase = 6;
        AllocateEntityViewportSprites(entity, 0);
    }
}

/* Release the asteroid actor and clear its stored object identifier. */
void IntroReleaseAsteroidEntity()
{
    if (gIntroAsteroidEntity)
        FreeEntity(gIntroAsteroidEntity);
    gIntroAsteroidEntity = 0;
}