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

/* Write the VDP register initialization sequence, then enter game setup.
 * ResetVDP also matches plain C exactly despite its previous assembly label. */

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

void VDPInitialisation()
{
    ResetVDP();
    FurtherInitialisation();
}

/* Select the physical controller and, when requested, exchange A and C. */
long MovePadInputIntoD0(player)
short player;
{
    long input, mapped;
    if (player == 0) {
        if (!gControllerPortsSwapped) input = P1_PAD_INPUT;
        else input = P2_PAD_INPUT;
    } else {
        if (!gControllerPortsSwapped) input = P2_PAD_INPUT;
        else input = P1_PAD_INPUT;
    }
    if (gControllerButtonsAcSwapped) {
        mapped = input & 0x9f;
        if (input & 0x40)
            mapped |= 0x20;
        if (input & 0x20)
            mapped |= 0x40;
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
            if (gLevel912C < 26)
                VDP_DATA = PaletteAnimationFramesA[gLevelPaletteAnimationFrame][colour];
            else
                VDP_DATA = PaletteAnimationFramesB[gLevelPaletteAnimationFrame][colour];
        }
    }
}

void ClearSkipVHBlankFlags()
{
    SKIP_VBLANK_FLAG = 0;
    SKIP_HBLANK_FLAG = 0;
}

void SetSkipVHBlankFlags()
{
    SKIP_VBLANK_FLAG = 1;
    SKIP_HBLANK_FLAG = 1;
}

int GetSkipVBlankFlag()
{
    return SKIP_VBLANK_FLAG;
}

void WaitForNextVBlank(count)
short count;
{
    unsigned short time;
    if (gMode2RegSoftcopy & 0x40) {
        while (count > 0) {
            time = TIME_COUNTER;
            while (time == TIME_COUNTER)
                ;
            --count;
        }
    }
}

void SelectViewportSpriteChain(player)
short player;
{
    if (SPLIT_SCREEN) {
        if (player == 0)
            SelectPrimaryViewportSprites();
        else
            SelectSecondaryViewportSprites();
    } else if ((player == 0 && P1_MENU_OPEN[PLAYER_TOEJAM]) ||
               (player == 1 && P1_MENU_OPEN[PLAYER_EARL]))
        SelectSecondaryViewportSprites();
    else if (player == 0 || P1_MENU_OPEN[PLAYER_TOEJAM])
        SelectPrimaryViewportSprites();
    else
        TerminatePrimarySpriteChain();
}
void HandleVBlank()
{
    if (gVBlankFunc)
        (*gVBlankFunc)();
    else {
        if (SKIP_VBLANK_FLAG) {
            if (VBLANK_FLAG_2) {
                P1_CAMERA_X = TOEJAM_CAMERA_NEWX;
                P1_CAMERA_Y = TOEJAM_CAMERA_NEWY;
                P2_CAMERA_X = EARL_CAMERA_NEWX;
                P2_CAMERA_Y = EARL_CAMERA_NEWY;
            }
            if (SPLIT_SCREEN || P1_MENU_OPEN[PLAYER_TOEJAM] || P1_MENU_OPEN[PLAYER_EARL]) {
                SelectSpriteChainStart(1);
                VDP_CONTROL_WORD = 0x8a70;
            } else {
                SelectSpriteChainStart(0);
                VDP_CONTROL_WORD = 0x8ab6;
            }
            VDP_CONTROL_WORD = 0x8014;
            if (P1_MENU_OPEN[PLAYER_TOEJAM])
                VDP_CONTROL_WORD = 0x920e;
            else {
                VDP_CONTROL_WORD = 0x8228;
                VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
                VDP_DATA = P1_CAMERA_Y;
                VDP_COMMAND = 0x7c000003;
                VDP_DATA = -P1_CAMERA_X;
                if (gLevel912C < 26) {
                    VDP_COMMAND = 0x40020010;
                    VDP_DATA = P1_CAMERA_Y / 2;
                    VDP_COMMAND = 0x7c020003;
                    VDP_DATA = -P1_CAMERA_X / 2;
                }
            }
            SelectViewportSpriteChain(0);
            if (WAIT_FOR_VBLANK_FLAG) {
                MapClearTilePriorityNearPlayer();
                /* The ROM jumps directly to the shared frame increment.
                 * An early return duplicates it rather than matching. */
                goto end_frame;
            } else if (!VBLANK_FLAG_3) {
                if (VBLANK_FLAG_4)
                    InventoryServiceRedrawPhase();
                else if (VBLANK_FLAG_2) {
                    SpriteLoadingStuff(0, 0);
                    SelectViewportSpriteChain(0);
                } else if (VBLANK_FLAG_5) {
                    HudLoadLivesSpriteTiles(0);
                    HudLoadLivesSpriteTiles(1);
                    DrawHUD();
                } else {
                    if (!TextAnimateFontBanks(4)) {
                        DrawLevelGraphics();
                        UpdateCameraY();
                        UpdateCameraX();
                        HudAnimateHealthBar(0);
                        HudAnimateHealthBar(1);
                    }
                    if (++gVBlankPlayerUpdateParity & 1)
                        HideFlaggedPlayerSprites(0);
                    else
                        HideFlaggedPlayerSprites(1);
                }
            }
        }
        if (!VBLANK_FLAG_3 || VBLANK_FLAG_6) {
            if (VBLANK_FLAG_8 != VBLANK_FLAG_7) {
                MusicSelectSongTemplateBank(VBLANK_FLAG_7);
                VBLANK_FLAG_8 = VBLANK_FLAG_7;
            } else
                MassiveRoutine();
            if (INPUT_READING_FLAG_LO & 1)
                P1_PAD_INPUT = GetCtrl1Input();
            else
                P2_PAD_INPUT = GetCtrl2Input();
        }
end_frame:
        ++TIME_COUNTER;
    }
}

