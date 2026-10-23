#include "player_animation.h"
#include "player_animation_groups.h"
#include "entities.h"
#include "hardware.h"
#include "assets_map.h"
#include "assets_palettes.h"
#include "assets_scenes.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "graphics_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "menu_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "title_calls.h"
#include "ui_calls.h"

/* Write the VDP's initial register values. */

void ResetVDP()
{
    VDP_CONTROL_WORD = 0x8004;
    VDP_CONTROL_WORD = 0x8144;
    VDP_CONTROL_WORD = 0x8228;
    VDP_CONTROL_WORD = 0x8334;
    VDP_CONTROL_WORD = 0x8407;
    VDP_CONTROL_WORD = 0x857c;
    VDP_CONTROL_WORD = 0x8700;
    VDP_CONTROL_WORD = 0x8a00;
    VDP_CONTROL_WORD = 0x8b00;
    VDP_CONTROL_WORD = 0x8c89;
    VDP_CONTROL_WORD = 0x8d3f;
    VDP_CONTROL_WORD = VDP_AUTOINCREMENT_WORD;
    VDP_CONTROL_WORD = 0x9001;
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    VDP_CONTROL_WORD = 0x9302;
    VDP_CONTROL_WORD = 0x9400;
    VDP_CONTROL_WORD = 0x9500;
    VDP_CONTROL_WORD = 0x9600;
    VDP_CONTROL_WORD = 0x9700;
}

/* Initialize the VDP, then enter hardware setup and the game loop. */
void VDPInitialisation()
{
    ResetVDP();
    FurtherInitialisation();
}

/* Select the physical controller and, when requested, exchange A and C. */
long ReadMappedPlayerInput(player)
short player;
{
    long input, mapped;
    if (player == PLAYER_TOEJAM) {
        if (!gControllerPortsSwapped) input = gController1RawInput;
        else input = gController2RawInput;
    } else {
        if (!gControllerPortsSwapped) input = gController2RawInput;
        else input = gController1RawInput;
    }
    if (gControllerButtonsAcSwapped) {
        mapped = input & (INPUT_START | INPUT_BUTTON_B | INPUT_DIRECTION_MASK);
        if (input & INPUT_BUTTON_A)
            mapped |= INPUT_BUTTON_C;
        if (input & INPUT_BUTTON_C)
            mapped |= INPUT_BUTTON_A;
        return mapped;
    }
    return input;
}

/* Draw the 32-by-32 starfield tilemap using the observed word indices. */
void DrawStarfieldPlane()
{
    short row, column;
    unsigned short tile;
    VDP_CONTROL_WORD = VDP_AUTOINCREMENT_TILE_ROW;
    for (row = 0; row < 32; ++row) {
        for (column = 0; column < 32; ++column) {
            tile = (StarfieldByteMap[row][column] & 0x87f) + 0x200;
            VDP_COMMAND = VDP_WRITE_PLANE_B + (row << 23) + (column << 17);
            VDP_DATA = tile;
            VDP_DATA = tile;
        }
    }
    VDP_CONTROL_WORD = VDP_AUTOINCREMENT_WORD;
}

/* Cycle four CRAM colours once every four calls. */
void DrawLevelGraphics()
{
    short colour;
    if (--gLevelPaletteAnimationDelay < 0) {
        gLevelPaletteAnimationDelay = 3;
        gLevelPaletteAnimationFrame = (gLevelPaletteAnimationFrame + 1) & 3;
        VDP_COMMAND = 0xc0340000;
        for (colour = 0; colour < 4; ++colour) {
            if (gPlayer1MapLevel < LEVEL_FINAL)
                VDP_DATA = PaletteAnimationFramesA[gLevelPaletteAnimationFrame][colour];
            else
                VDP_DATA = PaletteAnimationFramesB[gLevelPaletteAnimationFrame][colour];
        }
    }
}

