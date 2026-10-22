#include "entities.h"
#include "hardware.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Change levels once both falling players, or the only falling player,
 * are ready. A supported partner supplies the landing coordinates. */
void MakePlayerFall(falling)
char *falling;
{
    short player_number;
    struct object_state *player;

    if ((falling[0] && falling[1]) ||
        (falling[0] && !TOEJAM_FALL_STATE[1]) ||
        (falling[1] && !TOEJAM_FALL_STATE[0])) {
        for (player_number = 0; player_number <= 1; ++player_number) {
            player = &gPlayerInfo[player_number];
            if (falling[player_number]) {
                player->tile |= 0x8000;
                player->height = 600;
                player->velocity_height = -640;
                player->acceleration_height = -16;
                player->draw_flags |= 0x40;
                func_0174b6(player->level);
                func_0143e6(player->level);
                if (--player->level < 0)
                    player->level = gHighestLevel;
                TOEJAM_FALL_STATE[player_number] = 2;
            }
            if (gPlayerLives[player_number] < 0)
                LOAD_ELEVATOR_1();
        }
        if (falling[0]) {
            func_004dfe(0, gPlayerInfo[0].level);
            SetupElevator(gPlayerInfo[0].level, 0);
            func_017ac8(gPlayerInfo[0].level);
            PlaceStuffInLevel(gPlayerInfo[0].level);
        }
        if (falling[1]) {
            func_004dfe(1, gPlayerInfo[1].level);
            SetupElevator(gPlayerInfo[1].level, 0);
            func_017ac8(gPlayerInfo[1].level);
            PlaceStuffInLevel(gPlayerInfo[1].level);
        }
        ClearSkipVHBlankFlags();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
        if (gPlayerInfo[0].level == gPlayerInfo[1].level ||
            gPlayerLives[0] < 0 || gPlayer2Lives < 0) {
            if (falling[0] && falling[1])
                SetPositionOnNewLevel(2);
            else if (falling[0]) {
                player = &gPlayerInfo[1];
                /* (long) view of flags_52: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
                 * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
                if (LAND_UNDER_EARL >= 0x80 ||
                    ((long)player->flags_52 & 0x20000000L) || player->sprite_state == 5 ||
                    player->sprite_state == 6 || gPlayer2Lives < 0)
                    SetPositionOnNewLevel(0);
                else {
                    gPlayerInfo[0].x = gPlayerInfo[1].x;
                    gPlayerInfo[0].y = gPlayerInfo[1].y;
                    if (ELEVATOR_FLAG[1])
                        gPlayerInfo[0].y += 12;
                }
            } else {
                player = &gPlayerInfo[0];
                if (LAND_UNDER_TOEJAM >= 0x80 ||
                    ((long)player->flags_52 & 0x20000000L) || player->sprite_state == 5 ||
                    player->sprite_state == 6 || gPlayerLives[0] < 0)
                    SetPositionOnNewLevel(1);
                else {
                    gPlayerInfo[1].x = gPlayerInfo[0].x;
                    gPlayerInfo[1].y = gPlayerInfo[0].y;
                    if (ELEVATOR_FLAG[0])
                        gPlayerInfo[1].y += 12;
                }
            }
        } else {
            if (falling[0])
                SetPositionOnNewLevel(0);
            if (falling[1])
                SetPositionOnNewLevel(1);
        }
        func_00105a(1);
        func_00aa5a(0);
        func_00abbe(0);
        func_00aa5a(1);
        func_00abbe(1);
        VDP_CONTROL_WORD = 0x8c89;
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
        SetSkipVHBlankFlags();
    }
}

/* Advance a jump or landing animation and select the next sprite. */
void func_010086(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite;

    player_number = player->entity_number;
    if (!player->level || gPlayer1Powerup[player_number] == 1)
        return;
    sprite = -1;
    if (player->flags_52 & 0x80) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 9) {
            sprite = player->sprite_state + 4;
            player->height = 1;
        }
    } else if (player->flags_52 & 0x100) {
        if (!player->frame_duration) {
            StopPlayer(player);
            player->height = 35;
            player->draw_flags |= 64;
            sprite = player->sprite_state + 4;
            player->acceleration_height = -64;
        }
    } else if ((player->flags_52 & 0x200) && player->height < 4) {
        if (land == 0x80) {
            StopPlayer(player);
            player->height = 0;
            player->draw_flags &= ~64;
            sprite = player->sprite_state + 4;
            PlaySoundEffect((long)Sfx_Splash, 50, 0);
        } else {
            TOEJAM_JUMP_COUNTER[player_number] = 1;
            TOEJAM_FALL_STATE[player_number] = 2;
            sprite = player->sprite_state + 0xf0;
        }
    }
    if (sprite != -1)
        SetSprite(player, sprite);
}

/* Finish present animations and leave sprite 3 when nearby ground appears. */
void func_0101b4(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number, row, column;
    char sprite;

    player_number = player->entity_number;
    sprite = -1;
    if (player->flags_52 & 0x1000) {
        if (!player->frame_duration)
            sprite = GetActivePresentSprite(player);
    } else if ((player->flags_52 & 0x10400) && player->level &&
               !player->frame_duration) {
        if (gPlayer1Powerup[player_number] == 2)
            sprite = 77;
        else
            sprite = 3;
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->mode_15 = 0;
    }
    if (player->sprite_state == 3) {
        for (row = 0; row < 5; ++row) {
            for (column = 0; column < 5; ++column) {
                if ((short)(player_number == 0 ? LAND_NEAR_TOEJAM[row][column] :
                            LAND_NEAR_EARL[row][column]) != 0x80) {
                    sprite = 7;
                    row = 4;
                    break;
                }
            }
        }
    }
    if (sprite != -1)
        SetSprite(player, sprite);
}