void HandleHBlank()
{
    if (SKIP_HBLANK_FLAG && gLevel912C < 26) {
        SelectViewportSpriteChain(1);
        if (SPLIT_SCREEN || HBLANK_FLAG_1) {
            if (HBLANK_FLAG_1)
                VDP_CONTROL_WORD = 0x928e;
            else {
                VDP_CONTROL_WORD = 0x8230;
                VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
                VDP_DATA = P2_CAMERA_Y - 112;
                VDP_COMMAND = 0x7c000003;
                VDP_DATA = -P2_CAMERA_X;
                VDP_COMMAND = 0x40020010;
                VDP_DATA = P2_CAMERA_Y / 2 - 112;
                VDP_COMMAND = 0x7c020003;
                VDP_DATA = -P2_CAMERA_X / 2;
            }
        }
        VDP_CONTROL_WORD = 0x8aff;
    }
}

void GetPlayerInput()
{
    short player;
    struct object_state *entity;
    entity = gPlayerInfo;
    for (player = 0; player <= 1; ++player) {
        P1_PREV_PROCESSED_INPUT[player] = P1_PROCESSED_INPUT[player];
        if (gPlayerLives[player] >= 0) {
            P1_PROCESSED_INPUT[player] = MovePadInputIntoD0(player);
            if ((entity->animation_group_mask & PLAYER_GROUP_REACTION) || entity->sprite_state == PLAYER_ANIM_WHIRLWIND ||
                entity->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY || entity->sprite_state == PLAYER_ANIM_GHOST || entity->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
                P1_PROCESSED_INPUT[player] &= 0x80;
        } else
            P1_PROCESSED_INPUT[player] = 0;
        if (entity->animation_group_mask & PLAYER_GROUP_STATIONARY)
            P1_PROCESSED_INPUT[player] &= 0x8f;
        else if (entity->animation_group_mask & (PLAYER_GROUP_FALL | PLAYER_GROUP_LAND))
            P1_PROCESSED_INPUT[player] &= 0xcf;
        else if ((entity->animation_group_mask & PLAYER_GROUP_ALL_SKATES) || entity->sprite_state == PLAYER_ANIM_FLATTENED)
            P1_PROCESSED_INPUT[player] &= 0xef;
        entity = &gPlayerInfo[PLAYER_EARL];
    }
}

int CheckStartButton()
{
    if ((P1_PROCESSED_INPUT[PLAYER_TOEJAM] | P1_PROCESSED_INPUT[PLAYER_EARL]) & 0x80) {
        if (P1_PROCESSED_INPUT[PLAYER_TOEJAM] & 0x80) {
            if (!(P1_PREV_PROCESSED_INPUT[PLAYER_TOEJAM] & 0x80))
                GamePaused(0);
        } else if (!(gPlayer2PreviousProcessedInput & 0x80))
            GamePaused(1);
        return 1;
    }
    return 0;
}

void ProcessInput()
{
    unsigned char input1, input2, mask;
    short player, x, y;
    struct object_state *entity;
    unsigned char *map_cell;
    GetPlayerInput();
    CheckAButton();
    input1 = P1_PROCESSED_INPUT[PLAYER_TOEJAM];
    input2 = P1_PROCESSED_INPUT[PLAYER_EARL];
    if (CheckStartButton())
        return;
    if (gLevel912C < 26) {
        CheckBButton();
        if ((input1 | input2) & 0x20) {
            if (input1 & 0x20) {
                if (!P1_MENU_OPEN[PLAYER_TOEJAM]) {
                    DrawGameMap(0);
                    return;
                }
            } else if (!P1_MENU_OPEN[PLAYER_EARL]) {
                DrawGameMap(1);
                return;
            }
        }
    }
    for (player = 0; player <= 1; ++player) {
        if (gPlayerLives[player] >= 0 && !P1_MENU_OPEN[player]) {
            SpriteChecks(player, P1_PROCESSED_INPUT[player]);
            entity = &gPlayerInfo[player];
            x = entity->x - 72;
            y = entity->y - 8;
            if (x >= 0 && y >= 0 && x < 2560 && y < 1792) {
                map_cell = &gMapVisitedCoverBits[entity->level][y >> 8];
                mask = 0x80 >> (x / 320);
                if (!(*map_cell & mask)) {
                    *map_cell |= mask;
                    ++gPlayerPoints[player];
                    if (!SPLIT_SCREEN && gPlayerLives[player ^ 1] >= 0)
                        ++gPlayerPoints[player ^ 1];
                }
            }
        }
    }
}

void ReloadActiveCameraViews()
{
    SetCameraPosition(0, TOEJAM_CAMERA_NEWX, TOEJAM_CAMERA_NEWY);
    if (SPLIT_SCREEN)
        SetCameraPosition(1, EARL_CAMERA_NEWX, EARL_CAMERA_NEWY);
}
void EnterSplitScreen(animate)
char animate;
{
    short y;
    if (gMode2RegSoftcopy & 0x40) {
        while (VBLANK_FLAG_2)
            ;
        ClearSkipVHBlankFlags();
        VDP_CONTROL_WORD = 0x9200;
        HudHidePlayerSprites(2);
        if (animate) {
            AnimateSplitScreenWipe();
            AnimatePlayersIntoSplitViews();
        }
    }
    SPLIT_SCREEN = 1;
    if (!P1_MENU_OPEN[PLAYER_TOEJAM]) {
        y = gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_TOEJAM].height_pixels / 2 - 56;
        if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_TOEJAM].height / 2;
        SetCameraPosition(0, gPlayerInfo[PLAYER_TOEJAM].x - 160, y);
    }
    if (!HBLANK_FLAG_1) {
        y = gPlayerInfo[PLAYER_EARL].y - gPlayerInfo[PLAYER_EARL].height_pixels / 2 - 56;
        if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR)))
            y -= gPlayerInfo[PLAYER_EARL].height / 2;
        SetCameraPosition(1, gPlayerInfo[PLAYER_EARL].x - 160, y);
        P2_CAMERA_X = EARL_CAMERA_NEWX;
        P2_CAMERA_Y = EARL_CAMERA_NEWY;
    }
    if (gMode2RegSoftcopy & 0x40) {
        ForceTextBackgroundColorToBlack(0);
        VBLANK_FLAG_5 = 1;
        SetSkipVHBlankFlags();
    } else
        DrawHUD();
}

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
        ClearSkipVHBlankFlags();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    }
    SPLIT_SCREEN = 0;
    SetCameraPosition(0, x, y);
    EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    DrawHUD();
    if (gLevel912C < 26)
        CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    else
        CopyPaletteToCRAMSlot(PaletteShipScreen_bank3, 0);
    if (display_enabled) {
        ForceTextBackgroundColorToBlack(0);
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
        SetSkipVHBlankFlags();
    }
}

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
        ClearSkipVHBlankFlags();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    }
    SetCameraPosition(0, entity->x - 160, y);
    if (display_enabled) {
        ForceTextBackgroundColorToBlack(0);
        VBLANK_FLAG_5 = 1;
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
        SetSkipVHBlankFlags();
    } else
        DrawHUD();
}

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

