#include "revision.h"
#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_intro.h"
#include "assets_palettes.h"
#include "assets_scenes.h"
#include "core_calls.h"
#include "graphics_calls.h"
#include "hardware_calls.h"
#include "intro_calls.h"
#include "ram_camera.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "random.h"
#include "reset_calls.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Advance the two-frame background animation and transfer its four parts. */
void AnimateTitleTransitionBackground(restart)
char restart;
{
    short frame_phase = 0;
    short part_number, frame_number;
    char packing;
    struct render_frame *frame;
    struct render_piece *part;

    if (restart) {
        frame_phase = 1;
        gSceneBackgroundFrameTimer = 8;
    } else if (--gSceneBackgroundFrameTimer <= 0) {
        if (gSceneBackgroundFramePhase == 1)
            frame_phase = 2;
        else
            frame_phase = 1;
        gSceneBackgroundFrameTimer = 8;
    }
    if (frame_phase) {
        frame_number = gSceneBackgroundPairIndex * 2;
        frame_number += frame_phase;
        --frame_number;
        frame = Spr_TransitionBackground[frame_number];
        for (part_number = 0; part_number < 4; ++part_number) {
            /* These transition frames have exactly four payload pieces. */
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
        SpriteLoadingStuff(1, 1);
        gSceneBackgroundFramePhase = frame_phase;
        VDP_CONTROL_WORD = 0x8228;
    } else
        WaitForNextVBlank(1);
}

/* Repeat the eight-by-eight transition tile pattern over four row bands. */
void InitializeTitleTransitionTilemap()
{
    short column, repeat_number, row, band;
    unsigned short tile;
    for (band = 0; band < 4; ++band)
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = 0x60000002L + ((band * 8 + row) << 23);
            for (repeat_number = 0; repeat_number < 8; ++repeat_number)
                for (column = 0; column < 8; ++column) {
                    tile = ShipRelatedData[row][column];
                    ++tile;
                    VDP_DATA = gCameraTileAttributes + 0x2000 + tile;
                }
        }
    AnimateTitleTransitionBackground(1);
}

/* Fill the 64-by-32 plane at command base 0x60000003 with tile 0x8000. */
void ClearUpperVramTilemap()
{
    short column, row;
    WaitForNextVBlank(1);
    for (row = 0; row < 32; ++row) {
        VDP_COMMAND = 0x60000003L + (row << 23);
        for (column = 0; column < 64; ++column)
            VDP_DATA = 0x8000;
    }
}

/* Fill the 64-by-32 plane at command base 0x60000002 with tile 0x8000. */
void ClearLowerVramTilemap()
{
    short column, row;
    WaitForNextVBlank(1);
    for (row = 0; row < 32; ++row) {
        VDP_COMMAND = 0x60000002L + (row << 23);
        for (column = 0; column < 64; ++column)
            VDP_DATA = 0x8000;
    }
}

/* Select either title logo and write its 40-column tile map. */
void TitleDrawSelectedLogo()
{
    short column, row;
    unsigned short tile;
    ClearUpperVramTilemap();
    if (!gTitleLogoSelector) {
        VDP_COMMAND = 0x48800001L;
        MoveDataToVRAM(LogoScreen, LogoScreenSize << 4);
        for (row = 0; row < 28; ++row)
            if (row >= 1 && row < 26) {
                VDP_COMMAND = 0x60000003L + (row << 23);
                for (column = 0; column < 40; ++column) {
                    tile = TextMapping[row - 1][column];
                    VDP_DATA = tile + 0x8244;
                }
            }
    } else {
        VDP_COMMAND = 0x48800001L;
        MoveDataToVRAM(TAndELogoScreen, TAndELogoScreenSize << 4);
        for (row = 0; row < 28; ++row)
            if (row >= 4 && row < 21) {
                VDP_COMMAND = 0x60000003L + (row << 23);
                for (column = 0; column < 40; ++column) {
                    tile = TitleLogoTilemap[row - 4][column];
                    VDP_DATA = tile + 0x8244;
                }
            }
    }
}

/* Draw randomized edge tiles, optionally clearing the interior as well. */
void CutsceneDrawRandomizedScreenBorder(clear_interior)
short clear_interior;
{
    unsigned short tile;
    short column, row;
    WaitForNextVBlank(1);
    if (clear_interior) {
        for (row = 0; row < 28; ++row) {
            VDP_COMMAND = 0x60000002L + (row << 23);
            for (column = 0; column < 40; ++column)
                if (row == 0 || row == 27 || column == 0 || column == 39) {
                    tile = (GetRandomNumber() & 3) + 41;
                    if ((int)GetRandomNumber() & 1) {
                        tile |= 0x800;
                        tile |= 0x1000;
                    }
                    VDP_DATA = 0x8580 + tile;
                } else
                    VDP_DATA = 0x8000;
        }
    } else {
        for (row = 0; row < 28; ++row)
            for (column = 0; column < 40; ++column)
                if (row == 0 || row == 27 || column == 0 || column == 39) {
                    tile = (GetRandomNumber() & 3) + 41;
                    VDP_COMMAND = 0x60000002L + (row << 23) + (column << 17);
                    if ((int)GetRandomNumber() & 1) {
                        tile |= 0x800;
                        tile |= 0x1000;
                    }
                    VDP_DATA = 0x8580 + tile;
                }
    }
}

