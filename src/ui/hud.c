/* Keep the existing HudDrawRankLabels incomplete rank-table view before
 * the complete translation unit first includes assets_ui.h. */
#define TJE_LOCAL_RANKCHARSTABLE

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
    attributes = (LevelNumberPalettes[units_digit] << 13) + VDP_TILE_PRIORITY;
    gHardwareSpriteTable[sprite_number].size = 5;
    gHardwareSpriteTable[sprite_number].tile = tile_number + attributes;
    tile_number += 4;
    VDP_COMMAND = VRAM_WRITE(tile_number << 5);
    MoveDataToVRAM(ImageLevelNumbers[tens_digit], 64);
    attributes = (LevelNumberPalettes[tens_digit] << 13) + VDP_TILE_PRIORITY;
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
    if (powerup != PRESENT_NONE)
        icon = Spr_ActionIcons[powerup + 2];
    else
        icon = Spr_ActionIcons[player_number];
    if (GetVBlankGraphicsEnabled())
        while (gSpriteGraphicsUploadPending)
            ;
    QueueSpritePieceTiles(icon, tile_number);
    attributes = ((icon->tile_format & 3) << 13) + VDP_TILE_PRIORITY;
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
        maximum_health = PLAYER_TOEJAM_BASE_HEALTH;
    else
        maximum_health = PLAYER_EARL_BASE_HEALTH;
    maximum_health += gPlayerRank[player_number] << 2;
    return maximum_health;
}

#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "hardware_calls.h"
#include "ui_calls.h"