/* Disable graphics work in both blanking interrupt handlers. */
void DisableBlankingGraphics()
{
    gVBlankGraphicsEnabled = 0;
    gHBlankGraphicsEnabled = 0;
}

/* Enable graphics work in both blanking interrupt handlers. */
void EnableBlankingGraphics()
{
    gVBlankGraphicsEnabled = 1;
    gHBlankGraphicsEnabled = 1;
}

/* Return whether the vertical-blank handler may update graphics. */
int GetVBlankGraphicsEnabled()
{
    return gVBlankGraphicsEnabled;
}

/* Wait for count frame-counter changes while the display is enabled. */
void WaitForNextVBlank(count)
short count;
{
    unsigned short time;
    if (gMode2RegSoftcopy & 0x40) {
        while (count > 0) {
            time = gFrameCounter;
            while (time == gFrameCounter)
                ;
            --count;
        }
    }
}

/* Select a player's viewport sprite chain, accounting for open inventories. */
void SelectViewportSpriteChain(player)
short player;
{
    if (gSplitScreenEnabled) {
        if (player == PLAYER_TOEJAM)
            SelectPrimaryViewportSprites();
        else
            SelectSecondaryViewportSprites();
    } else if ((player == PLAYER_TOEJAM && gPlayerInventoryOpen[PLAYER_TOEJAM]) ||
               (player == PLAYER_EARL && gPlayerInventoryOpen[PLAYER_EARL]))
        SelectSecondaryViewportSprites();
    else if (player == PLAYER_TOEJAM || gPlayerInventoryOpen[PLAYER_TOEJAM])
        SelectPrimaryViewportSprites();
    else
        TerminatePrimarySpriteChain();
}

/* Service graphics, music and controller polling, then advance the frame counter. */
void HandleVBlank()
{
    if (gVBlankFunc)
        (*gVBlankFunc)();
    else {
        if (gVBlankGraphicsEnabled) {
            if (gSpriteGraphicsUploadPending) {
                gCamera1CommittedX = gCamera1PendingX;
                gCamera1CommittedY = gCamera1PendingY;
                gCamera2CommittedX = gCamera2PendingX;
                gCamera2CommittedY = gCamera2PendingY;
            }
            if (gSplitScreenEnabled || gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayerInventoryOpen[PLAYER_EARL]) {
                SelectSpriteChainStart(1);
                VDP_CONTROL_WORD = 0x8a70;
            } else {
                SelectSpriteChainStart(0);
                VDP_CONTROL_WORD = 0x8ab6;
            }
            VDP_CONTROL_WORD = 0x8014;
            if (gPlayerInventoryOpen[PLAYER_TOEJAM])
                VDP_CONTROL_WORD = 0x920e;
            else {
                VDP_CONTROL_WORD = 0x8228;
                VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
                VDP_DATA = gCamera1CommittedY;
                VDP_COMMAND = 0x7c000003;
                VDP_DATA = -gCamera1CommittedX;
                if (gPlayer1MapLevel < LEVEL_FINAL) {
                    VDP_COMMAND = 0x40020010;
                    VDP_DATA = gCamera1CommittedY / 2;
                    VDP_COMMAND = 0x7c020003;
                    VDP_DATA = -gCamera1CommittedX / 2;
                }
            }
            SelectViewportSpriteChain(PLAYER_TOEJAM);
            if (gTerrainPriorityUpdatePending) {
                MapClearTilePriorityNearPlayer();
                /* MATCHING QUALIFICATION: jump to the shared frame increment;
                 * an early return duplicates it instead of sharing the tail. */
                goto end_frame;
            } else if (!gVBlankGraphicsPaused) {
                if (gInventoryRedrawPhase)
                    InventoryServiceRedrawPhase();
                else if (gSpriteGraphicsUploadPending) {
                    UploadQueuedSpriteGraphics(0, 0);
                    SelectViewportSpriteChain(PLAYER_TOEJAM);
                } else if (gHudRedrawPending) {
                    HudLoadLivesSpriteTiles(PLAYER_TOEJAM);
                    HudLoadLivesSpriteTiles(PLAYER_EARL);
                    DrawHUD();
                } else {
                    if (!TextAnimateFontBanks(4)) {
                        DrawLevelGraphics();
                        UpdateCameraY();
                        UpdateCameraX();
                        HudAnimateHealthBar(PLAYER_TOEJAM);
                        HudAnimateHealthBar(PLAYER_EARL);
                    }
                    if (++gVBlankPlayerUpdateParity & 1)
                        HideFlaggedPlayerSprites(PLAYER_TOEJAM);
                    else
                        HideFlaggedPlayerSprites(PLAYER_EARL);
                }
            }
        }
        if (!gVBlankGraphicsPaused || gVBlankAudioInputOverride) {
            if (gCurrentMusicSelection != gRequestedMusicSelection) {
                MusicSelectSongTemplateBank(gRequestedMusicSelection);
                gCurrentMusicSelection = gRequestedMusicSelection;
            } else
                MusicUpdateSequencer();
            if (gFrameCounterLowByte & 1)
                gController1RawInput = GetCtrl1Input();
            else
                gController2RawInput = GetCtrl2Input();
        }
end_frame:
        ++gFrameCounter;
    }
}

