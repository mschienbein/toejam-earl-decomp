#include "entities.h"
#include "hardware.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "menu_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
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

/* Lower the player on terrain 0x20, with speech and partner overlap checks. */
void func_010294(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number, previous_height;
    int timer;

    player_number = player->entity_number;
    if (ram_ffe236[player_number] || (player->flags_52 & 0x20000) ||
        player->sprite_state == 79 || player->sprite_state == 78 ||
        player->sprite_state == 5 || player->sprite_state == 6)
        return;
    previous_height = player->height;
    if (land == 0x20 && player->height <= 0 && !(player->flags_52 & (unsigned long)0x60) &&
        !ELEVATOR_FLAG[player_number]) {
        if (!ram_ffda24[player_number]) {
            ram_ffda24[player_number] = 1;
            player->height = -3;
        }
        player->draw_flags |= 64;
        if (!((timer = TIME_COUNTER) & (unsigned long)60)) {
            if ((player_number == 0 && player->height > -16) ||
                (player_number == 1 && player->height > -20))
                --player->height;
            else if (!ram_ffda2e) {
                if (!SPLIT_SCREEN && gPlayerLives[player_number ^ 1] >= 0) {
                    if (PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) :
                                      ++ram_ff91e0] & 1) {
                        GenerateSpeech(player_number, "Help", 1);
                        /* MATCHING QUALIFICATION: 256 deliberately truncates to zero
                         * in this byte store, selecting the observed MOVE.B
                         * instead of CLR.B. The historical literal is unknown;
                         * no original bug or intended cooldown is asserted. */
                        ram_ffda2e = 256;
                    }
                }
            } else
                --ram_ffda2e;
        }
        if (func_00dd1e(player, &gPlayerInfo[player_number ^ 1], 18, 7))
            player->height = -3;
    } else if (ram_ffda24[player_number]) {
        player->height = 0;
        ram_ffda24[player_number] = 0;
        player->draw_flags &= ~64;
    }
    if (previous_height != player->height)
        player->sprite_ready = 0;
}

/* Apply the active sinking animation, then handle Earl's idle variation. */
void func_01044e(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char direction;
    unsigned char *nearby_land;

    player_number = player->entity_number;
    if (!player_number)
        nearby_land = &LAND_NEAR_TOEJAM[0][0];
    else
        nearby_land = &LAND_NEAR_EARL[0][0];
    if (((player->flags_52 & 0x2000) || player->sprite_state == 68) &&
        !player->frame_duration) {
        player->draw_flags &= ~64;
        TOEJAMS_HEALTH[player_number] -= ram_ffda2c[player_number];
        direction = player->sprite_state & 3;
        if (gPlayer1Powerup[player_number] == 5)
            OpenRocketSkates(player, player_number, land);
        else if (player->height > 4) {
            StopPlayer(player);
            player->mode_15 = 0;
            player->acceleration_height = -32;
            ram_ffda30[player_number] = 2;
            if (player->sprite_state == 68)
                direction = 1;
            SetSprite(player, direction + 20);
        } else if (nearby_land[12] < 0x80)
            SetSprite(player, GetActivePresentSprite(player));
        else {
            if (nearby_land[11] < 0x80)
                player->x += 16;
            if (nearby_land[13] < 0x80)
                player->x -= 16;
            if (nearby_land[12] == 0x80) {
                if (player->sprite_state == 68)
                    direction = 1;
                else if (direction & 2)
                    direction ^= 1;
                SetSprite(player, direction + 64);
                player->height = 0;
                PlaySoundEffect((long)Sfx_Splash, 50, 0);
            } else {
                if (TOEJAMS_HEALTH[player_number] <= 0)
                    TOEJAMS_HEALTH[player_number] = 1;
                StopPlayer(player);
                SetToejamFalling(player, player_number);
            }
        }
    }
    if (player_number == 1) {
        if (!player->sprite_state && player->frame) {
            if ((GetRandomNumber() & 0x7ff) == 1)
                SetSprite(player, 88);
        } else if (player->sprite_state == 88 && !player->frame_duration) {
            SetSprite(player, 0);
            player->frame = 1;
        }
    }
}