/* Build the maximum-capacity health-bar pixels and reset displayed health. */
void HudInitializeHealthBar(player_number, upload_now)
short player_number, upload_now;
{
    unsigned char pixels;
    unsigned char full_bar;
    short sprite_number;
    short maximum_health;
    short row;
    short unit;
    short byte_offset;
    short tile_row;
    long tile_number;
    short health_unit;

    gHudDisplayedHealth[player_number] = 0;
    if (player_number == PLAYER_TOEJAM) {
        tile_number = 0x7b4;
        sprite_number = 15;
        maximum_health = PLAYER_TOEJAM_BASE_HEALTH;
    } else {
        tile_number = 0x7d4;
        sprite_number = 17;
        maximum_health = PLAYER_EARL_BASE_HEALTH;
    }
    if (gPlayerLives[player_number] >= 0)
        maximum_health += gPlayerRank[player_number] * 4;
    gHardwareSpriteTable[sprite_number].size = VDP_SPRITE_SIZE_4X1;
    gHardwareSpriteTable[sprite_number].tile = (unsigned short)tile_number + VDP_TILE_PRIORITY;
    gHardwareSpriteTable[sprite_number + 1].size = VDP_SPRITE_SIZE_4X1;
    gHardwareSpriteTable[sprite_number + 1].tile = (unsigned short)tile_number + (VDP_TILE_PRIORITY + 4);
    for (row = 0; row < HUD_HEALTH_BAR_TILES * VDP_TILE_BYTES; ++row)
        gSpriteTileUploadBuffer[gSpriteTileBufferOffset + row] = 0;

    full_bar = 0;
    for (row = 0; row < 8 && !full_bar; ++row) {
        for (unit = 0; unit < 8; ++unit) {
            health_unit = (row << 3) + unit;
            if (health_unit >= maximum_health) {
                full_bar = 1;
                break;
            }
            pixels = 13;
            if (!(unit & 1))
                pixels <<= 4;
            byte_offset = gSpriteTileBufferOffset;
            byte_offset += row * VDP_TILE_BYTES + unit / 2;
            for (tile_row = 0; tile_row < 3; ++tile_row)
                gSpriteTileUploadBuffer[(tile_row << 2) + byte_offset] |= pixels;
        }
    }

    if (upload_now) {
        /* MATCHING QUALIFICATION: the explicit int index retains the original
         * zero-extended long indexed address. A bare word index selects word
         * addressing; ordinary pointer addition chooses different arithmetic.
         * The shared buffer declaration and word offset are unchanged. */
        DmaCopyWordsToVram(
            &gSpriteTileUploadBuffer[(int)gSpriteTileBufferOffset],
            tile_number << 5, HUD_HEALTH_BAR_TILES * (VDP_TILE_BYTES / 2));
        while (gHudDisplayedHealth[player_number] < gPlayerHealth[player_number])
            HudAnimateHealthBar(player_number);
    } else {
        gSpriteTileBufferOffset += HUD_HEALTH_BAR_TILES * VDP_TILE_BYTES;
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_number;
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = HUD_HEALTH_BAR_TILES;
        gSpriteGraphicsUploadPending = 1;
        while (gSpriteGraphicsUploadPending)
            ;
        gHudDisplayedHealth[player_number] = 0;
    }
}

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
    /* MATCHING QUALIFICATION: (&gPlayerInfo[player_number])->field retains
     * the observed code; equivalent direct indexed members compile differently. */
    if (gPlayerLives[player_number] < 0 || (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_GHOST ||
        (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayer1MapLevel == LEVEL_FINAL)
        return;
    if (player_number == PLAYER_TOEJAM)
        maximum_health = PLAYER_TOEJAM_BASE_HEALTH;
    else
        maximum_health = PLAYER_EARL_BASE_HEALTH;
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
    if (player == PLAYER_TOEJAM) {
        tile = 0x7bc;
        sprite_number = 19;
        icons = IconTilesA;
    } else {
        tile = 0x7dc;
        sprite_number = 20;
        icons = IconTilesB;
    }
    lives = gPlayerLives[player];
    gHardwareSpriteTable[sprite_number].size = VDP_SPRITE_SIZE_4X1;
    gHardwareSpriteTable[sprite_number].tile = (unsigned short)tile + (VDP_TILE_PRIORITY | VDP_TILE_PALETTE_3);
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

#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#define TJE_LOCAL_RANKCHARSTABLE

#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_ui.h"
#include "hardware_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "ui_calls.h"

/* MATCHING QUALIFICATION: the compatible incomplete RankCharsTable array
 * preserves long-index/offset instructions. Its shared [9] bound changes
 * them; historical source spelling is unknown [INFERENCE]. */
extern char *RankCharsTable[];

/* Position the life icons for the menu, split-screen, or combined HUD. */
void HudPositionLivesSprite(player)
short player;
{
    short sprite_number, y, lives, x;

    if (player == PLAYER_TOEJAM) {
        sprite_number = 19;
        y = 226;
    } else {
        sprite_number = 20;
        y = 329;
    }
    lives = gPlayerLives[player];
    if (gPlayerInventoryOpen[player])
        x = 298;
    else if (gSplitScreenEnabled || gPlayerInventoryOpen[player ^ 1]) {
        /* MATCHING QUALIFICATION: the redundant default-only switch keeps
         * the observed TST/BNE.S/BRA.W join. This is compiler coaxing, not
         * evidence that the historical source used this control structure. */
        if (player == PLAYER_TOEJAM) {
            switch (player) {
            default:
                x = 138;
                break;
            }
        } else
            x = 138;
        y = 5;
    } else {
        if (player == PLAYER_TOEJAM)
            x = 228;
        else
            x = 398;
        y = 330;
    }
    if (lives < 5)
        y -= 3;
    if (gPlayerLives[player] < 0)
        y = 1;
    gHardwareSpriteTable[sprite_number].x = x;
    gHardwareSpriteTable[sprite_number].y = y;
}

/* Print each active player's rank and the absent player's vacation label. */
void HudDrawRankLabels()
{
    if (gPlayerSelection == 2) {
        VDP_COMMAND = 0x5c160003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_TOEJAM]]);
        VDP_COMMAND = 0x5c400003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_EARL]]);
    } else if (gPlayerSelection == PLAYER_TOEJAM) {
        VDP_COMMAND = 0x5c160003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_TOEJAM]]);
        VDP_COMMAND = 0x5c3c0003;
        TextWriteStringTiles(" on");
        VDP_COMMAND = 0x5cba0003;
        TextWriteStringTiles("vacation");
    } else {
        VDP_COMMAND = 0x5c400003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_EARL]]);
        VDP_COMMAND = 0x5c120003;
        TextWriteStringTiles(" on");
        VDP_COMMAND = 0x5c900003;
        TextWriteStringTiles("vacation");
    }
}

/* Draw the five-row combined HUD when neither inventory is open, otherwise
 * refresh the alternate HUD and the hardware sprite positions. */