void UpdateScreenSplitMode(force)
char force;
{
    short handled, distance_y, distance_x;
    if (force) {
        handled = 1;
        if (gPlayerLives[PLAYER_TOEJAM] < 0)
            CenterSharedCameraOnPlayer(1);
        else if (gPlayer2Lives < 0)
            CenterSharedCameraOnPlayer(0);
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
        if (distance_y > 124 || distance_x > 220 || P1_MENU_OPEN[PLAYER_TOEJAM] || HBLANK_FLAG_1)
            EnterSplitScreen(0);
        else
            EnterSharedScreen(distance_y, distance_x);
    } else {
        if (!SPLIT_SCREEN && (distance_y > 124 || distance_x > 220))
            EnterSplitScreen(1);
        else if (SPLIT_SCREEN && distance_y < 104 && distance_x < 200 &&
                 !P1_MENU_OPEN[PLAYER_TOEJAM] && !HBLANK_FLAG_1)
            EnterSharedScreen(distance_y, distance_x);
    }
    if (!SPLIT_SCREEN) {
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    }
}

/* Original busy-wait: the argument counts iterations, not video frames. */
void DelayRoutine1(iterations)
short iterations;
{
    while (iterations)
        --iterations;
}

void InitialiseTheZ80()
{
    char *destination = Z80_RAM;
    /* The zero reset value is kept across all six delay calls in D2. */
    short reset = 0;
    Z80_RESET = reset;
    DelayRoutine1(10);
    Z80_BUS_REQUEST = 0x100;
    DelayRoutine1(10);
    Z80_RESET = 0x100;
    DelayRoutine1(10);
    /* Install the six-byte Z80 bootstrap through its mapped RAM port.
     * These are the original hardware writes, not replacement 68000 code. */
    *destination++ = 0xed;
    *destination++ = 0x56;
    *destination++ = 0xf3;
    *destination++ = 0xc3;
    *destination++ = 3;
    *destination++ = reset;
    Z80_RESET = reset;
    DelayRoutine1(10);
    Z80_BUS_REQUEST = reset;
    DelayRoutine1(10);
    Z80_RESET = 0x100;
    DelayRoutine1(10);
}
/* Convert a one-based selection to a repeating zero-based six-entry index.
 * Keep signed C remainder behavior for zero or negative inputs. */