/* Switch the lower viewport's sprite chain and scroll registers at horizontal blank. */
void HandleHBlank()
{
    if (gHBlankGraphicsEnabled && gPlayer1MapLevel < LEVEL_FINAL) {
        SelectViewportSpriteChain(PLAYER_EARL);
        if (gSplitScreenEnabled || gPlayer2InventoryOpen) {
            if (gPlayer2InventoryOpen)
                VDP_CONTROL_WORD = 0x928e;
            else {
                VDP_CONTROL_WORD = 0x8230;
                VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
                VDP_DATA = gCamera2CommittedY - 112;
                VDP_COMMAND = 0x7c000003;
                VDP_DATA = -gCamera2CommittedX;
                VDP_COMMAND = 0x40020010;
                VDP_DATA = gCamera2CommittedY / 2 - 112;
                VDP_COMMAND = 0x7c020003;
                VDP_DATA = -gCamera2CommittedX / 2;
            }
        }
        VDP_CONTROL_WORD = 0x8aff;
    }
}

/* Refresh both mapped input bytes, masking buttons unavailable in each player state. */
void GetPlayerInput()
{
    short player;
    struct object_state *entity;
    entity = gPlayerInfo;
    for (player = PLAYER_TOEJAM; player <= PLAYER_EARL; ++player) {
        gPlayerPreviousProcessedInput[player] = gPlayerProcessedInput[player];
        if (gPlayerLives[player] >= 0) {
            gPlayerProcessedInput[player] = ReadMappedPlayerInput(player);
            if ((entity->animation_group_mask & PLAYER_GROUP_REACTION) || entity->sprite_state == PLAYER_ANIM_WHIRLWIND ||
                entity->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY || entity->sprite_state == PLAYER_ANIM_GHOST || entity->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
                gPlayerProcessedInput[player] &= INPUT_START;
        } else
            gPlayerProcessedInput[player] = 0;
        if (entity->animation_group_mask & PLAYER_GROUP_STATIONARY)
            gPlayerProcessedInput[player] &= (INPUT_START | INPUT_DIRECTION_MASK);
        else if (entity->animation_group_mask & (PLAYER_GROUP_FALL | PLAYER_GROUP_LAND))
            gPlayerProcessedInput[player] &= (INPUT_START | INPUT_BUTTON_A | INPUT_DIRECTION_MASK);
        else if ((entity->animation_group_mask & PLAYER_GROUP_ALL_SKATES) || entity->sprite_state == PLAYER_ANIM_FLATTENED)
            gPlayerProcessedInput[player] &= (INPUT_START | INPUT_BUTTON_A | INPUT_BUTTON_C | INPUT_DIRECTION_MASK);
        entity = &gPlayerInfo[PLAYER_EARL];
    }
}

/* Pause on a newly pressed Start button and report whether either player holds it. */
int CheckStartButton()
{
    if ((gPlayerProcessedInput[PLAYER_TOEJAM] | gPlayerProcessedInput[PLAYER_EARL]) & INPUT_START) {
        if (gPlayerProcessedInput[PLAYER_TOEJAM] & INPUT_START) {
            if (!(gPlayerPreviousProcessedInput[PLAYER_TOEJAM] & INPUT_START))
                GamePaused(PLAYER_TOEJAM);
        } else if (!(gPlayer2PreviousProcessedInput & INPUT_START))
            GamePaused(PLAYER_EARL);
        return 1;
    }
    return 0;
}

/* Dispatch menu and movement input, then award points for newly visited map cells. */
void ProcessInput()
{
    unsigned char input1, input2, mask;
    short player, x, y;
    struct object_state *entity;
    unsigned char *map_cell;
    GetPlayerInput();
    CheckAButton();
    input1 = gPlayerProcessedInput[PLAYER_TOEJAM];
    input2 = gPlayerProcessedInput[PLAYER_EARL];
    if (CheckStartButton())
        return;
    if (gPlayer1MapLevel < LEVEL_FINAL) {
        CheckBButton();
        if ((input1 | input2) & INPUT_BUTTON_C) {
            if (input1 & INPUT_BUTTON_C) {
                if (!gPlayerInventoryOpen[PLAYER_TOEJAM]) {
                    DrawGameMap(PLAYER_TOEJAM);
                    return;
                }
            } else if (!gPlayerInventoryOpen[PLAYER_EARL]) {
                DrawGameMap(PLAYER_EARL);
                return;
            }
        }
    }
    for (player = PLAYER_TOEJAM; player <= PLAYER_EARL; ++player) {
        if (gPlayerLives[player] >= 0 && !gPlayerInventoryOpen[player]) {
            SpriteChecks(player, gPlayerProcessedInput[player]);
            entity = &gPlayerInfo[player];
            x = entity->x - 72;
            y = entity->y - 8;
            if (x >= 0 && y >= 0 && x < 2560 && y < 1792) {
                map_cell = &gMapVisitedCoverBits[entity->level][y >> 8];
                mask = 0x80 >> (x / 320);
                if (!(*map_cell & mask)) {
                    *map_cell |= mask;
                    ++gPlayerPoints[player];
                    if (!gSplitScreenEnabled && gPlayerLives[player ^ 1] >= 0)
                        ++gPlayerPoints[player ^ 1];
                }
            }
        }
    }
}

/* Reload the pending primary camera position and any active secondary view. */
void ReloadActiveCameraViews()
{
    SetCameraPosition(PLAYER_TOEJAM, gCamera1PendingX, gCamera1PendingY);
    if (gSplitScreenEnabled)
        SetCameraPosition(PLAYER_EARL, gCamera2PendingX, gCamera2PendingY);
}

/* Enter split-screen mode, optionally animating the transition, and recenter both views. */
void EnterSplitScreen(animate)
char animate;
{
    short y;
    if (gMode2RegSoftcopy & 0x40) {
        while (gSpriteGraphicsUploadPending)
            ;
        DisableBlankingGraphics();
        VDP_CONTROL_WORD = 0x9200;
        HudHidePlayerSprites(2);
        if (animate) {
            AnimateSplitScreenWipe();
            AnimatePlayersIntoSplitViews();
        }
    }
    gSplitScreenEnabled = 1;
    if (!gPlayerInventoryOpen[PLAYER_TOEJAM]) {
        y = gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_TOEJAM].height_pixels / 2 - 56;
        if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_TOEJAM].height / 2;
        SetCameraPosition(PLAYER_TOEJAM, gPlayerInfo[PLAYER_TOEJAM].x - 160, y);
    }
    if (!gPlayer2InventoryOpen) {
        y = gPlayerInfo[PLAYER_EARL].y - gPlayerInfo[PLAYER_EARL].height_pixels / 2 - 56;
        if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_EARL].height / 2;
        SetCameraPosition(PLAYER_EARL, gPlayerInfo[PLAYER_EARL].x - 160, y);
        gCamera2CommittedX = gCamera2PendingX;
        gCamera2CommittedY = gCamera2PendingY;
    }
    if (gMode2RegSoftcopy & 0x40) {
        ForceTextBackgroundColorToBlack(0);
        gHudRedrawPending = 1;
        EnableBlankingGraphics();
    } else
        DrawHUD();
}

