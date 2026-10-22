#include "present_types.h"
#include "psg_effects.h"
#include "fm_effects.h"
#include "music_ids.h"
#define TJE_LOCAL_RAM_FFB8DA

#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_audio.h"
#include "assets_map.h"
#include "assets_palettes.h"
#include "assets_scenes.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "cutscene_calls.h"
#include "elevator_calls.h"
#include "hardware_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_ship.h"
#include "ram_speech.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "ship.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Compatible incomplete-array compiler view. The actual complete shared
 * declaration changes the observed instructions; historical source spelling
 * is unknown [INFERENCE]. */
extern char gSpriteTileUploadBuffer[];

/* Expand each nonzero packed pixel nibble to palette index fifteen. */
void FillSpriteOpaquePixels(tile_count)
short tile_count;
{
    unsigned char packed_pixels;
    short byte_number;
    unsigned char *pixels;

    tile_count <<= 5;
    pixels = (unsigned char *)&gSpriteTileUploadBuffer[gSpriteTileBufferOffset];
    for (byte_number = 0; byte_number < tile_count; ++byte_number) {
        packed_pixels = *pixels;
        if (packed_pixels & 15)
            packed_pixels |= 15;
        if (packed_pixels & 240)
            packed_pixels |= 240;
        *pixels++ = packed_pixels;
    }
}

/* Advance the ship background every twelve calls, or force its first frame. */
void AnimateShipScreenBackground(force_frame)
char force_frame;
{
    short frame_number = 0, part_number;
    struct render_frame *frame;
    struct render_piece *part;
    char packing;

    if (force_frame) {
        frame_number = 1;
        gShipBackgroundAnimationTimer = 12;
    } else if (--gShipBackgroundAnimationTimer <= 0) {
        if (gShipScreenState[0] == 1)
            frame_number = 2;
        else
            frame_number = 1;
        gShipBackgroundAnimationTimer = 12;
    }
    if (frame_number) {
        /* Only pointer-word slot one is known to hold a frame table; the
         * other ROM pointer remains opaque. */
        frame = ((struct render_frame **)AnimationPointerWords_AF040[1])
            [frame_number + 9];
        for (part_number = 0; part_number < 8; ++part_number) {
            /* Compiler view: retain the renderer's twelve-slot word-index
             * capacity, not a fixed twelve-piece extent on each ROM frame. */
            part = &((struct render_piece (*)[12])(frame + 1))[0][part_number];
            packing = part->format;
            if (packing < 0)
                SpriteUnpackInterleavedRLE(part, 16);
            else
                SpriteUnpackRLE(part, 16);
            gSpriteTileBufferOffset += 512;
        }
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 1;
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 128;
        WaitForNextVBlank(1);
        TextAnimateFontBanks(8);
        SpriteLoadingStuff(1, 1);
        gShipScreenState[0] = frame_number;
    } else {
        WaitForNextVBlank(1);
        TextAnimateFontBanks(8);
    }
}

/* Restore game tiles, HUD sprites, and all four game palettes. */
void RestoreGameplayGraphics()
{
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    VDP_COMMAND = 0x40000001L;
    MoveDataToVRAM(ImgStarfieldLarge, StarryBackgroundSize << 4);
    VDP_COMMAND = 0x46600001L;
    MoveDataToVRAM(InventoryLabelOpen, InventoryLabelOpenSize << 4);
    VDP_COMMAND = 0x79a00000L;
    MoveDataToVRAM(InventoryLabelIdentify, InventoryLabelIdentifySize << 4);
    DrawHUD();
    VDPWritesRoutine1(gHardwareSpriteTable, 0xf800L, 320);
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ClearVHBlankFlag2();
    VDP_CONTROL_WORD = 0x8c89;
}

/* Advance the two-frame transition background every twelve calls. */
void AnimateTransitionBackground(background_number)
short background_number;
{
    short part_number, frame_number;
    struct render_frame *frame;
    struct render_piece *part;
    char packing;

    frame_number = 0;
    if (--gShipBackgroundAnimationTimer <= 0) {
        if (gShipScreenState[0] == 1)
            frame_number = 2;
        else
            frame_number = 1;
        gShipBackgroundAnimationTimer = 12;
    }
    if (frame_number) {
        frame = Spr_TransitionBackground[background_number * 2 + frame_number - 1];
        for (part_number = 0; part_number < 4; ++part_number) {
            /* Compiler view: four transition payloads retain the original
             * bounded word-index arithmetic after the four-byte prefix. */
            part = &((struct render_piece (*)[4])(frame + 1))[0][part_number];
            packing = part->format;
            if (packing < 0)
                SpriteUnpackInterleavedRLE(part, 16);
            else
                SpriteUnpackRLE(part, 16);
            gSpriteTileBufferOffset += 512;
        }
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 1;
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 64;
        WaitForNextVBlank(1);
        TextAnimateFontBanks(8);
        SpriteLoadingStuff(1, 1);
        gShipScreenState[0] = frame_number;
    } else {
        WaitForNextVBlank(1);
        TextAnimateFontBanks(8);
    }
}

