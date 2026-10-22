#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Initialize the rocketship and its separate exhaust object. */
void CutsceneAllocateRocketshipPair()
{
    struct object_state *object;

    gCutsceneRocketshipEntity = (int)AllocateEntity();
    object = &gPlayerInfo[gCutsceneRocketshipEntity];
    object->tile_count = 101;
    object->sprite_count = 8;
    object->tile = 0;
    object->current_frame = Spr_IntroRocketshipDodging[0];
    object->sprite_ready = 0;
    object->sequence = &IntroHeldFrameStreams[0].frame;
    object->frames = Spr_IntroRocketshipDodging;
    object->frame_phase = 1;
    object->frame_duration = 1;
    object->frame = -1;
    gCutsceneRocketshipExhaustEntity = (int)AllocateEntity();
    object = &gPlayerInfo[gCutsceneRocketshipExhaustEntity];
    object->tile_count = 15;
    object->sprite_count = 2;
    object->tile = 0;
    object->current_frame = Spr_RocketshipExhaust[(unsigned char)IntroFrameIndexLists[119]];
    object->sprite_ready = 0;
    object->sequence = IntroAnimationPointers[0];
    object->frames = Spr_RocketshipExhaust;
    object->frame_phase = 6;
    object->frame_duration = 1;
    object->frame = -1;
}