/* Start and complete spring-shoe jumps while the powerup is active. */
void func_01062c(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite, direction;

    player_number = player->entity_number;
    if (gPlayer1Powerup[player_number] != 1 ||
        TOEJAM_FALL_STATE[player_number] || ELEVATOR_FLAG[player_number])
        return;
    sprite = -1;
    direction = player->sprite_state & 3;
    if (player->flags_52 & 0x4000) {
        if ((P1_PROCESSED_INPUT[player_number] & 64) &&
            (player->velocity_x || player->velocity_y)) {
            sprite = direction + 28;
            TOEJAM_JUMP_COUNTER[player_number] = 0;
        }
    } else if (player->flags_52 & 0x80) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 4) {
            if (player->velocity_x > 0)
                sprite = 35;
            else if (player->velocity_x < 0)
                sprite = 34;
            else
                sprite = player->sprite_state + 4;
            player->height = 1;
            PlaySoundEffect((long)Sfx_Springing, 50, 0);
        }
    } else if (player->flags_52 & 0x100) {
        if (!player->frame_duration) {
            player->height = 35;
            player->velocity_height = 256;
            player->acceleration_height = -4;
            player->velocity_x <<= 1;
            player->mode_15 = 0;
            player->draw_flags |= 64;
            sprite = direction + 20;
            if (sprite & 2)
                sprite ^= 1;
            ram_ffda30[player_number] = 2;
            ram_ffda24[player_number] = 0;
        }
    } else if (player->flags_52 & 4)
        sprite = direction + 56;
    if (sprite != -1)
        SetSprite(player, sprite);
}

/* Resolve a falling or descending player against terrain and present state. */
void func_0107b2(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite, direction;

    player_number = player->entity_number;
    if (TOEJAM_FALL_STATE[player_number])
        return;
    if ((player->flags_52 & 0x20) || (player->flags_52 & 0x10000000)) {
        sprite = -1;
        direction = player->sprite_state & 3;
        if (land == 0x90)
            player->draw_flags &= ~64;
        else
            player->draw_flags |= 64;
        if (gPlayer1Powerup[player_number] == 1 && player->velocity_height > 0) {
            if (player->height > 60 || !(P1_PROCESSED_INPUT[player_number] & 64)) {
                player->velocity_height = 0;
                player->acceleration_height = -64;
            }
        } else if (player->height < 4) {
            if (land >= 0x80) {
                if (land == 0x90)
                    SetToejamFalling(player, player_number);
                else {
                    player->height = 0;
                    player->acceleration_height = 0;
                    player->velocity_height = 0;
                    if (direction & 2)
                        direction ^= 1;
                    sprite = direction + 64;
                    player->draw_flags &= ~64;
                    PlaySoundEffect((long)Sfx_Splash, 50, 0);
                }
            } else if (gPlayer1Powerup[player_number] == 7) {
                player->height = 0;
                player->acceleration_height = 0;
                player->velocity_height = 0;
                if (P1_PROCESSED_INPUT[player_number] & 15)
                    sprite = direction + 108;
                else
                    sprite = GetActivePresentSprite(player);
                player->draw_flags &= ~64;
            } else {
                TOEJAM_JUMP_COUNTER[player_number] = ram_ffda30[player_number];
                player->acceleration_height = -16;
                TOEJAM_FALL_STATE[player_number] = 2;
            }
            if ((sprite & 0xfc) != 108) {
                player->velocity_x = 0;
                player->velocity_y = 0;
            }
        }
        if (sprite != -1)
            SetSprite(player, sprite);
    }
}

/* Advance the player's level after waiting for the associated object to finish. */
void func_010968(player, player_number)
struct object_state *player;
short player_number;
{
    struct object_state *effect;

    func_015b66(player_number, player->x, player->y + 1, player->height + 9, player->level);
    effect = &gPlayerInfo[ram_ffde56[player_number]];
    func_00beb2(ram_ffde56[player_number]);
    if (effect->draw_flags & 2) {
        while (ram_ffde56[player_number]) {
            if (func_015d52(player_number))
                player->flags_4d |= 64;
            WaitForNextVBlank(2);
            func_00cc82();
        }
        player->flags_4d &= ~64;
    }
    func_0174b6(player->level);
    func_0143e6(player->level);
    ++player->level;
    func_004dfe(0, player->level);
    SetupElevator(player->level, 0);
    func_017ac8(player->level);
    PlaceStuffInLevel(player->level);
    if (player->level > 1)
        func_012762(player);
    else
        SetPositionOnNewLevel(player_number);
    player->height = 70;
}

