#include "player_animation.h"
#include "player_animation_groups.h"
#include "terrain.h"
#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"
#include "sprite_calls.h"

/* Start falling in the current facing direction, nudging away from a
 * neighboring supported tile before the vblank terrain-priority update. */
void PlayerStartFalling(player, player_number)
struct object_state *player;
short player_number;
{
    unsigned char *nearby_land;
    char sprite;
    short center_tile;

    nearby_land = player_number == PLAYER_TOEJAM ? &LAND_NEAR_TOEJAM[0][0] : &LAND_NEAR_EARL[0][0];
    TOEJAM_FALL_STATE[player_number] = 1;
    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
    center_tile = 12;
    if (sprite == PLAYER_ANIM_FALL_UP || sprite == PLAYER_ANIM_FALL_DOWN) {
        if (nearby_land[center_tile - 1] < TERRAIN_WATER)
            player->x += 8;
        else if (nearby_land[center_tile + 1] < TERRAIN_WATER)
            player->x -= 8;
    }
    player->acceleration_height = -128;
    SetSprite(player, sprite);
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    player->tile &= 0x7fff;
    gTerrainPriorityUpdatePlayer = player_number;
    WAIT_FOR_VBLANK_FLAG = 1;
    while (WAIT_FOR_VBLANK_FLAG)
        ;
    GenerateSpeech(player_number, "Bummer", 0);
    PlaySoundEffect((long)Sfx_Falling, 50, 0);
}

/* Launch a short directional fall and remember that it has started. */
void PlayerStartTeeteringFall(player)
struct object_state *player;
{
    short player_number;
    char sprite;

    player_number = player->entity_index;
    player->height = 5;
    player->velocity_height = 512;
    player->acceleration_height = -80;
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
    gPlayerLandingCounterSeed[player_number] = 1;
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite == PLAYER_ANIM_FALL_UP)
        player->velocity_y = -256;
    else if (sprite == PLAYER_ANIM_FALL_DOWN)
        player->velocity_y = 256;
    else if (sprite == PLAYER_ANIM_FALL_LEFT)
        player->velocity_x = -512;
    else if (sprite == PLAYER_ANIM_FALL_RIGHT)
        player->velocity_x = 512;
    SetSprite(player, sprite);
}

/* Count the teetering frames and play the observed randomized speech sounds.
 * After thirty frames, start the short fall unless the shared flag delays it. */
void PlayerUpdateTeetering(player)
struct object_state *player;
{
    short player_number;
    unsigned short sound_choice;

    player_number = player->entity_index;
    if (player->animation_group_mask & PLAYER_GROUP_LEAN) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 30) {
            if (gElevatorDepartureInProgress)
                --TOEJAM_JUMP_COUNTER[player_number];
            else
                PlayerStartTeeteringFall(player);
        } else if (TOEJAM_JUMP_COUNTER[player_number] == 1) {
            if (PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                PlaySoundEffect((long)Sfx_Teetering1, 86, 0);
            else
                PlaySoundEffect((long)Sfx_Teetering2, 86, 0);
        } else if (!gPcmEffectCooldownTicks) {
            sound_choice = PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 15;
            if (sound_choice == 0)
                PlaySoundEffect((long)Sfx_Teetering1, 20, 0);
            else if (sound_choice == 1)
                PlaySoundEffect((long)Sfx_Teetering2, 20, 0);
            else if (sound_choice == 2)
                PlaySoundEffect((long)Sfx_Teetering3, 20, 0);
        }
    }
}
