#include "entities.h"
#include "hardware.h"
#include "ram.h"
#include "assets.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "random.h"
#include "player.h"
#include "present_calls.h"
#include "sprite_calls.h"
#include "ui_calls.h"
#include "romcalls.h"
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
