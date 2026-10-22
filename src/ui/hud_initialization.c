#include "present_types.h"
#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_palettes.h"
#include "assets_ui.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Upload the two level digits and set the pair of hardware sprite tiles. */
void HudLoadLevelNumberSprites(player_number)
short player_number;
{
    char level_number, units_digit, tens_digit;
    short sprite_number, tile_number;
    unsigned short attributes;

    if (player_number == PLAYER_TOEJAM) {
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
    gHardwareSpriteTable[sprite_number].size = 5;
    gHardwareSpriteTable[sprite_number].tile = tile_number + attributes;
    tile_number += 4;
    VDP_COMMAND = VRAM_WRITE(tile_number << 5);
    MoveDataToVRAM(ImageLevelNumbers[tens_digit], 64);
    attributes = (LevelNumberPalettes[tens_digit] << 13) + 0x8000;
    gHardwareSpriteTable[sprite_number + 1].size = 5;
    gHardwareSpriteTable[sprite_number + 1].tile = tile_number + attributes;
}

/* Position the level-number sprites for menus, split-screen, or the HUD. */
void HudPositionLevelNumberSprites(player_number)
short player_number;
{
    short sprite_number, x, tens_digit, y;

    if (player_number == PLAYER_TOEJAM) {
        sprite_number = 11;
        tens_digit = gToejamLevel / 10;
    } else {
        sprite_number = 13;
        tens_digit = gEarlLevel / 10;
    }
    if (gPlayerLives[player_number] < 0) {
        y = 1;
        x = 1;
    } else if (gPlayerInventoryOpen[player_number]) {
        if (player_number == PLAYER_TOEJAM)
            y = 218;
        else
            y = 321;
        x = 348;
    } else if (gSplitScreenEnabled || gPlayerInventoryOpen[player_number ^ 1]) {
        if (player_number == PLAYER_TOEJAM)
            y = 136;
        else
            y = 249;
        x = 410;
    } else {
        if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
            gHardwareSpriteTable[13].y = 1;
            gHardwareSpriteTable[14].y = 1;
        }
        y = 321;
        x = 276;
    }
    if (tens_digit) {
        gHardwareSpriteTable[sprite_number].x = x + 8;
        gHardwareSpriteTable[sprite_number + 1].x = x;
        gHardwareSpriteTable[sprite_number + 1].y = y;
    } else {
        gHardwareSpriteTable[sprite_number].x = x + 4;
        gHardwareSpriteTable[sprite_number + 1].y = 1;
    }
    gHardwareSpriteTable[sprite_number].y = y;
}

/* Select the player's default or active-powerup icon and upload its tiles. */
void HudLoadActionIconSprite(player_number)
short player_number;
{
    short sprite_number, tile_number, powerup;
    unsigned short attributes;
    struct render_piece *icon;

    if (player_number == PLAYER_TOEJAM) {
        sprite_number = 21;
        tile_number = 0x7f1;
    } else {
        sprite_number = 22;
        tile_number = 0x7f5;
    }
    powerup = gPlayerPowerups[player_number];
    if (powerup != -1)
        icon = Spr_ActionIcons[powerup + 2];
    else
        icon = Spr_ActionIcons[player_number];
    if (GetVBlankGraphicsEnabled())
        while (gSpriteGraphicsUploadPending)
            ;
    QueueSpritePieceTiles(icon, tile_number);
    attributes = ((icon->tile_format & 3) << 13) + 0x8000;
    gHardwareSpriteTable[sprite_number].size = 5;
    gHardwareSpriteTable[sprite_number].tile = tile_number + attributes;
    gSpriteGraphicsUploadPending = 1;
}

/* Position the action icon, including the icon's signed origin offsets. */
void HudPositionActionIconSprite(player_number)
short player_number;
{
    short x, icon_number, y;
    struct render_piece *icon;

    if (gPlayerPowerups[player_number] == PRESENT_NONE)
        icon_number = player_number;
    else
        icon_number = gPlayerPowerups[player_number] + 2;
    icon = Spr_ActionIcons[icon_number];
    if (player_number == PLAYER_TOEJAM) {
        icon_number = 21;
        y = 218;
    } else {
        icon_number = 22;
        y = 321;
    }
    if (gPlayerLives[player_number] < 0) {
        y = 1;
        x = 1;
    } else if (gPlayerInventoryOpen[player_number])
        x = 208;
    else if (gSplitScreenEnabled || gPlayerInventoryOpen[player_number ^ 1]) {
        x = 392;
        y = 1;
        if (gPlayerPowerups[player_number] != PRESENT_NONE) {
            if (player_number == PLAYER_TOEJAM)
                y = 136;
            else
                y = 249;
        }
    } else {
        if (player_number == PLAYER_TOEJAM)
            x = 136;
        else
            x = 306;
        y = 321;
    }
    gHardwareSpriteTable[icon_number].x = icon->x + x;
    gHardwareSpriteTable[icon_number].y = icon->y + y;
}

/* ToeJam starts at 23 health units and Earl at 31, plus four per rank. */
int PlayerGetMaximumHealth(player_number)
short player_number;
{
    int maximum_health;

    if (player_number == PLAYER_TOEJAM)
        maximum_health = 23;
    else
        maximum_health = 31;
    maximum_health += gPlayerRank[player_number] << 2;
    return maximum_health;
}
