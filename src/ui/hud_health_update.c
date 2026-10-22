#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_ui.h"
#include "ui_calls.h"

/* Position the two health-bar sprites for the active HUD arrangement. */
void func_00b0cc(player_number)
short player_number;
{
    short x, sprite_number, y;

    if (player_number == 0) {
        sprite_number = 15;
        y = 229;
    } else {
        sprite_number = 17;
        y = 332;
    }
    if (P1_MENU_OPEN[player_number])
        x = 230;
    else if (SPLIT_SCREEN || P1_MENU_OPEN[player_number ^ 1]) {
        if (player_number == 0)
            y = 142;
        else
            y = 252;
        x = 144;
    } else {
        if (player_number == 0)
            x = 160;
        else
            x = 330;
        y = 333;
    }
    if (gPlayerLives[player_number] < 0)
        y = 1;
    ram_ffb65a[sprite_number].x = x;
    ram_ffb65a[sprite_number].y = y;
    ram_ffb65a[sprite_number + 1].x = x + 32;
    ram_ffb65a[sprite_number + 1].y = y;
}

/* Move the displayed health one step toward the clamped health value and
 * rewrite the affected three rows of the health bar. */
void func_00b18c(player_number)
short player_number;
{
    short maximum_health, tile_offset, displayed_health, vram_base, vram_address, pixel_number;
    unsigned short pixel_pattern;

    if (ram_ffa256[player_number] != gPlayerLives[player_number]) {
        func_00b3e4(player_number);
        func_00b590(player_number);
    }
    /* (&gPlayerInfo[i])->field rather than gPlayerInfo[i].field: the two compile differently
     * here and only this spelling reproduces the ROM. */
    if (gPlayerLives[player_number] < 0 || (&gPlayerInfo[player_number])->sprite_state == 5 ||
        (&gPlayerInfo[player_number])->sprite_state == 6 || gLevel912C == 26)
        return;
    if (player_number == 0)
        maximum_health = 23;
    else
        maximum_health = 31;
    maximum_health += gPlayerRank[player_number] * 4;
    if (TOEJAMS_HEALTH[player_number] < 0)
        TOEJAMS_HEALTH[player_number] = 0;
    if (TOEJAMS_HEALTH[player_number] > maximum_health)
        TOEJAMS_HEALTH[player_number] = maximum_health;
    if (TOEJAM_RESTORE_HEALTH[player_number] > maximum_health)
        TOEJAM_RESTORE_HEALTH[player_number] = maximum_health;
    if (TOEJAMS_HEALTH[player_number] > TOEJAMS_MAX_HEALTH[player_number])
        TOEJAMS_MAX_HEALTH[player_number]++;
    else if (TOEJAMS_HEALTH[player_number] < TOEJAMS_MAX_HEALTH[player_number])
        TOEJAMS_MAX_HEALTH[player_number]--;
    else
        return;
    displayed_health = TOEJAMS_MAX_HEALTH[player_number];
    if (player_number == 0)
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
    VDP_CONTROL_WORD = 0x8f02;
}
