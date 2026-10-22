#include "player_animation.h"
#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_ui.h"
#include "ui_calls.h"
#include "hardware_calls.h"
#include "assets_map.h"
#include "assets_ui.h"

/* Position the two health-bar sprites for the active HUD arrangement. */
void HudPositionHealthBarSprites(player_number)
short player_number;
{
    short x, sprite_number, y;

    if (player_number == PLAYER_TOEJAM) {
        sprite_number = 15;
        y = 229;
    } else {
        sprite_number = 17;
        y = 332;
    }
    if (gPlayerInventoryOpen[player_number])
        x = 230;
    else if (gSplitScreenEnabled || gPlayerInventoryOpen[player_number ^ 1]) {
        if (player_number == PLAYER_TOEJAM)
            y = 142;
        else
            y = 252;
        x = 144;
    } else {
        if (player_number == PLAYER_TOEJAM)
            x = 160;
        else
            x = 330;
        y = 333;
    }
    if (gPlayerLives[player_number] < 0)
        y = 1;
    gHardwareSpriteTable[sprite_number].x = x;
    gHardwareSpriteTable[sprite_number].y = y;
    gHardwareSpriteTable[sprite_number + 1].x = x + 32;
    gHardwareSpriteTable[sprite_number + 1].y = y;
}

/* Move the displayed health one step toward the clamped health value and
 * rewrite the affected three rows of the health bar. */
void HudAnimateHealthBar(player_number)
short player_number;
{
    short maximum_health, tile_offset, displayed_health, vram_base, vram_address, pixel_number;
    unsigned short pixel_pattern;

    if (gHudDisplayedLives[player_number] != gPlayerLives[player_number]) {
        HudLoadLivesSpriteTiles(player_number);
        HudPositionLivesSprite(player_number);
    }
    /* (&gPlayerInfo[i])->field rather than gPlayerInfo[i].field: the two compile differently
     * here and only this spelling reproduces the ROM. */
    if (gPlayerLives[player_number] < 0 || (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_GHOST ||
        (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayer1MapLevel == 26)
        return;
    if (player_number == PLAYER_TOEJAM)
        maximum_health = 23;
    else
        maximum_health = 31;
    maximum_health += gPlayerRank[player_number] * 4;
    if (gPlayerHealth[player_number] < 0)
        gPlayerHealth[player_number] = 0;
    if (gPlayerHealth[player_number] > maximum_health)
        gPlayerHealth[player_number] = maximum_health;
    if (gPlayerHealthBeforeSwimming[player_number] > maximum_health)
        gPlayerHealthBeforeSwimming[player_number] = maximum_health;
    if (gPlayerHealth[player_number] > gHudDisplayedHealth[player_number])
        gHudDisplayedHealth[player_number]++;
    else if (gPlayerHealth[player_number] < gHudDisplayedHealth[player_number])
        gHudDisplayedHealth[player_number]--;
    else
        return;
    displayed_health = gHudDisplayedHealth[player_number];
    if (player_number == PLAYER_TOEJAM)
        vram_base = 0xf680;
    else
        vram_base = 0xfa80;
    VDP_CONTROL_WORD = 0x8f04;
    tile_offset = (displayed_health / 8) * 32 + ((displayed_health >> 1) & 2);
    pixel_pattern = 0xdddd;
    if (displayed_health <= maximum_health - 4) {
        if (displayed_health & 4)
            vram_address = vram_base + tile_offset + 30;
        else
            vram_address = vram_base + tile_offset + 2;
        VDP_COMMAND = VRAM_WRITE(vram_address);
        VDP_DATA = pixel_pattern;
        VDP_DATA = pixel_pattern;
        VDP_DATA = pixel_pattern;
    }
    if (displayed_health)
        for (pixel_number = displayed_health & 3; pixel_number >= 0; pixel_number--) {
            pixel_pattern >>= 4;
            pixel_pattern |= 0xb000;
        }
    VDP_COMMAND = VRAM_WRITE(vram_base + tile_offset);
    VDP_DATA = pixel_pattern;
    VDP_DATA = pixel_pattern;
    VDP_DATA = pixel_pattern;
    VDP_CONTROL_WORD = VDP_AUTOINCREMENT_WORD;
}

/* Load four life-icon tiles, using the second icon image above four lives. */
void HudLoadLivesSpriteTiles(player)
short player;
{
    short sprite_number, remaining, lives;
    long tile;
    char *icons;
    /* MATCHING QUALIFICATION: assigning the unoffset icon base inside the
     * argument preserves the original D0-pointer/D1-offset arithmetic and
     * argument scheduling. The assignment-only local is compiler coaxing;
     * the historical source spelling is not claimed. Assigning the completed
     * pointer instead adds an instruction, while a standalone assignment
     * changes scheduling. */
    char *icon_base;

    if (gPlayerLives[player] < 0)
        return;
    if (player == 0) {
        tile = 0x7bc;
        sprite_number = 19;
        icons = IconTilesA;
    } else {
        tile = 0x7dc;
        sprite_number = 20;
        icons = IconTilesB;
    }
    lives = gPlayerLives[player];
    gHardwareSpriteTable[sprite_number].size = 12;
    gHardwareSpriteTable[sprite_number].tile = (unsigned short)tile + 0xe000;
    if (lives > 4) {
        for (remaining = lives - 4; remaining > 0; remaining--) {
            VDP_COMMAND = VRAM_WRITE(tile << 5);
            MoveDataToVRAM((icon_base = icons) + 32, 16);
            tile++;
        }
        for (remaining = 8 - lives; remaining > 0; remaining--) {
            VDP_COMMAND = VRAM_WRITE(tile << 5);
            MoveDataToVRAM(icons, 16);
            tile++;
        }
    } else {
        for (remaining = lives; remaining > 0; remaining--) {
            VDP_COMMAND = VRAM_WRITE(tile << 5);
            MoveDataToVRAM(icons, 16);
            tile++;
        }
        for (remaining = 4 - lives; remaining > 0; remaining--) {
            VDP_COMMAND = VRAM_WRITE(tile << 5);
            MoveDataToVRAM(LevelTiles, 16);
            tile++;
        }
    }
    gHudDisplayedLives[player] = lives;
}
