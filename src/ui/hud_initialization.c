#include "sprites.h"
#include "hardware.h"
#include "ram.h"
#include "assets.h"
#include "core_calls.h"
#include "ui_calls.h"
#include "romcalls.h"
/* Upload the two level digits and set the pair of hardware sprite tiles. */
void func_00aa5a(player_number)
short player_number;
{
    char level_number, units_digit, tens_digit;
    short sprite_number, tile_number;
    unsigned short attributes;

    if (player_number == 0) {
        sprite_number = 11;
        tile_number = 0x7e1;
        level_number = gToejamLevel;
    } else {
        sprite_number = 13;
        tile_number = 0x7e9;
        level_number = gEarlLevel;
    }
    units_digit = level_number % 10;
    tens_digit = level_number / 10;
    VDP_COMMAND = VRAM_WRITE(tile_number << 5);
    MoveDataToVRAM(ImageLevelNumbers[units_digit], 64);
    attributes = (LevelNumberPalettes[units_digit] << 13) + 0x8000;
    ram_ffb65a[sprite_number].size = 5;
    ram_ffb65a[sprite_number].tile = tile_number + attributes;
    tile_number += 4;
    VDP_COMMAND = VRAM_WRITE(tile_number << 5);
    MoveDataToVRAM(ImageLevelNumbers[tens_digit], 64);
    attributes = (LevelNumberPalettes[tens_digit] << 13) + 0x8000;
    ram_ffb65a[sprite_number + 1].size = 5;
    ram_ffb65a[sprite_number + 1].tile = tile_number + attributes;
}

/* Position the level-number sprites for menus, split-screen, or the HUD. */
void func_00abbe(player_number)
short player_number;
{
    short sprite_number, x, tens_digit, y;

    if (player_number == 0) {
        sprite_number = 11;
        tens_digit = gToejamLevel / 10;
    } else {
        sprite_number = 13;
        tens_digit = gEarlLevel / 10;
    }
    if (gPlayerLives[player_number] < 0) {
        y = 1;
        x = 1;
    } else if (P1_MENU_OPEN[player_number]) {
        if (player_number == 0)
            y = 218;
        else
            y = 321;
        x = 348;
    } else if (SPLIT_SCREEN || P1_MENU_OPEN[player_number ^ 1]) {
        if (player_number == 0)
            y = 136;
        else
            y = 249;
        x = 410;
    } else {
        if (gPlayerLives[0] >= 0) {
            ram_ffb65a[13].y = 1;
            ram_ffb65a[14].y = 1;
        }
        y = 321;
        x = 276;
    }
    if (tens_digit) {
        ram_ffb65a[sprite_number].x = x + 8;
        ram_ffb65a[sprite_number + 1].x = x;
        ram_ffb65a[sprite_number + 1].y = y;
    } else {
        ram_ffb65a[sprite_number].x = x + 4;
        ram_ffb65a[sprite_number + 1].y = 1;
    }
    ram_ffb65a[sprite_number].y = y;
}

/* Select the player's default or active-powerup icon and upload its tiles. */
void func_00acca(player_number)
short player_number;
{
    short sprite_number, tile_number, powerup;
    unsigned short attributes;
    struct render_piece *icon;

    if (player_number == 0) {
        sprite_number = 21;
        tile_number = 0x7f1;
    } else {
        sprite_number = 22;
        tile_number = 0x7f5;
    }
    powerup = gPlayer1Powerup[player_number];
    if (powerup != -1)
        icon = Spr_ActionIcons[powerup + 2];
    else
        icon = Spr_ActionIcons[player_number];
    if (GetSkipVBlankFlag())
        while (VBLANK_FLAG_2)
            ;
    func_00c7a8(icon, tile_number);
    attributes = ((icon->format & 3) << 13) + 0x8000;
    ram_ffb65a[sprite_number].size = 5;
    ram_ffb65a[sprite_number].tile = tile_number + attributes;
    VBLANK_FLAG_2 = 1;
}

/* Position the action icon, including the icon's signed origin offsets. */
void func_00ad8a(player_number)
short player_number;
{
    short x, icon_number, y;
    struct render_piece *icon;

    if (gPlayer1Powerup[player_number] == -1)
        icon_number = player_number;
    else
        icon_number = gPlayer1Powerup[player_number] + 2;
    icon = Spr_ActionIcons[icon_number];
    if (player_number == 0) {
        icon_number = 21;
        y = 218;
    } else {
        icon_number = 22;
        y = 321;
    }
    if (gPlayerLives[player_number] < 0) {
        y = 1;
        x = 1;
    } else if (P1_MENU_OPEN[player_number])
        x = 208;
    else if (SPLIT_SCREEN || P1_MENU_OPEN[player_number ^ 1]) {
        x = 392;
        y = 1;
        if (gPlayer1Powerup[player_number] != -1) {
            if (player_number == 0)
                y = 136;
            else
                y = 249;
        }
    } else {
        if (player_number == 0)
            x = 136;
        else
            x = 306;
        y = 321;
    }
    ram_ffb65a[icon_number].x = icon->x + x;
    ram_ffb65a[icon_number].y = icon->y + y;
}

/* ToeJam starts at 23 health units and Earl at 31, plus four per rank. */
int func_00ae90(player_number)
short player_number;
{
    int maximum_health;

    if (player_number == 0)
        maximum_health = 23;
    else
        maximum_health = 31;
    maximum_health += gPlayerRank[player_number] << 2;
    return maximum_health;
}