/* Run the 480-tick ship completion sequence and restore the game scene. */
void PlayShipCompletionSequence(player_number)
short player_number;
{
    short repeat, column, row, page;
    unsigned short tile;
    short sound_stage = 1;

    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    for (page = 0; page < 4; ++page) {
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = VDP_WRITE_WINDOW + ((page * 8 + row) << 23);
            for (repeat = 0; repeat < 6; ++repeat) {
                for (column = 0; column < 8; ++column) {
                    tile = ShipRelatedData[row][column] + 1;
                    VDP_DATA = tile + 0xa000L;
                }
            }
        }
    }
    for (row = 0; row < 28; ++row) {
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39) {
                tile = GetRandomNumber() & 3;
                tile += 41;
                if (GetRandomNumber() & 1) {
                    tile |= 0x800;
                    tile |= 0x1000;
                }
                VDP_COMMAND = (VDP_WRITE_WINDOW + (row << 23)) + (column << 17);
                VDP_DATA = tile + 0x8580L;
            }
        }
    }
    TIME_COUNTER = 0;
    gShipBackgroundAnimationTimer = 0;
    gShipScreenState[0] = 1;
    AnimateTransitionBackground(20);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
    AudioRequestChannelTemplateEffect(FM_EFFECT_CLEAR);
    WaitForNextVBlank(1);
    VBLANK_FLAG_8 = MUSIC_TRANSITION;
    VBLANK_FLAG_7 = MUSIC_TRANSITION;
    AudioRequestFmChannelRelease();
    MusicRequestTrackPointerReset();
    PlaySoundEffect((long)Sfx_JamClap, 90, 0);
    while (TIME_COUNTER < 480) {
        AnimateTransitionBackground(20);
        if ((sound_stage == 1 && TIME_COUNTER > 20) ||
            (sound_stage == 2 && !gPcmEffectCooldownTicks) ||
            (sound_stage == 3 && TIME_COUNTER > 60)) {
            PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            ++sound_stage;
        } else if (sound_stage == 4 && !gPcmEffectCooldownTicks) {
            PlaySoundEffect((long)Sfx_Hallelujah, 92, 1);
            ++sound_stage;
        } else if (sound_stage == 5 && !gPcmEffectCooldownTicks) {
            PlaySoundEffect((long)Sfx_Jammin, 90, 0);
            ++sound_stage;
        } else if (sound_stage == 6 && !gPcmEffectCooldownTicks)
            VBLANK_FLAG_7 = 0;
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    RestoreGameplayGraphics();
    if (P1_MENU_OPEN[player_number ^ 1]) {
        InventoryDeactivateDisplayEntities(player_number ^ 1);
        P1_MENU_OPEN[player_number ^ 1] = 0;
    }
    SelectSpriteChainStart(0);
    SPLIT_SCREEN = 0;
    HudHidePlayerSprites(2);
    gPlayer1Powerup[PLAYER_TOEJAM] = PRESENT_NONE;
    gPlayer2Powerup = PRESENT_NONE;
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    MapClearActiveLevelObjects();
    PlayEndingSequence();
    MapEnterFinalLevel();
}

/* Fill the ship-screen plane and random star border, then install palettes. */
void InitializeShipScreenTilemapAndPalettes()
{
    short first_tile, column, row, page;
    register short repeat;
    unsigned short tile;

    for (page = 0; page < 4; ++page) {
        /* ORIGINAL BUG: page never reaches four inside this loop, so this
         * conditional exit cannot run; row is not initialized until below. */
        if (page == 4 && row == 3)
            break;
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = VDP_WRITE_WINDOW + ((page * 8 + row) << 23);
            for (repeat = 0; repeat < 5; ++repeat) {
                first_tile = ((page ^ repeat) & 1) << 6;
                ++first_tile;
                for (column = 0; column < 8; ++column) {
                    tile = ShipRelatedData[row][column] + first_tile;
                    VDP_DATA = tile + 0xa000L;
                }
            }
        }
    }
    for (row = 0; row < 28; ++row) {
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39) {
                tile = GetRandomNumber() & 3;
                tile += 41;
                if (GetRandomNumber() & 1) {
                    tile |= 0x800;
                    tile |= 0x1000;
                }
                VDP_COMMAND = (VDP_WRITE_WINDOW + (row << 23)) + (column << 17);
                VDP_DATA = tile + 0x8580L;
            }
        }
    }
    VDP_CONTROL_WORD = 0x911e;
    VDP_CONTROL_WORD = 0x921c;
    CopyPaletteToCRAMSlot(PaletteShipLine1, 0);
    CopyPaletteToCRAMSlot(PaletteShipLine2, 1);
    CopyPaletteToCRAMSlot(PaletteShipLine3, 2);
    CopyPaletteToCRAMSlot(PaletteAllBlack, 3);
    VDP_CONTROL_WORD = 0x8c81;
    SetVHBlankFlag2();
    AnimateShipScreenBackground(1);
}

