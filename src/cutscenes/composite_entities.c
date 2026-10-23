#include "hardware.h"
#define TJE_LOCAL_ALLOCATEENTITY

#include "entities.h"
#include "assets_cutscene.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int AllocateEntity();

/* Set both selected objects to the same position. */
void CutscenePositionRocketshipPair(x, y)
int x, y;
{
    struct object_state *object;

    object = &gPlayerInfo[gCutsceneRocketshipEntity];
    object->x = x;
    object->y = y;
    object = &gPlayerInfo[gCutsceneRocketshipExhaustEntity];
    object->x = x;
    object->y = y;
}

/* Allocate the first composite object and start its held-frame stream. */
void EndingCreatePoseComposite()
{
    struct object_state *object;

    if (gEndingPoseCompositeEntity = AllocateEntity()) {
        object = &gPlayerInfo[gEndingPoseCompositeEntity];
        object->tile = 0;
        object->current_frame = EndingPoseCompositeFrames[0];
        object->frames = EndingPoseCompositeFrames;
        object->tile_count = 30;
        object->sprite_count = 3;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= VDP_TILE_PRIORITY;
        object->sequence = &IntroHeldFrameStreams[0].frame;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Position the composite five pixels before the supplied coordinates and
 * select a two-byte held-frame stream. */
void EndingSetPoseComposite(frame, x, y)
short frame, x, y;
{
    struct object_state *object;

    if (gEndingPoseCompositeEntity) {
        object = &gPlayerInfo[gEndingPoseCompositeEntity];
        object->x = x - 5;
        object->y = y - 5;
        object->tile |= VDP_TILE_PRIORITY;
        object->draw_flags |= ENTITY_DRAW_LATE;
        object->sequence = &IntroHeldFrameStreams[frame].frame;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
    }
}

/* Release the selected composite and clear its allocation slot. */
void EndingReleasePoseComposite()
{
    if (gEndingPoseCompositeEntity)
        FreeEntity(gEndingPoseCompositeEntity);
    gEndingPoseCompositeEntity = 0;
}

/* Allocate the second composite with its held frame and tile attributes. */
void EndingCreateSixPieceComposite()
{
    struct object_state *object;

    if (gEndingSixPieceCompositeEntity = AllocateEntity()) {
        object = &gPlayerInfo[gEndingSixPieceCompositeEntity];
        object->current_frame = EndingSixPieceCompositeFrames[0];
        object->frames = EndingSixPieceCompositeFrames;
        object->tile_count = 69;
        object->sprite_count = 6;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= VDP_TILE_PRIORITY;
        object->sequence = &IntroHeldFrameStreams[0].frame;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Place the second composite at the requested x and 100 pixels below y. */
void EndingPositionSixPieceComposite(x, y)
short x, y;
{
    struct object_state *object;

    if (gEndingSixPieceCompositeEntity) {
        object = &gPlayerInfo[gEndingSixPieceCompositeEntity];
        object->x = x;
        object->y = y + 100;
        object->height = 100;
    }
}

/* Release the second composite and clear its allocation slot. */
void EndingReleaseSixPieceComposite()
{
    if (gEndingSixPieceCompositeEntity)
        FreeEntity(gEndingSixPieceCompositeEntity);
    gEndingSixPieceCompositeEntity = 0;
}

/* Allocate the ending composite with the first ending frame-index stream. */
void EndingCreateRightOffsetComposite()
{
    struct object_state *object;

    if (gEndingRightOffsetCompositeEntity = AllocateEntity()) {
        object = &gPlayerInfo[gEndingRightOffsetCompositeEntity];
        object->tile = 0;
        object->current_frame = EndingRightOffsetCompositeFrames[0];
        object->frames = EndingRightOffsetCompositeFrames;
        object->tile_count = 43;
        object->sprite_count = 4;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= VDP_TILE_PRIORITY;
        object->sequence = &EndingFrameIndexStreams[242];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Offset the ending composite 50 pixels to the right. */
void EndingPositionRightOffsetComposite(x, y)
short x, y;
{
    struct object_state *object;

    if (gEndingRightOffsetCompositeEntity) {
        object = &gPlayerInfo[gEndingRightOffsetCompositeEntity];
        object->x = x + 50;
        object->y = y;
    }
}

/* Release the ending composite and clear its allocation slot. */
void EndingReleaseRightOffsetComposite()
{
    if (gEndingRightOffsetCompositeEntity)
        FreeEntity(gEndingRightOffsetCompositeEntity);
    gEndingRightOffsetCompositeEntity = 0;
}

/* Allocate the falling rocketship with the corresponding intro stream. */
void IntroCreateFallingRocketship()
{
    struct object_state *object;

    if (gIntroFallingRocketshipEntity = AllocateEntity()) {
        object = &gPlayerInfo[gIntroFallingRocketshipEntity];
        object->tile = 0;
        object->current_frame = Spr_IntroRocketshipFalling[0];
        object->frames = Spr_IntroRocketshipFalling;
        object->tile_count = 12;
        object->sprite_count = 2;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->sequence = &IntroFrameIndexLists[7];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Release the falling rocketship and clear its allocation slot. */
void IntroReleaseFallingRocketship()
{
    if (gIntroFallingRocketshipEntity)
        FreeEntity(gIntroFallingRocketshipEntity);
    gIntroFallingRocketshipEntity = 0;
}

/* Set the falling rocketship's position if it is allocated. */
void IntroPositionFallingRocketship(x, y)
short x, y;
{
    struct object_state *object;

    if (gIntroFallingRocketshipEntity) {
        object = &gPlayerInfo[gIntroFallingRocketshipEntity];
        object->x = x;
        object->y = y;
    }
}

/* Allocate the three-piece ending composite at an elevated position. */
void EndingCreateElevatedThreePieceComposite(x, y)
unsigned short x, y;
{
    struct object_state *object;

    if (gEndingElevatedThreePieceCompositeEntity = AllocateEntity()) {
        object = &gPlayerInfo[gEndingElevatedThreePieceCompositeEntity];
        object->tile = 0;
        object->current_frame = EndingElevatedCompositeFrames[0];
        object->frames = EndingElevatedCompositeFrames;
        object->tile = VDP_TILE_PRIORITY;
        object->tile_count = 30;
        object->sprite_count = 3;
        object->x = x;
        object->y = y + 50;
        object->height = 50;
        object->sprite_ready = 0;
        object->sequence = &EndingFrameIndexStreams[245];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 4;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Release the elevated ending composite and clear its allocation slot. */
void EndingReleaseElevatedThreePieceComposite()
{
    if (gEndingElevatedThreePieceCompositeEntity)
        FreeEntity(gEndingElevatedThreePieceCompositeEntity);
    gEndingElevatedThreePieceCompositeEntity = 0;
}

/* Restart the elevated composite on the alternate ending stream. */
void EndingRestartElevatedCompositeSequence()
{
    struct object_state *object;

    if (gEndingElevatedThreePieceCompositeEntity) {
        object = &gPlayerInfo[gEndingElevatedThreePieceCompositeEntity];
        object->sprite_ready = 0;
        object->sequence = &EndingFrameIndexStreams[249];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 4;
    }
}
