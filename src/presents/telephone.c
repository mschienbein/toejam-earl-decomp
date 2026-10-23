#include "hardware.h"
#include "entities.h"
#include "presents.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_presents.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "map_calls.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "ram_speech.h"
#include "random.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: the (int)AllocateEntityWithEviction() result view
 * retains the observed call-result width with the shared short declaration.
 * Exact historical spelling remains unknown [INFERENCE]. */

/* Allocate the telephone's displayed object and copy its position and level. */
void TelephoneCreateDisplayEntity(telephone)
struct telephone_state *telephone;
{
    struct object_state *object;
    struct animation_descriptor *animation;

    if (telephone->object = (int)AllocateEntityWithEviction(0)) {
        object = &gPlayerInfo[telephone->object];
        animation = TelephoneAnimations;
        object->frames = animation->frames;
        object->sequence = SingleFrameHoldAnimation;
        object->frame_phase = 0;
        object->frame_duration = 1;
        object->frame = -1;
        object->x = telephone->x;
        object->y = telephone->y;
        object->level = telephone->level;
        object->width = 24;
        object->height_pixels = 16;
        object->sprite_count = 1;
        object->tile_count = 6;
        object->tile = VDP_TILE_PRIORITY;
        object->draw_flags |= ENTITY_SKIP_PHYSICS;
    }
}

/* Restart the ring timer and advance the visible telephone animation. */
void TelephoneStartRing(telephone_number, telephone)
short telephone_number;
struct telephone_state *telephone;
{
    struct object_state *object;

    if (!gElevatorDepartureInProgress)
        PlaySoundEffect((long)Sfx_Telephone, AUDIO_PRIORITY_NORMAL, 0);
    gTelephoneTimeToRing[telephone_number] = 60;
    if (telephone->object) {
        object = &gPlayerInfo[telephone->object];
        object->sequence = TelephoneAnimations[0].sequence;
        object->frame = -1;
        object->frame_duration = 1;
        object->frame_phase = 3;
    }
}

/* Visibility creates/removes the object. Nearby living players start its
 * ringing; a player reaching it opens the map. */
void CheckForTelephone(telephone_number)
short telephone_number;
{
    struct telephone_state *telephone;
    struct object_state *player;
    short player_number, screen_x, screen_y;
    short earl_screen_x, earl_screen_y;

    telephone = &gTelephoneStates[telephone_number];
    if (!telephone->level)
        return;
    screen_x = telephone->x - gCamera1PendingX;
    screen_y = telephone->y - gCamera1PendingY;
    earl_screen_x = telephone->x - gCamera2PendingX;
    earl_screen_y = telephone->y - gCamera2PendingY;
    if ((telephone->level == gPlayer1MapLevel && screen_x + 20 > 0 &&
         screen_x < 340 && screen_y + 20 > 0 && screen_y < 244) ||
        (telephone->level == gPlayer2MapLevel && gSplitScreenEnabled &&
         earl_screen_x + 20 > 0 && earl_screen_x < 340 &&
         earl_screen_y + 20 > 0 && earl_screen_y < 132)) {
        if (!telephone->object)
            TelephoneCreateDisplayEntity(telephone);
    } else if (telephone->object) {
        FreeEntity(telephone->object);
        telephone->object = 0;
    }
    if (!gTelephoneActivated[telephone_number]) {
        player = &gPlayerInfo[PLAYER_TOEJAM];
        player_number = PLAYER_TOEJAM;
        do {
            if (gPlayerLives[player_number] >= 0 &&
                player->level == telephone->level && !gPcmEffectCooldownTicks &&
                abs(telephone->x - player->x) < 320 &&
                abs(telephone->y - player->y) < 224) {
                gTelephoneActivated[telephone_number] = 1;
                gTelephoneRingsLeft[telephone_number] = 6;
                TelephoneStartRing(telephone_number, telephone);
                break;
            }
            ++player;
            ++player_number;
        } while (player_number <= PLAYER_EARL);
        if (!gTelephoneActivated[telephone_number])
            return;
    }
    if (--gTelephoneTimeToRing[telephone_number] <= 0) {
        if (--gTelephoneRingsLeft[telephone_number] > 0 &&
            (gPlayerInfo[PLAYER_TOEJAM].level == telephone->level ||
             gPlayerInfo[PLAYER_EARL].level == telephone->level)) {
            if (!gPcmEffectCooldownTicks)
                TelephoneStartRing(telephone_number, telephone);
            else {
                ++gTelephoneRingsLeft[telephone_number];
                gTelephoneTimeToRing[telephone_number] = 1;
            }
        } else {
            if (telephone->object) {
                FreeEntity(telephone->object);
                telephone->object = 0;
                PresentStartTransientEffect(telephone_number, telephone->x, telephone->y + 1,
                            0, telephone->level);
            }
            telephone->level = 0;
        }
    }
    if (telephone->object) {
        player = &gPlayerInfo[PLAYER_TOEJAM];
        player_number = PLAYER_TOEJAM;
        do {
            if (gPlayerLives[player_number] >= 0 &&
                player->level == telephone->level &&
                abs(telephone->x - player->x) < 16 &&
                abs(telephone->y - player->y) < 8) {
                FreeEntity(telephone->object);
                telephone->object = 0;
                gMapRevealMode = 1;
                DrawGameMap(player_number);
                gMapRevealMode = 0;
                telephone->level = 0;
                break;
            }
            ++player;
            ++player_number;
        } while (player_number <= PLAYER_EARL);
    }
}