short GetLevelMusicIndex(selection)
short selection;
{
    return (selection - 1) % 6;
}

void FurtherInitialisation()
{
    /* ORIGINAL BUG: the matching source reads an uninitialized frame count.
     * The ROM has no explicit initialization of this local. Its allocated
     * D2 register last held TIME_COUNTER << 16, whose low word is zero.
     * [INFERENCE] The original source relied on an uninitialized frame count;
     * adding an initializer changes the bytes (and should not be hidden). */
    short frame;
    unsigned short time;
    gMode2RegSoftcopy = 0x60;
    ClearSkipVHBlankFlags();
    VDPWritesRoutine1(&TIME_COUNTER, 0, 1);
    RAMWritesRoutine1();
    InitialiseTheZ80();
    LoadSoundDriverToZ80();
    MassiveRoutine();
    EnableAllInterrupts();
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    StackBasedRandom(12345);
    PreGenerateRandoms();
    ClearColourRAM();
    ClearSRAM();
    ClearVideoRAM();
    VDP_COMMAND = 0x70000002;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    VDP_COMMAND = 0x40000001;
    MoveDataToVRAM(ImgStarfieldLarge, StarryBackgroundSize << 4);
    TextCopyFontBanksToRam();
    SetRAMIndexes();
    SEGAAndIntroVideoRoutine();
    NewGameMainMenu();
    VBLANK_FLAG_7 = 0;
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
    ToggleLOC_00FF8196(1);
    UpdateScreenSplitMode(1);
    StackBasedRandom(GetRandomNumber() | ((long)TIME_COUNTER << 16));
    GenerateLevelSeeds();
    DrawHUD();
    SetupInitialElevator();
    SetupInitialInventory();
    SetupInitialEarthlings();
    GetPlayerInput();
    P1_CAMERA_X = TOEJAM_CAMERA_NEWX;
    P1_CAMERA_Y = TOEJAM_CAMERA_NEWY;
    EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
    for (;;) {
        time = TIME_COUNTER;
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
                UpdateLandAroundPlayer(0);
                UpdateLandAroundPlayer(1);
                ProcessInput();
                ProcessPlayers();
                Process2FEntityFlag();
            }
            ProcessMovement();
            UpdateAllEntityViewportSprites();
            ProcessEntityAnimationMotion();
            UpdateSpeechAndDialogue();
        }
        while (time == TIME_COUNTER)
            GetRandomNumber();
    }
}