/* Center the selected piece's name and animate the background for 90 ticks. */
void ShowShipPieceName(piece_number)
short piece_number;
{
    short character_count;
    char *piece_name;

    /* ORIGINAL BUG: an out-of-range piece leaves piece_name uninitialized. */
    switch (piece_number) {
    case 0: piece_name = "rocketship windshield"; break;
    case 1: piece_name = "left megawatt speaker"; break;
    case 2: piece_name = "super funkomatic amplamator"; break;
    case 3: piece_name = "amplamator connector fin"; break;
    case 4: piece_name = "forward stabilizing unit"; break;
    case 5: piece_name = "rear leg"; break;
    case 6: piece_name = "awesome snowboard"; break;
    case 7: piece_name = "righteous rapmaster capsule"; break;
    case 8: piece_name = "right megawatt speaker"; break;
    case 9: piece_name = "hyperfunk thruster"; break;
    }
    for (character_count = 0; piece_name[character_count]; ++character_count)
        ;
    VDP_COMMAND = 0x5c000003L + ((20 - character_count / 2) << 17);
    TextWriteStringTiles(piece_name);
    for (character_count = 0; character_count < 90; ++character_count)
        AnimateShipScreenBackground(0);
}

/* Report a missing piece on this player's level and count all missing pieces. */
void ShowMissingShipPieceSummary(player_number)
int player_number;
{
    short piece_number, missing_pieces = 0, piece_on_level = 0;
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    for (piece_number = 0; piece_number < 10; ++piece_number) {
        if (gShipPieceLevelAssignments[piece_number]) {
            ++missing_pieces;
            if (gShipPieceLevelAssignments[piece_number] == player->level)
                piece_on_level = 1;
        }
    }
    if (piece_on_level) {
        if (gPlayerInfo[player_number ^ 1].level == player->level) {
            VDP_COMMAND = 0x518a0003L;
            TextWriteStringTiles("Rocketship piece on this level");
        } else if (player_number == PLAYER_TOEJAM) {
            VDP_COMMAND = 0x51860003L;
            TextWriteStringTiles("Rocketship piece on toejam's level");
        } else {
            VDP_COMMAND = 0x51880003L;
            TextWriteStringTiles("Rocketship piece on earl's level");
        }
    } else {
        if (gPlayerInfo[player_number ^ 1].level == player->level) {
            VDP_COMMAND = 0x51900003L;
            TextWriteStringTiles("no pieces on this level");
        } else if (player_number == PLAYER_TOEJAM) {
            VDP_COMMAND = 0x518c0003L;
            TextWriteStringTiles("no pieces on toejam's level");
        } else {
            VDP_COMMAND = 0x518e0003L;
            TextWriteStringTiles("no pieces on earl's level");
        }
    }
    VDP_COMMAND = 0x5c0e0003L;
    TextWriteDecimalTiles(missing_pieces);
    TextWriteStringTiles(" missing rocketship pieces");
}

/* Flash the final ship palette through the stepped brightness schedule.
 * The signed word views preserve the original palette sign extensions. */
void AnimateShipScreenPalette()
{
    short low_component, middle_component, high_component;
    short frame_number, color_number, fade_step = 0;
    unsigned short palette[16];

    for (frame_number = 0; frame_number < 16; ++frame_number) {
        palette[frame_number] = 0;
        AnimateShipScreenBackground(0);
    }
    PlaySound(PSG_LONG_NOISE_RISE, 90);
    for (frame_number = 1; frame_number < 61; ++frame_number) {
        if (frame_number == 8 || frame_number == 16 || frame_number == 22 ||
            frame_number == 28 || frame_number == 32 || frame_number == 36 ||
            frame_number == 38 || frame_number == 40)
            ++fade_step;
        else if (frame_number == 44 || frame_number == 48 ||
                 frame_number == 54 || frame_number == 60)
            --fade_step;
        else {
            AnimateShipScreenBackground(0);
            continue;
        }
        for (color_number = 1; color_number < 16; ++color_number) {
            high_component = PaletteFadeLookup[fade_step - 1]
                [((short)PaletteShipLine3[color_number] >> 9) & 7];
            middle_component = PaletteFadeLookup[fade_step - 1]
                [(short)((long)(short)PaletteShipLine3[color_number] >> 5) & 7];
            low_component = PaletteFadeLookup[fade_step - 1]
                [(short)((long)(short)PaletteShipLine3[color_number] >> 1) & 7];
            palette[color_number] = (high_component << 9) |
                (middle_component << 5) | (low_component << 1);
        }
        WaitForNextVBlank(1);
        CopyPaletteToCRAMSlot(palette, 3);
    }
}
