#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "map_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ship.h"
#include "random.h"
#include "ship.h"
#include "speech.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Release the collected world object, show the corresponding ship piece,
 * and restore the appropriate single-screen or split-screen message. */
void ShipPieceAbility(player_number, ship_piece)
short player_number;
struct earthling_state *ship_piece;
{
    struct object_state *player;
    char object_number;
    short piece_number;

    if (!ship_piece->object)
        return;
    player = &gPlayerInfo[player_number];
    /* [INFERENCE] The original retains this otherwise-unused object read. */
    object_number = ship_piece->object;
    for (piece_number = 0; piece_number < 10; ++piece_number) {
        if (gShipPieceLevelAssignments[piece_number] == player->level)
            break;
    }
    FreeEntity(ship_piece->object);
    ship_piece->kind = -1;
    ship_piece->object = 0;
    ShowShipPieces(piece_number, player_number);
    if (player_number == 0 || !SPLIT_SCREEN)
        StartDialogue(2);
    else
        GenerateSpeech(1, "Jammin", 1);
}

/* Create the pending camera-relative indicator only outside split-screen.
 * Remove an existing indicator when it leaves its fixed screen row. */
void ShipUpdatePendingPieceIndicator()
{
    char *active_object;
    struct object_state *object;
    struct animation_descriptor *animation;

    active_object = &gShipPieceIndicatorEntity;
    if (*active_object) {
        object = &gPlayerInfo[*active_object];
        if (object->y - TOEJAM_CAMERA_NEWY != 224) {
            FreeEntity(*active_object);
            *active_object = 0;
        }
    } else if (gShipScreenState[4]) {
        if (!SPLIT_SCREEN) {
            *active_object = (int)AllocateEntityWithEviction(0);
            object = &gPlayerInfo[*active_object];
            object->x = TOEJAM_CAMERA_NEWX + 160;
            object->y = TOEJAM_CAMERA_NEWY + 224;
            object->height = 112;
            object->level = gLevel912C;
            object->width = 96;
            object->height_pixels = 32;
            object->sprite_count = 3;
            object->tile_count = 48;
            object->tile |= 0x8000;
            animation = &AnimationDescriptors_0AF048;
            object->sequence = animation->sequence;
            object->frames = animation->frames;
            object->frame_duration = 1;
            object->frame = -1;
        }
        gShipScreenState[4] = 0;
    }
}

/* Start at a random byte and cycle until an uncollected piece is found.
 * ORIGINAL BUG: this loop has no exit if all ten entries are zero. */
void ShowRandomUncollectedShipPiece()
{
    short piece_number;

    piece_number = GetRandomNumber() & 255;
    do {
        piece_number = (piece_number + 1) % 10;
    } while (!gShipPieceLevelAssignments[piece_number]);
    ShowShipPieces(piece_number, 2);
}
