#include "hardware.h"
#define TJE_LOCAL_ALLOCATEENTITY

#include "entities.h"
#include "sprites.h"
#include "assets_cutscene.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int AllocateEntity();

/* Select ending-character sequences according to which players have lives. */
void EndingCreateActivePlayerEntities(x, y)
unsigned short x, y;
{
    short second_active, first_active;
    struct object_state *object;
    struct render_frame **frames;

    second_active = !(gPlayer2Lives < 0);
    first_active = !(gPlayerLives[PLAYER_TOEJAM] < 0);
    if (first_active) {
        if (gEndingPlayer1Entity = AllocateEntity()) {
            object = &gPlayerInfo[gEndingPlayer1Entity];
            frames = EndingPlayer1Frames;
            object->sprite_count = 3;
            object->tile_count = 48;
            object->tile = VDP_TILE_PRIORITY;
            object->frames = frames;
            if (second_active)
                object->sequence = EndingPlayer1PairedAnimation;
            else
                object->sequence = EndingPlayer1SoloAnimation;
            object->x = x;
            object->y = y + 100;
            object->height = 100;
            object->sprite_ready = 0;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            AllocateEntityViewportSprites(object, 0);
        }
    }
    if (second_active) {
        if (gEndingPlayer2Entity = AllocateEntity()) {
            object = &gPlayerInfo[gEndingPlayer2Entity];
            frames = EndingPlayer2Frames;
            object->sprite_count = 3;
            object->tile_count = 48;
            object->tile = VDP_TILE_PRIORITY;
            object->frames = frames;
            if (first_active)
                object->sequence = EndingPlayer2PairedAnimation;
            else
                object->sequence = EndingFrameIndexStreams;
            object->x = x;
            object->y = y + 100;
            object->height = 100;
            object->sprite_ready = 0;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            AllocateEntityViewportSprites(object, 0);
        }
    }
}