/* Merge both views around the players' midpoint and restore shared-screen graphics. */
void EnterSharedScreen(distance_y, distance_x)
short distance_y, distance_x;
{
    short x, y;
    char display_enabled;
    if (gPlayerInfo[PLAYER_TOEJAM].x > gPlayerInfo[PLAYER_EARL].x)
        x = gPlayerInfo[PLAYER_EARL].x;
    else
        x = gPlayerInfo[PLAYER_TOEJAM].x;
    x += distance_x / 2 - 160;
    if (gPlayerInfo[PLAYER_TOEJAM].y > gPlayerInfo[PLAYER_EARL].y) {
        y = gPlayerInfo[PLAYER_EARL].y - gPlayerInfo[PLAYER_EARL].height_pixels / 2;
        if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_EARL].height;
    } else {
        y = gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_TOEJAM].height_pixels / 2;
        if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_TOEJAM].height;
    }
    y += distance_y / 2 - 96;
    display_enabled = gMode2RegSoftcopy & 0x40;
    if (display_enabled) {
        WaitForVBlankWithGraphicsPaused();
        DisableBlankingGraphics();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    }
    gSplitScreenEnabled = 0;
    SetCameraPosition(PLAYER_TOEJAM, x, y);
    gCamera2PendingX = gCamera1PendingX;
    gCamera2PendingY = gCamera1PendingY;
    DrawHUD();
    if (gPlayer1MapLevel < LEVEL_FINAL)
        CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    else
        CopyPaletteToCRAMSlot(PaletteShipScreen_bank3, 0);
    if (display_enabled) {
        ForceTextBackgroundColorToBlack(0);
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
        EnableBlankingGraphics();
    }
}