/* Draw the same randomized border around an inclusive tile rectangle. */
void CutsceneDrawRandomizedRectangleBorder(left, top, right, bottom)
short left, top, right, bottom;
{
    unsigned short tile;
    short column, row;
    WaitForNextVBlank(1);
    for (row = top; row <= bottom; ++row) {
        VDP_COMMAND = 0x60000002L + (row << 23) + (left << 17);
        for (column = left; column <= right; ++column)
            if (row == top || row == bottom || column == left || column == right) {
                tile = (GetRandomNumber() & 3) + 41;
                if ((int)GetRandomNumber() & 1) {
                    tile |= 0x800;
                    tile |= 0x1000;
                }
                VDP_DATA = 0x8580 + tile;
            } else
                VDP_DATA = 0x8244;
    }
}

/* Draw one 16-by-8 metatile at a tile coordinate. */
void CutsceneDrawMetatileBlock(metatile, left, top)
short metatile, left, top;
{
    short column, row;
    for (row = 0; row < 8; ++row) {
        VDP_COMMAND = 0x60000002L + ((top + row) << 23) + (left << 17);
        for (column = 0; column < 16; ++column)
            VDP_DATA = 0x8000 + GetLevelMetatileTile(metatile, row, column);
    }
}

/* Lay out the nine observed metatiles in three rows. */
void EndingDrawNineMetatileBackground()
{
    CutsceneDrawMetatileBlock(1, 0, 0);
    CutsceneDrawMetatileBlock(7, 16, 0);
    CutsceneDrawMetatileBlock(2, 32, 0);
    CutsceneDrawMetatileBlock(6, 0, 8);
    CutsceneDrawMetatileBlock(30, 16, 8);
    CutsceneDrawMetatileBlock(4, 32, 8);
    CutsceneDrawMetatileBlock(0, 0, 16);
    CutsceneDrawMetatileBlock(5, 16, 16);
    CutsceneDrawMetatileBlock(3, 32, 16);
}

/* Cover a four-by-four metatile area with metatile 82. */
void EndingFillRepeatedMetatileBackground()
{
    short row, column;
    for (column = 0; column < 4; ++column)
        for (row = 0; row < 4; ++row)
            CutsceneDrawMetatileBlock(82, column * 16, row * 8);
}

/* Examine the first 34 records and pass indices 6 through 39 to deletion. */
void CutsceneReleaseFlaggedEntityRange()
{
    unsigned short object_number = 6;
    struct object_state *object = gPlayerInfo;
    for (; object_number < 40; ++object_number, ++object)
        if (object->draw_flags & 1)
            FreeEntity(object_number);
}

/* Tick the background and object systems; music events gate the scene counter. */
int CutsceneStepFrameAndMusicCadence(advance_period, animate_objects)
unsigned short advance_period;
short animate_objects;
{
    AnimateTitleTransitionBackground(0);
    VBLANK_FLAG_2 = 0;
    UpdateAndQueueEntitySprites();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
    SelectViewportSpriteChain(0);
    ++gSceneUpdateCounter;
    if (!gMusicUnknownE488)
        return 1;
    gMusicUnknownE488 = 0;
    if (TIME_COUNTER - gSceneLastMusicEventTime < 10)
        return 1;
    gSceneLastMusicEventTime = TIME_COUNTER;
    if (animate_objects)
        IntroSetLargePlayerPartSequence(2, &IntroFrameIndexLists[67], 6);
    if (gSceneUpdateCounter > 51) {
        gSceneUpdateCounter = 0;
        return 0;
    }
    if (advance_period) {
        ++gSceneBackgroundAdvanceCounter;
        if (advance_period > gSceneBackgroundAdvanceCounter)
            return 1;
        else {
            gSceneBackgroundAdvanceCounter = 0;
            gSceneBackgroundPairIndex = (gSceneBackgroundPairIndex + 1) % 24;
            AnimateTitleTransitionBackground(1);
        }
    }
    return 1;
}

/* Display alternating title logos until start is released or 1,200 ticks pass. */
unsigned short TitleScreenVideo()
{
    long buttons;
    unsigned short start_pressed;
#if REVISION < 2
    unsigned short start_time;
#endif
    short random_word;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    VDP_CONTROL_WORD = 0x8c81;
    gCameraTileAttributes = 0;
    InitializeTitleTransitionTilemap();
    TitleDrawSelectedLogo();
    gMusicUnknownE489 = 2;
    gSceneBackgroundAdvanceCounter = 0;
    start_pressed = 0;
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
#if REVISION >= 2
    TIME_COUNTER = 0;
    gSceneLastMusicEventTime = 0;
    while (TIME_COUNTER < 1200) {
#else
    start_time = TIME_COUNTER;
    gSceneLastMusicEventTime = TIME_COUNTER;
    while (TIME_COUNTER - start_time < 1200) {
#endif
        /* Preserve the observed discarded word read after advancing the RNG. */
        random_word = GetRandomNumber();
        if (!CutsceneStepFrameAndMusicCadence(3, 0)) {
            gTitleLogoSelector ^= 1;
            TitleDrawSelectedLogo();
        }
        buttons = GetCtrl1Input() | GetCtrl2Input();
        if (buttons & 0x80) {
            while ((GetCtrl1Input() | GetCtrl2Input()) & 0x80)
                WaitForNextVBlank(1);
            start_pressed = 1;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    return start_pressed;
}

/* Cycle the Sega, title and intro scenes, then restore the four game palettes. */
void SEGAAndIntroVideoRoutine()
{
    short show_intro;
    show_intro = 1;
    VBLANK_FLAG_6 = 1;
    for (;;) {
        SEGAVideo();
        if (TitleScreenVideo())
            break;
        if (IntroVideo()) {
            show_intro = 0;
            break;
        }
    }
    if (show_intro)
        IntroVideo();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    VBLANK_FLAG_6 = 0;
    VDP_CONTROL_WORD = 0x8c89;
}