void DrawHUD()
{
    short row, column;
    unsigned short tile;

    if (gPlayer1MapLevel == LEVEL_FINAL) {
        gHudRedrawPending = 0;
        return;
    }
    if (!gSplitScreenEnabled && !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayerInventoryOpen[PLAYER_EARL]) {
        for (row = 0; row < 5; row++) {
            VDP_COMMAND = VDP_WRITE_WINDOW + ((row + 23) << 23);
            if (row < 4) {
                for (column = 0; column < 40; column++) {
                    tile = CombinedHudBackgroundByteMap[row][column];
                    if (tile & 0x40)
                        tile |= VDP_TILE_HORIZONTAL_FLIP;
                    if (tile & 0x80)
                        tile |= VDP_TILE_VERTICAL_FLIP;
                    tile &= 0xff3f;
                    VDP_DATA = tile + 0x8580;
                }
            } else
                for (column = 0; column < 40; column++)
                    VDP_DATA = 0xc580;
        }
        VDP_CONTROL_WORD = 0x9297;
        HudDrawRankLabels();
    } else {
        if (!gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayerInventoryOpen[PLAYER_EARL])
            VDP_CONTROL_WORD = 0x9200;
        HudPositionLevelNumberSprites(PLAYER_EARL);
    }
    if (gPlayerLives[PLAYER_TOEJAM] >= 0)
        HudPositionLevelNumberSprites(PLAYER_TOEJAM);
    else
        HudPositionLevelNumberSprites(PLAYER_EARL);
    HudPositionActionIconSprite(PLAYER_TOEJAM);
    HudPositionActionIconSprite(PLAYER_EARL);
    HudPositionHealthBarSprites(PLAYER_TOEJAM);
    HudPositionHealthBarSprites(PLAYER_EARL);
    HudPositionLivesSprite(PLAYER_TOEJAM);
    HudPositionLivesSprite(PLAYER_EARL);
    DmaCopyWordsToVram(gHudHardwareSpriteBytes, 0xf858, 48);
    gHudRedrawPending = 0;
}

/* Sum the promotion thresholds below rank eight. */
short GetScoreForPromo(rank)
short rank;
{
    short score = 40;
    short prior_rank;

    if (rank >= 8)
        score = 0;
    else
        for (prior_rank = 0; prior_rank < rank; ++prior_rank)
            score += (prior_rank + 3) * 20;
    return score;
}

/* Advance an eligible player's rank when its score reaches the next threshold. */
int CheckPromotionScore(player_number)
short player_number;
{
    short rank;

    if (gPlayerLives[player_number] < 0)
        return 0;
    rank = gPlayerRank[player_number];
    if (rank < 8 && gPlayerPoints[player_number] >= GetScoreForPromo(rank)) {
        ++gPlayerRank[player_number];
        while (gSpriteGraphicsUploadPending)
            ;
        HudInitializeHealthBar(player_number, 0);
        if (!gSplitScreenEnabled)
            gHudRedrawPending = 1;
        return 1;
    }
    return 0;
}

/* Hide the selected player's HUD sprites in VRAM and the sprite shadow. */
void HudHidePlayerSprites(player_number)
short player_number;
{
    if (player_number == PLAYER_TOEJAM || player_number == 2) {
        VDP_COMMAND = 0x78580003;
        VDP_DATA = 1;
        gHardwareSpriteTable[11].y = 1;
        VDP_COMMAND = 0x78600003;
        VDP_DATA = 1;
        gHardwareSpriteTable[12].y = 1;
        VDP_COMMAND = 0x78a80003;
        VDP_DATA = 1;
        gHardwareSpriteTable[21].y = 1;
        VDP_COMMAND = 0x78780003;
        VDP_DATA = 1;
        gHardwareSpriteTable[15].y = 1;
        VDP_COMMAND = 0x78800003;
        VDP_DATA = 1;
        gHardwareSpriteTable[16].y = 1;
        VDP_COMMAND = 0x78980003;
        VDP_DATA = 1;
        gHardwareSpriteTable[19].y = 1;
    }
    if (player_number == PLAYER_EARL || player_number == 2) {
        VDP_COMMAND = 0x78680003;
        VDP_DATA = 1;
        gHardwareSpriteTable[13].y = 1;
        VDP_COMMAND = 0x78700003;
        VDP_DATA = 1;
        gHardwareSpriteTable[14].y = 1;
        VDP_COMMAND = 0x78b00003;
        VDP_DATA = 1;
        gHardwareSpriteTable[22].y = 1;
        VDP_COMMAND = 0x78880003;
        VDP_DATA = 1;
        gHardwareSpriteTable[17].y = 1;
        VDP_COMMAND = 0x78900003;
        VDP_DATA = 1;
        gHardwareSpriteTable[18].y = 1;
        VDP_COMMAND = 0x78a00003;
        VDP_DATA = 1;
        gHardwareSpriteTable[20].y = 1;
    }
}