/* Center the shared camera on the selected player and request a HUD redraw. */
void CenterSharedCameraOnPlayer(player)
short player;
{
    struct object_state *entity;
    short y;
    char display_enabled;
    entity = &gPlayerInfo[player];
    y = entity->y - entity->height_pixels / 2 - 112;
    if (entity->height > 0 && (entity->animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
        y -= entity->height / 2;
    display_enabled = gMode2RegSoftcopy & 0x40;
    if (display_enabled) {
        WaitForVBlankWithGraphicsPaused();
        DisableBlankingGraphics();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    }
    SetCameraPosition(PLAYER_TOEJAM, entity->x - 160, y);
    if (display_enabled) {
        ForceTextBackgroundColorToBlack(0);
        gHudRedrawPending = 1;
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
        EnableBlankingGraphics();
    } else
        DrawHUD();
}

/* Report whether two living players occupy different levels or exceed shared-view bounds. */
int PlayersRequireSeparateViews()
{
    short distance_y, distance_x;
    if (gPlayerLives[PLAYER_TOEJAM] < 0 || gPlayer2Lives < 0)
        return 0;
    if (gPlayerInfo[PLAYER_TOEJAM].level != gPlayerInfo[PLAYER_EARL].level)
        return 1;
    distance_y = gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_EARL].y;
    if (distance_y < 0)
        distance_y = -distance_y;
    distance_x = gPlayerInfo[PLAYER_TOEJAM].x - gPlayerInfo[PLAYER_EARL].x;
    if (distance_x < 0)
        distance_x = -distance_x;
    if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)) &&
        gPlayerInfo[PLAYER_TOEJAM].y < gPlayerInfo[PLAYER_EARL].y)
        distance_y += gPlayerInfo[PLAYER_TOEJAM].height;
    else if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)) &&
             gPlayerInfo[PLAYER_EARL].y < gPlayerInfo[PLAYER_TOEJAM].y)
        distance_y += gPlayerInfo[PLAYER_EARL].height;
    if (distance_y > 124 || distance_x > 220)
        return 1;
    return 0;
}

