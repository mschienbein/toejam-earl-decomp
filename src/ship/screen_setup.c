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
extern char ram_ffb8da[];

/* Expand each nonzero packed pixel nibble to palette index fifteen. */
void func_01faf4(tile_count)
short tile_count;
{
    unsigned char packed_pixels;
    short byte_number;
    unsigned char *pixels;

    tile_count <<= 5;
    pixels = (unsigned char *)&ram_ffb8da[SPRITE_STATE_TABLE];
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
void func_01fb40(force_frame)
char force_frame;
{
    short frame_number = 0, part_number;
    struct render_frame *frame;
    struct render_piece *part;
    char packing;

    if (force_frame) {
        frame_number = 1;
        ram_ffe21e = 12;
    } else if (--ram_ffe21e <= 0) {
        if (gShipPieceLevels[0] == 1)
            frame_number = 2;
        else
            frame_number = 1;
        ram_ffe21e = 12;
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
            SPRITE_STATE_TABLE += 512;
        }
        ram_ffd8dc[ram_ffd92e++] = 1;
        ram_ffd8dc[ram_ffd92e++] = 128;
        WaitForNextVBlank(1);
        func_008be4(8);
        SpriteLoadingStuff(1, 1);
        gShipPieceLevels[0] = frame_number;
    } else {
        WaitForNextVBlank(1);
        func_008be4(8);
    }
}

/* Restore game tiles, HUD sprites, and all four game palettes. */
void func_01fc58()
{
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    VDP_COMMAND = 0x40000000L;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    VDP_COMMAND = 0x40000001L;
    MoveDataToVRAM(ImgStarfieldLarge, StarryBackgroundSize << 4);
    VDP_COMMAND = 0x46600001L;
    MoveDataToVRAM(InventoryLabelOpen, InventoryLabelOpenSize << 4);
    VDP_COMMAND = 0x79a00000L;
    MoveDataToVRAM(InventoryLabelIdentify, InventoryLabelIdentifySize << 4);
    DrawHUD();
    VDPWritesRoutine1(ram_ffb65a, 0xf800L, 320);
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ClearVHBlankFlag2();
    VDP_CONTROL_WORD = 0x8c89;
}

/* Advance the two-frame transition background every twelve calls. */
void func_01fd58(background_number)
short background_number;
{
    short part_number, frame_number;
    struct render_frame *frame;
    struct render_piece *part;
    char packing;

    frame_number = 0;
    if (--ram_ffe21e <= 0) {
        if (gShipPieceLevels[0] == 1)
            frame_number = 2;
        else
            frame_number = 1;
        ram_ffe21e = 12;
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
            SPRITE_STATE_TABLE += 512;
        }
        ram_ffd8dc[ram_ffd92e++] = 1;
        ram_ffd8dc[ram_ffd92e++] = 64;
        WaitForNextVBlank(1);
        func_008be4(8);
        SpriteLoadingStuff(1, 1);
        gShipPieceLevels[0] = frame_number;
    } else {
        WaitForNextVBlank(1);
        func_008be4(8);
    }
}

/* Run the 480-tick ship completion sequence and restore the game scene. */
void func_01fe68(player_number)
short player_number;
{
    short repeat, column, row, page;
    unsigned short tile;
    short sound_stage = 1;

    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    for (page = 0; page < 4; ++page) {
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = 0x50000003L + ((page * 8 + row) << 23);
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
                VDP_COMMAND = (0x50000003L + (row << 23)) + (column << 17);
                VDP_DATA = tile + 0x8580L;
            }
        }
    }
    TIME_COUNTER = 0;
    ram_ffe21e = 0;
    gShipPieceLevels[0] = 1;
    func_01fd58(20);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    PlaySound(6, 99);
    func_03e4f8(0);
    WaitForNextVBlank(1);
    VBLANK_FLAG_8 = 6;
    VBLANK_FLAG_7 = 6;
    func_03e43c();
    func_03e40e();
    PlaySoundEffect((long)Sfx_JamClap, 90, 0);
    while (TIME_COUNTER < 480) {
        func_01fd58(20);
        if ((sound_stage == 1 && TIME_COUNTER > 20) ||
            (sound_stage == 2 && !TOEJAM_SPEECH_COUNTER) ||
            (sound_stage == 3 && TIME_COUNTER > 60)) {
            PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            ++sound_stage;
        } else if (sound_stage == 4 && !TOEJAM_SPEECH_COUNTER) {
            PlaySoundEffect((long)Sfx_Hallelujah, 92, 1);
            ++sound_stage;
        } else if (sound_stage == 5 && !TOEJAM_SPEECH_COUNTER) {
            PlaySoundEffect((long)Sfx_Jammin, 90, 0);
            ++sound_stage;
        } else if (sound_stage == 6 && !TOEJAM_SPEECH_COUNTER)
            VBLANK_FLAG_7 = 0;
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    func_01fc58();
    if (P1_MENU_OPEN[player_number ^ 1]) {
        OpeningPresents4(player_number ^ 1);
        P1_MENU_OPEN[player_number ^ 1] = 0;
    }
    func_00cfba(0);
    SPLIT_SCREEN = 0;
    func_00b95c(2);
    gPlayer1Powerup[0] = -1;
    gPlayer2Powerup = -1;
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    func_012f34();
    func_037470();
    func_013036();
}

/* Fill the ship-screen plane and random star border, then install palettes. */
void func_020182()
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
            VDP_COMMAND = 0x50000003L + ((page * 8 + row) << 23);
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
                VDP_COMMAND = (0x50000003L + (row << 23)) + (column << 17);
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
    func_01fb40(1);
}

/* Center the selected piece's name and animate the background for 90 ticks. */
void func_020310(piece_number)
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
    func_009f8a(piece_name);
    for (character_count = 0; character_count < 90; ++character_count)
        func_01fb40(0);
}

/* Report a missing piece on this player's level and count all missing pieces. */
void func_0204ce(player_number)
int player_number;
{
    short piece_number, missing_pieces = 0, piece_on_level = 0;
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    for (piece_number = 0; piece_number < 10; ++piece_number) {
        if (ram_ffe212[piece_number]) {
            ++missing_pieces;
            if (ram_ffe212[piece_number] == player->level)
                piece_on_level = 1;
        }
    }
    if (piece_on_level) {
        if (gPlayerInfo[player_number ^ 1].level == player->level) {
            VDP_COMMAND = 0x518a0003L;
            func_009f8a("Rocketship piece on this level");
        } else if (player_number == 0) {
            VDP_COMMAND = 0x51860003L;
            func_009f8a("Rocketship piece on toejam's level");
        } else {
            VDP_COMMAND = 0x51880003L;
            func_009f8a("Rocketship piece on earl's level");
        }
    } else {
        if (gPlayerInfo[player_number ^ 1].level == player->level) {
            VDP_COMMAND = 0x51900003L;
            func_009f8a("no pieces on this level");
        } else if (player_number == 0) {
            VDP_COMMAND = 0x518c0003L;
            func_009f8a("no pieces on toejam's level");
        } else {
            VDP_COMMAND = 0x518e0003L;
            func_009f8a("no pieces on earl's level");
        }
    }
    VDP_COMMAND = 0x5c0e0003L;
    func_009fbe(missing_pieces);
    func_009f8a(" missing rocketship pieces");
}
