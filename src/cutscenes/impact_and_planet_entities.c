#include "entities.h"
#include "assets_cutscene.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Allocate the rocketship impact object at the supplied position. */
void IntroCreateRocketshipImpact(x, y)
short x, y;
{
    struct object_state *object;

    if (gIntroRocketshipImpactEntity = (int)AllocateEntity()) {
        object = &gPlayerInfo[gIntroRocketshipImpactEntity];
        object->tile = 0;
        object->current_frame = Spr_IntroRocketshipImpact[0];
        object->frames = Spr_IntroRocketshipImpact;
        object->tile_count = 8;
        object->sprite_count = 1;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)IntroFrameIndexLists;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Release the impact slot without clearing its stored index. */
void IntroReleaseRocketshipImpact()
{
    if (gIntroRocketshipImpactEntity)
        FreeEntity(gIntroRocketshipImpactEntity);
}

/* Allocate the planet object at the supplied position. */
void CutsceneCreatePlanetEntity(x, y)
short x, y;
{
    struct object_state *object;

    if (gCutscenePlanetEntity = (int)AllocateEntity()) {
        object = &gPlayerInfo[gCutscenePlanetEntity];
        object->tile = 0;
        object->current_frame = &Spr_PlanetFrame00;
        object->frames = Spr_Planet;
        object->tile_count = 72;
        object->sprite_count = 6;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Release the planet slot without clearing its stored index. */
void CutsceneReleasePlanetEntity()
{
    if (gCutscenePlanetEntity)
        FreeEntity(gCutscenePlanetEntity);
}

/* Allocate the ending composite at the supplied position. */
void EndingCreateStationaryComposite(x, y)
short x, y;
{
    struct object_state *object;

    if (gEndingStationaryCompositeEntity = (int)AllocateEntity()) {
        object = &gPlayerInfo[gEndingStationaryCompositeEntity];
        object->tile = 0;
        /* ORIGINAL BUG: current_frame receives the frame-table address,
         * not its first frame; dereferencing the table does not match. */
        object->current_frame = EndingStationaryCompositeFrames;
        object->frames = EndingStationaryCompositeFrames;
        object->tile_count = 72;
        object->sprite_count = 6;
        object->x = x;
        object->y = y;
        object->sprite_ready = 0;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        AllocateEntityViewportSprites(object, 0);
    }
}

/* Release the ending composite slot without clearing its stored index. */
void EndingReleaseStationaryComposite()
{
    if (gEndingStationaryCompositeEntity)
        FreeEntity(gEndingStationaryCompositeEntity);
}