/* Choose shared or split views from player separation, life state and inventory state. */
void UpdateScreenSplitMode(force)
char force;
{
    short handled, distance_y, distance_x;
    if (force) {
        handled = 1;
        if (gPlayerLives[PLAYER_TOEJAM] < 0)
            CenterSharedCameraOnPlayer(PLAYER_EARL);
        else if (gPlayer2Lives < 0)
            CenterSharedCameraOnPlayer(PLAYER_TOEJAM);
        else if (gPlayerInfo[PLAYER_TOEJAM].level != gPlayerInfo[PLAYER_EARL].level)
            EnterSplitScreen(0);
        else
            handled = 0;
        if (handled)
            return;
    } else if (gPlayerLives[PLAYER_TOEJAM] < 0 || gPlayer2Lives < 0 ||
               gPlayerInfo[PLAYER_TOEJAM].level != gPlayerInfo[PLAYER_EARL].level)
        return;
    distance_y = gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_EARL].y;
    if (distance_y < 0)
        distance_y = -distance_y;
    distance_x = gPlayerInfo[PLAYER_TOEJAM].x - gPlayerInfo[PLAYER_EARL].x;
    if (distance_x < 0)
        distance_x = -distance_x;
    if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)) &&
        gPlayerInfo[PLAYER_TOEJAM].y < gPlayerInfo[PLAYER_EARL].y)
        distance_y += gPlayerInfo[PLAYER_TOEJAM].height;
    else if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)) &&
             gPlayerInfo[PLAYER_EARL].y < gPlayerInfo[PLAYER_TOEJAM].y)
        distance_y += gPlayerInfo[PLAYER_EARL].height;
    if (force) {
        if (distance_y > 124 || distance_x > 220 || gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayer2InventoryOpen)
            EnterSplitScreen(0);
        else
            EnterSharedScreen(distance_y, distance_x);
    } else {
        if (!gSplitScreenEnabled && (distance_y > 124 || distance_x > 220))
            EnterSplitScreen(1);
        else if (gSplitScreenEnabled && distance_y < 104 && distance_x < 200 &&
                 !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen)
            EnterSharedScreen(distance_y, distance_x);
    }
    if (!gSplitScreenEnabled) {
        gCamera2PendingX = gCamera1PendingX;
        gCamera2PendingY = gCamera1PendingY;
    }
}

/* Original busy-wait: the argument counts iterations, not video frames. */
void DelayIterations(iterations)
short iterations;
{
    while (iterations)
        --iterations;
}

/* Reset and release the Z80 around installation of its six-byte bootstrap. */
void InitialiseTheZ80()
{
    char *destination = Z80_RAM;
    /* The zero reset value is kept across all six delay calls in D2. */
    short reset = 0;
    Z80_RESET = reset;
    DelayIterations(10);
    Z80_BUS_REQUEST = 0x100;
    DelayIterations(10);
    Z80_RESET = 0x100;
    DelayIterations(10);
    /* Install the six-byte Z80 bootstrap through its mapped RAM port.
     * These are the original hardware writes, not replacement 68000 code. */
    *destination++ = 0xed;
    *destination++ = 0x56;
    *destination++ = 0xf3;
    *destination++ = 0xc3;
    *destination++ = 3;
    *destination++ = reset;
    Z80_RESET = reset;
    DelayIterations(10);
    Z80_BUS_REQUEST = reset;
    DelayIterations(10);
    Z80_RESET = 0x100;
    DelayIterations(10);
}
/* Convert a one-based selection to a repeating zero-based six-entry index.
 * Keep signed C remainder behavior for zero or negative inputs. */