/* Recompute rank from points, restore starting resources, and refresh HUD data. */
void PlayerResetRankAndResources(player_number)
short player_number;
{
    gPlayerRank[player_number] = 0;
    /* ORIGINAL BUG: no rank-eight stop. Its zero threshold makes this loop
     * endless when points are at least the rank-seven threshold (880). */
    while (gPlayerPoints[player_number] >= GetScoreForPromo(gPlayerRank[player_number]))
        ++gPlayerRank[player_number];
    gPlayerBucks[player_number] = 0;
    if (gPlayerSelection == 2)
        gPlayerLives[player_number] = 0;
    else {
        gPlayerLives[player_number] = 3;
        gPlayerBucks[player_number] = 3;
    }
    gPlayerHealth[player_number] = PlayerGetMaximumHealth(player_number);
    gPlayerHealthBeforeSwimming[player_number] = -1;
    gPlayerPowerups[player_number] = PRESENT_NONE;
    HudInitializeHealthBar(player_number, 1);
    HudLoadActionIconSprite(player_number);
    HudLoadLivesSpriteTiles(player_number);
}

/* Initialize living players' level-one resources and both players' HUD sprite state. */
void SetupTheMainGame()
{
    if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        gToejamLevel = 1;
        gPlayerHealth[PLAYER_TOEJAM] = PLAYER_TOEJAM_BASE_HEALTH;
        gPlayerLives[PLAYER_TOEJAM] = 3;
        gPlayerBucks[PLAYER_TOEJAM] = 3;
    }
    if (gPlayerLives[PLAYER_EARL] >= 0) {
        gEarlLevel = 1;
        gPlayer2Health = PLAYER_EARL_BASE_HEALTH;
        gPlayerLives[PLAYER_EARL] = 3;
        gPlayer2Bucks = 3;
    }
    gPlayer2HealthBeforeSwimming = -1;
    gPlayerHealthBeforeSwimming[PLAYER_TOEJAM] = -1;
    gPlayerPowerups[PLAYER_TOEJAM] = PRESENT_NONE;
    gPlayer2Powerup = PRESENT_NONE;
    HudInitializeHealthBar(PLAYER_TOEJAM, 1);
    HudInitializeHealthBar(PLAYER_EARL, 1);
    HudLoadLevelNumberSprites(PLAYER_TOEJAM);
    HudLoadLevelNumberSprites(PLAYER_EARL);
    HudLoadActionIconSprite(PLAYER_TOEJAM);
    HudLoadActionIconSprite(PLAYER_EARL);
    HudLoadLivesSpriteTiles(PLAYER_TOEJAM);
    HudLoadLivesSpriteTiles(PLAYER_EARL);
}

/* MATCHING QUALIFICATION: PlayerUpdateHealthAndLives reads the shared
 * unsigned-long animation_group_mask as signed long for the reaction and
 * swimming tests. Direct unsigned reads change MCC68K's mask code and
 * scratch allocation. The field remains unsigned in the common record;
 * historical spelling is unknown [INFERENCE]. */
/* Apply death, preserve/restore health around the observed state/mask cases,
 * and check promotion. */
void PlayerUpdateHealthAndLives(entity, terrain_class)
struct object_state *entity;
unsigned short terrain_class;
{
    short player_number = entity->entity_index;

    if (entity->sprite_state == PLAYER_ANIM_GHOST || entity->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST ||
        ((long)entity->animation_group_mask & PLAYER_GROUP_REACTION) || gPlayer1MapLevel == LEVEL_FINAL ||
        gPlayerPromotionDisplayEntities[player_number])
        return;
    if (!gHudDisplayedHealth[player_number] && gPlayerHealth[player_number] <= 0) {
        KillThePlayer(player_number);
        if (--gPlayerLives[player_number] < 0) {
            ++gPlayerLives[player_number];
            gPlayerHealthBeforeSwimming[player_number] = -1;
        } else {
            gPlayerHealth[player_number] = 100;
            if (gPlayerHealthBeforeSwimming[player_number] != -1)
                gPlayerHealthBeforeSwimming[player_number] = 100;
        }
        return;
    }
    if (((long)entity->animation_group_mask & PLAYER_GROUP_SWIM) || entity->sprite_state == PLAYER_ANIM_SWIM_IDLE ||
        entity->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE) {
        if (gPlayerHealthBeforeSwimming[player_number] == -1)
            gPlayerHealthBeforeSwimming[player_number] = gPlayerHealth[player_number];
        if (!(gFrameCounter & 12) && gPlayerHealth[player_number] > 0)
            --gPlayerHealth[player_number];
    } else {
        if (gPlayerHealthBeforeSwimming[player_number] != -1)
            gPlayerHealth[player_number] = gPlayerHealthBeforeSwimming[player_number];
        gPlayerHealthBeforeSwimming[player_number] = -1;
    }
    if ((entity->draw_flags & ENTITY_DRAW_LATE) && entity->sprite_state != PLAYER_ANIM_FLATTENED)
        entity->draw_flags &= ~ENTITY_DRAW_LATE;
    if (CheckPromotionScore(player_number))
        PerformPromotion(player_number, PRESENT_PROMOTION, terrain_class);
}