/* Bring the player back at the other player's position, rebuilding a split level if needed. */
void func_010ab6(player_number)
short player_number;
{
    struct object_state *player, *other_player;
    short terrain;

    player = &gPlayerInfo[player_number];
    ram_ffda24[player_number] = 0;
    if (gPlayerLives[player_number ^ 1] >= 0)
        other_player = &gPlayerInfo[player_number ^ 1];
    else {
        if (player->level != gHighestLevel) {
            func_010968(player, player_number);
            goto redraw;
        }
        other_player = player;
    }
    player->x = other_player->x;
    player->y = other_player->y;
    /* (long) view of flags_52: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
     * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
    if ((long)other_player->flags_52 & 0x20000000L)
        player->y += 10;
    if (!ram_ffda6c) {
        if (ELEVATOR_FLAG[player_number ^ 1])
            player->y += 15;
        player->height = 70;
    } else
        player->height = 0;
    if (SPLIT_SCREEN && player->level != other_player->level) {
        func_0174b6(player->level);
        func_0143e6(player->level);
        player->level = other_player->level;
        if (player_number == 0) {
            func_004dfe(0, gPlayerInfo[0].level);
            SetupElevator(gPlayerInfo[0].level, 0);
            func_017ac8(gPlayerInfo[0].level);
            PlaceStuffInLevel(gPlayerInfo[0].level);
        } else {
            func_004dfe(1, gPlayerInfo[1].level);
            SetupElevator(gPlayerInfo[1].level, 0);
            func_017ac8(gPlayerInfo[1].level);
            PlaceStuffInLevel(gPlayerInfo[1].level);
        }
    }
redraw:
    func_008c5c();
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    func_00aa5a(player_number);
    func_00105a(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
    UpdateLandAroundPlayer(player_number);
    if (!ram_ffda6c) {
        SetSprite(player, 21);
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->velocity_height = -128;
        player->acceleration_height = -16;
        player->draw_flags |= 64;
        terrain = func_00d9dc(0, player->x, player->y);
        if (terrain != 0x90 && terrain != 0x80) {
            TOEJAM_FALL_STATE[player_number] = 2;
            TOEJAM_JUMP_COUNTER[player_number] = 3;
        } else
            player->y += 20;
        func_015b66(player_number, player->x, player->y + 1, player->height, player->level);
    } else {
        StopPlayer(player);
        ELEVATOR_FLAG[player_number] = ELEVATOR_FLAG[player_number ^ 1];
        player->flags_4d |= 64;
        SetSprite(player, 0);
    }
}

/* Start the death animation, cancel a held powerup, and remember the previous sprite state. */
void KillThePlayer(player_number)
short player_number;
{
    struct object_state *player;

    if (P1_MENU_OPEN[player_number])
        return;
    player = &gPlayerInfo[player_number];
    StopPlayer(player);
    player->velocity_height = 256;
    player->mode_15 = 0;
    if (gPlayer1Powerup[player_number] == 5) {
        PlaySound(6, 99);
        PlaySound(0, 98);
        gPlayer1Powerup[player_number] = -1;
        func_00acca(player_number);
        func_00ad8a(player_number);
    }
    if (player->sprite_state == 3 || player->sprite_state == 7 || (player->flags_52 & 0x800))
        player->height = -58;
    else
        player->height = -16;
    player->draw_flags &= 0x3f;
    ram_ffda1e[player_number] = player->sprite_state;
    SetSprite(player, 5);
    if (!gPlayerLives[player_number])
        player->flags_4d |= 128;
    TOEJAM_JUMP_COUNTER[player_number] = 0;
    ram_ffda24[player_number] = 0;
    player->flags_4d &= ~1;
}

/* Remove a player whose last-life animation has left the screen, then restore the HUD. */
void func_010ec2(player, player_number)
struct object_state *player;
short player_number;
{
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    if (gPlayerLives[player_number ^ 1] < 0) {
        func_00cffa();
        func_023554();
    }
    --gPlayerLives[player_number];
    func_0174b6(player->level);
    func_0143e6(player->level);
    SPLIT_SCREEN = 0;
    func_00cfba(0);
    player->level = 99;
    if (player->draw_flags & 2)
        OpeningPresents7(player, 0);
    if (player->draw_flags & 4)
        OpeningPresents7(player, 1);
    player->draw_flags = 0;
    gPlayerRank[player_number] = 9;
    func_00b95c(player_number);
    player = &gPlayerInfo[player_number ^ 1];
    func_004dfe(0, player->level);
    func_00105a(1);
    DrawHUD();
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
}

/* Complete death and return animations and age the temporary flag timer. */
void func_010ffc(player, terrain)
struct object_state *player;
short terrain;
{
    short player_number, camera_y;
    int active_sprite;

    player_number = player->entity_number;
    if (player->sprite_state == 5 || player->sprite_state == 6) {
        if (!SPLIT_SCREEN || !player_number)
            camera_y = TOEJAM_CAMERA_NEWY;
        else
            camera_y = EARL_CAMERA_NEWY;
        if (player->y - player->height - camera_y < 0) {
            player->velocity_height = 0;
            if (player->flags_4d & 128) {
                if (!P1_MENU_OPEN[player_number ^ 1])
                    func_010ec2(player, player_number);
                return;
            }
            player->height = 0;
            player->flags_4d |= 1;
            if (ram_ffda1e[player_number] == 2) {
                player->flags_4d |= 16;
                ram_ffda2a[player_number] = 20;
            }
            /* Compiler-spelling qualification: assignment within the
             * argument retains the original D0-to-D1 result copy; a
             * separate assignment or direct call omits it. */
            if (terrain != 0x90)
                SetSprite(player, (active_sprite = GetActivePresentSprite(player)));
            else
                SetToejamFalling(player, player_number);
        } else if (player->height < 0) {
            if (!--TOEJAM_JUMP_COUNTER[player_number])
                TOEJAM_JUMP_COUNTER[player_number] = 6;
            if (TOEJAM_JUMP_COUNTER[player_number] > 3)
                player->frame = 0;
            else
                player->frame = -1;
            player->sprite_ready = 0;
        } else if ((player->flags_4d & 128) && player->height >= 20) {
            if (player->sprite_state == 5) {
                SetSprite(player, 6);
                player->velocity_height = 0;
                TOEJAM_JUMP_COUNTER[player_number] = 30;
                if (!player_number)
                    GenerateSpeech(1, "Bye Toejam", 1);
                else
                    GenerateSpeech(0, "Bye Earl", 1);
            } else if (!player->velocity_height && --TOEJAM_JUMP_COUNTER[player_number] <= 0) {
                SetSprite(player, 5);
                player->sprite_state = 6;
                player->velocity_height = 256;
            }
        }
    }
    if (ram_ffda2a[player_number] && !--ram_ffda2a[player_number])
        player->flags_4d &= ~16;
}

/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = LAND_UNDER_TOEJAM & 0xf0;
    player = gPlayerInfo;
    for (player_number = 0; player_number <= 1;
         terrain = LAND_UNDER_EARL & 0xf0, ++player, ++player_number) {
        if (gPlayerLives[player_number] < 0)
            func_021d32(player_number);
        else {
            if (P1_MENU_OPEN[player_number])
                continue;
            func_00f96c(player);
            func_01044e(player, terrain);
            func_01062c(player, terrain);
            func_0107b2(player, terrain);
            func_011ac0(player, terrain);
            func_01189c(player, terrain);
            func_010086(player, terrain);
            func_0101b4(player, terrain);
            func_010294(player, terrain);
            func_010ffc(player, terrain);
            func_01c188(player, terrain);
            ProcessSleep(player);
            func_01b8ba(player, terrain);
            HealthAndLivesChecks(player, terrain);
            if (ELEVATOR_FLAG[player_number] || (ram_ffde60[0] & 3) == player_number + 1 ||
                (ram_ffde61 & 3) == player_number + 1)
                player->flags_4d |= 32;
            else if (player->sprite_state != 70 && player->sprite_state != 120)
                player->flags_4d &= ~32;
        }
        BummerLifePromo(player, terrain);
        func_01ccd2(player);
        func_01d084(player, terrain);
    }
    CheckPlayerFalling();
    if (PLAYER_INDICATOR == 2)
        func_012116();
}