short GetLevelMusicIndex(selection)
short selection;
{
    return (selection - 1) % 6;
}

/* Initialize hardware and game resources, then run the alternating frame-update phases. */
void FurtherInitialisation()
{
    /* ORIGINAL BUG: the matching source reads an uninitialized frame count.
     * The ROM has no explicit initialization of this local. Its allocated
     * D2 register last held gFrameCounter << 16, whose low word is zero.
     * [INFERENCE] The original source relied on an uninitialized frame count;
     * MATCHING QUALIFICATION: adding an initializer changes the bytes;
     * the observed source model is compiler-specific, not portable C. */
    short frame;
    unsigned short time;
    gMode2RegSoftcopy = 0x60;
    DisableBlankingGraphics();
    DmaCopyWordsToVram(&gFrameCounter, 0, 1);
    MusicRestoreInitialState();
    InitialiseTheZ80();
    LoadSoundDriverToZ80();
    MusicUpdateSequencer();
    EnableAllInterrupts();
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    StackBasedRandom(12345);
    PreGenerateRandoms();
    ClearColourRAM();
    ClearVerticalScrollRam();
    ClearVideoRAM();
    VDP_COMMAND = 0x70000002;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    VDP_COMMAND = 0x40000001;
    MoveDataToVRAM(ImgStarfieldLarge, StarryBackgroundSize << 4);
    TextCopyFontBanksToRam();
    SetRAMIndexes();
    SEGAAndIntroVideoRoutine();
    NewGameMainMenu();
    gRequestedMusicSelection = 0;
    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    VDP_COMMAND = 0x46600001;
    MoveDataToVRAM(InventoryLabelOpen, InventoryLabelOpenSize << 4);
    VDP_COMMAND = 0x79a00000;
    MoveDataToVRAM(InventoryLabelIdentify, InventoryLabelIdentifySize << 4);
    VDP_COMMAND = 0x74000003;
    MoveDataToVRAM(HUDAdditionalTiles, 304);
    VDP_COMMAND = 0x7f200003;
    MoveDataToVRAM(HUDAdditionalTilesSmall, 64);
    DrawStarfieldPlane();
    LoadLevel1Tiles();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    SetupPlayer();
    SetupTheMainGame();
    SetPositionOnNewLevel(2);
    SetCameraTilePriorityEnabled(1);
    UpdateScreenSplitMode(1);
    StackBasedRandom(GetRandomNumber() | ((long)gFrameCounter << 16));
    GenerateLevelSeeds();
    DrawHUD();
    SetupInitialElevator();
    SetupInitialInventory();
    SetupInitialEarthlings();
    GetPlayerInput();
    gCamera1CommittedX = gCamera1PendingX;
    gCamera1CommittedY = gCamera1PendingY;
    gCamera2PendingX = gCamera1PendingX;
    gCamera2PendingY = gCamera1PendingY;
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
    for (;;) {
        time = gFrameCounter;
        if (++frame & 1) {
            UpdateScreenSplitMode(0);
            UpdateCameraFollowing();
            ProcessLand();
            UpdateAndQueueEntitySprites();
        } else {
            BuildPendingCameraTiles();
            if (frame & 2) {
                ProcessEarthlings();
                ElevatorUpdateEntityVisibility();
                ProcessElevators();
                MapUpdateItemsAndPresents();
                ProcessTomatoRain();
            } else {
                UpdateLandAroundPlayer(PLAYER_TOEJAM);
                UpdateLandAroundPlayer(PLAYER_EARL);
                ProcessInput();
                ProcessPlayers();
                UpdateAllEntityShadows();
            }
            ProcessMovement();
            UpdateAllEntityViewportSprites();
            ProcessEntityAnimationMotion();
            UpdateSpeechAndDialogue();
        }
        while (time == gFrameCounter)
            GetRandomNumber();
    }
}
