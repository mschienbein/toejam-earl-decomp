#define TJE_LOCAL_FUNC_0289DA

#include "hardware.h"
#include "assets_audio.h"
#include "assets_map.h"
#include "assets_palettes.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "cutscene_calls.h"
#include "cutscene_text_calls.h"
#include "hardware_calls.h"
#include "intro_object_calls.h"
#include "ram_camera.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "ram_system.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int func_0289da();

/* Reset both camera scrolls, load the text and level tiles, and play the ending. */
void func_037470()
{
    VBLANK_FLAG_7 = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ClearSkipVHBlankFlags();
    TOEJAM_CAMERA_NEWX = 0;
    TOEJAM_CAMERA_NEWY = 0;
    VDP_CONTROL_WORD = 0x8228;
    VDP_COMMAND = 0x40000010L;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    VDP_COMMAND = 0x40020010L;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c020003L;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    func_02743a();
    ram_ff8196 = 0x8000;
    VDP_COMMAND = 0x70000002L;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    VDP_COMMAND = 0x40000000L;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    func_00046a();
    func_0375c8();
    func_037a02();
    func_037c00();
    func_0380c0();
    func_03824a();
    func_027e42();
    func_02747c();
    func_0278e8();
}

/* Advance the celebration dialogue and the accelerating exit animation. */
short func_0375c8()
{
    short character_x, character_y, mouth_timer, speed, clap_played;

    func_028b80();
    func_028e74();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    func_02747c();
    func_02782e();
    VDP_COMMAND = 0x40000010L;
    VDP_DATA = -30;
    character_y = 127;
    character_x = 92;
    speed = 0;
    clap_played = 0;
    func_028d54(3, character_x, character_y);
    func_028ef0(0, character_x, 127);
    func_028844(character_x, 127);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    mouth_timer = 0;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 34;
    while (ram_ffe3f8 != -1) {
        WaitForNextVBlank(1);
        SpriteLoadingStuff(0, 0);
        func_00059e(0);
        WaitForNextVBlank(1);
        func_00cc82();
        switch (ram_ffe3f8) {
        case 0:
            if (!--ram_ffe3fc) {
                if (PLAYER_INDICATOR == 2)
                    func_028034("we did it", 90, 95);
                else
                    func_028034("i did it", 90, 95);
                ram_ffe3fc = 51;
                ++ram_ffe3f8;
            }
            break;
        case 1:
            if (!--ram_ffe3fc) {
                func_027e42();
                func_0291ec(character_x, 127);
                ram_ffe3fc = 323;
                ++ram_ffe3f8;
            }
            break;
        case 2:
            if (PLAYER_INDICATOR == 2 && !clap_played &&
                28 == func_0289da()) {
                clap_played = 1;
                PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            }
            if (!--ram_ffe3fc) {
                ++ram_ffe3f8;
                func_02899c();
                func_0292a4();
                ram_ffe3fc = 34;
            }
            break;
        case 3:
            if (!--ram_ffe3fc) {
                ++ram_ffe3f8;
                ram_ffe3fc = 51;
            }
            break;
        case 4:
            if (!--ram_ffe3fc)
                ++ram_ffe3f8;
            break;
        case 5:
            if (--character_y < 107) {
                ram_ffe3fc = 17;
                ++ram_ffe3f8;
            }
            if (character_y == 112)
                func_028ef0(1, character_x, 127);
            func_028e36(character_x, character_y);
            break;
        case 6:
            if (!--ram_ffe3fc)
                ++ram_ffe3f8;
            break;
        case 7:
            if (PLAYER_INDICATOR == 2)
                func_028034("crank it earl!", 160, 30);
            else
                func_028034(" crank it!", 160, 30);
            ram_ffe3fc = 51;
            ++ram_ffe3f8;
        case 8:
            if (!--ram_ffe3fc)
                ++ram_ffe3f8;
            break;
        case 9:
            func_029284();
            func_027e42();
            func_028f82();
            func_028ffa(character_x, -50);
            VBLANK_FLAG_7 = 6;
            ram_ffe3fc = 221;
            ++ram_ffe3f8;
            break;
        case 10:
            if (gMusicUnknownE488) {
                func_028ffa(character_x, character_y);
                mouth_timer = 4;
                gMusicUnknownE488 = 0;
            }
            if (!--mouth_timer)
                func_028ffa(character_x, -50);
            if (!--ram_ffe3fc) {
                ++ram_ffe3f8;
                PlaySoundEffect((long)Sfx_Running, 50, 0);
                func_029036();
            }
            break;
        case 11:
            if ((character_x -= speed) < -30)
                ++ram_ffe3f8;
            ++speed;
            func_028e36(character_x, character_y);
            func_028ef0(1, character_x, 127);
            break;
        case 12:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_029284();
    func_02899c();
    func_028c4c();
    func_028f62();
    func_029036();
    func_027e42();
    func_02791e(0, 0);
    func_02747c();
    speed = 0;
    VDP_COMMAND = 0x40000010L;
    VDP_DATA = speed;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    return 0;
}

/* Move the character upward and left while displaying the driving exchange. */
short func_037a02()
{
    short character_x, character_y, move_timer, move_period;

    func_0275e6(1);
    func_028b80();
    VBLANK_FLAG_7 = 0;
    character_y = 195;
    character_x = 112;
    move_timer = 15;
    move_period = move_timer;
    func_028c8a(0, character_x, character_y);
    func_02994c(160, 180);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 17;
    while (ram_ffe3f8 != -1) {
        func_02791e(0, 0);
        switch (ram_ffe3f8) {
        case 0:
            if (!--ram_ffe3fc)
                ++ram_ffe3f8;
            break;
        case 1:
            if (move_timer & 1)
                --character_y;
            if (!--move_timer) {
                if (!(move_period >>= 1))
                    move_period = 1;
                move_timer = move_period;
                --character_x;
                if (character_x == 107 && PLAYER_INDICATOR == 2)
                    CutsceneTextLoad("Can I drive?", character_x + 70,
                                     character_y - 85);
            }
            func_028e36(character_x, character_y);
            if (character_y <= 132)
                ++ram_ffe3f8;
            break;
        case 2:
            --character_x;
            func_028e36(character_x, character_y);
            if (character_x == 57 && PLAYER_INDICATOR == 2)
                CutsceneTextLoad("No!", character_x + 35, character_y - 80);
            if (character_x <= -30)
                ++ram_ffe3f8;
            break;
        case 3:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_0299cc();
    func_028c4c();
    func_02791e(0, 0);
    return 0;
}

/* Scroll the background while alternating credit pages and blank intervals. */
short func_037c00()
{
    short scroll, animation_phase;

    func_0292de();
    func_028b80();
    func_00046a();
    func_0275e6(1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    func_028c8a(0, 280, 160);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3fc = 85;
    ram_ffe3f8 = 0;
    scroll = 0;
    animation_phase = 0;
    while (ram_ffe3f8 != -1) {
        WaitForNextVBlank(1);
        VDP_COMMAND = 0x7c020003L;
        VDP_DATA = scroll / 4;
        SpriteLoadingStuff(0, 0);
        func_00059e(0);
        WaitForNextVBlank(1);
        func_029450(scroll);
        if (!(animation_phase = (animation_phase + 1) % 3))
            func_028df2(1);
        func_00cc82();
        ++scroll;
        if (--ram_ffe3fc)
            continue;
        ram_ffe3fc = 102;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            CutsceneTextLoad("game design -- greg johnson", 160, 35);
            func_027f5e("game program -- mark voorsanger", 160, 48);
            ram_ffe3fc = 136;
            break;
        case 2:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 3:
            CutsceneTextLoad("additional programming         ", 160, 35);
            func_027f5e("             -- robert leyland", 160, 45);
            break;
        case 4:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 5:
            CutsceneTextLoad("music direction -- mark miller", 160, 35);
            func_027f5e("music composition -- john baker", 160, 48);
            break;
        case 6:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 7:
            CutsceneTextLoad("artwork -- greg johnson", 160, 35);
            func_027f5e("           avril harrison", 160, 45);
            break;
        case 8:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 9:
            CutsceneTextLoad("sound fx -- robert leyland", 160, 35);
            func_027f5e("            mark miller ", 160, 45);
            break;
        case 10:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 11:
            CutsceneTextLoad("invaluable aid -- paul reiche", 160, 35);
            func_027f5e("                  fred ford", 160, 45);
            break;
        case 12:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 13:
            ram_ffe3fc = 8;
            break;
        case 14:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 15:
            CutsceneTextLoad("producer -- scott berfield", 160, 35);
            func_027f5e("awesome support -- hugh bowen", 160, 48);
            break;
        case 16:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 17:
            CutsceneTextLoad("a johnson-voorsanger production", 160, 35);
            ram_ffe3fc = 136;
            break;
        case 18:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_029414();
    func_028c4c();
    func_02791e(0, 0);
    return 0;
}

/* Move the character left, then curve downward out of the scene. */
short func_0380c0()
{
    short character_x, character_y, move_period, move_timer;

    func_0275e6(1);
    func_028b80();
    ram_ffe3fc = 42;
    character_y = 132;
    character_x = 280;
    move_timer = 20;
    move_period = move_timer;
    func_028c8a(0, character_x, character_y);
    CopyPaletteToCRAMSlot(EndingPaletteA, 3);
    func_0299e8(170, 180);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3f8 = 0;
    while (ram_ffe3f8 != -1) {
        func_02791e(0, 0);
        switch (ram_ffe3f8) {
        case 0:
            if (--character_x == 182)
                ++ram_ffe3f8;
            func_028e36(character_x, character_y);
            break;
        case 1:
            --character_x;
            if (!--move_timer) {
                if (!(move_period >>= 1))
                    move_period = 1;
                move_timer = move_period;
                ++character_y;
            }
            func_028e36(character_x, character_y);
            if (character_x == 122)
                ++ram_ffe3f8;
            break;
        case 2:
            if (character_y < 185)
                ++character_y;
            else if (++move_period & 1)
                ++character_y;
            func_028e36(character_x, character_y);
            if (character_y >= 195)
                ++ram_ffe3f8;
            break;
        case 3:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_029a68();
    func_028c4c();
    func_02791e(0, 0);
    return 0;
}

/* Scroll the plane and move the character vertically while swapping companions. */
short func_03824a()
{
    short character_y, character_x, velocity_y, acceleration_y, scroll;
    short movement_frame;

    func_028b80();
    func_028e74();
    func_029056();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(EndingPaletteB, 3);
    func_0278ae();
    character_y = 72;
    character_x = 152;
    scroll = 0;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = scroll;
    velocity_y = 0;
    acceleration_y = 1;
    func_028d54(3, character_x, character_y);
    func_028ef0(1, character_x, 127);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 17;
    while (ram_ffe3f8 != -1) {
        VBLANK_FLAG_2 = 0;
        func_00cc82();
        WaitForNextVBlank(1);
        SpriteLoadingStuff(0, 0);
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = scroll;
        func_00059e(0);
        scroll += 5;
        if (++ram_ffe3f8 & 1)
            --character_x;
        character_y += velocity_y;
        /* ORIGINAL BUG: the ROM increments this stack word without ever
         * initializing it; its initial movement phase is indeterminate. */
        if (!(++movement_frame % 9))
            velocity_y += acceleration_y;
        if (character_y >= 137) {
            if (!velocity_y)
                acceleration_y = 0;
            else
                velocity_y = -velocity_y + 1;
        }
        if (character_y >= 127) {
            func_028e36(character_x, 127);
            func_0290d2(character_x, 127);
            func_028ef0(0, character_x, 127);
        } else {
            func_028e36(character_x, character_y);
            func_0290d2(character_x, -50);
            if (character_y < 107)
                func_028ef0(1, character_x, 127);
            else
                func_028ef0(0, character_x, 127);
        }
        if (character_x < -30)
            ram_ffe3f8 = -1;
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_028c4c();
    func_028f62();
    func_029108();
    func_02791e(0, 0);
    func_02747c();
    scroll = 0;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = scroll;
    return 0;
}
