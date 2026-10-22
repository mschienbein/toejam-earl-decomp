#include "hardware.h"
#include "assets_audio.h"
#include "assets_intro.h"
#include "assets_palettes.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "cutscene_text_calls.h"
#include "hardware_calls.h"
#include "intro_calls.h"
#include "intro_object_calls.h"
#include "jam_calls.h"
#include "ram_camera.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "reset_calls.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Run the intro parts in order; a nonzero part result ends the sequence. */
int IntroVideo()
{
    short interrupted;

    VBLANK_FLAG_7 = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    gMusicUnknownE489 = 0;
    TOEJAM_CAMERA_NEWX = 0;
    TOEJAM_CAMERA_NEWY = 0;
    VDP_COMMAND = 0x40000010;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    func_02743a();
    ram_ff8196 = 0x8000;
    VDP_COMMAND = 0x70000002;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    func_0273ae();
    func_0275e6(0);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    if (IntroPart1() || IntroPart2() || IntroPart3() ||
        IntroPart4() || IntroPart5() || IntroPart6() || IntroPart7() ||
        IntroPart8() || IntroPart9() || IntroPart10() || IntroPart11())
        interrupted = 1;
    else
        interrupted = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_02743a();
    func_02747c();
    func_0278e8();
    TOEJAM_CAMERA_NEWX = 0;
    TOEJAM_CAMERA_NEWY = 0;
    VDP_COMMAND = 0x40000010;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    return interrupted;
}

/* Initialize the large-character scene and select its background. */
void func_029ca4(background)
short background;
{
    func_028104();
    ram_ffe3a6 = background;
    func_0282fc();
    func_0272a0(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
}

/* Text and animation advance on the shared countdown or a released face button. */
short IntroPart1()
{
    short advance_scene;

    func_029ca4(7);
    advance_scene = 0;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 51;
    ram_ffe3fa = 1;
    while ((GetCtrl1Input() | GetCtrl2Input()) & 0x80)
        func_02791e(0, ram_ffe3fa);
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        do {
            func_02791e(0, ram_ffe3fa);
            if (ram_ffe3fe = (GetCtrl1Input() | GetCtrl2Input()) & 0x70)
                advance_scene = 1;
        } while (ram_ffe3fe);
        if (!advance_scene && --ram_ffe3fc > 0)
            continue;
        ++ram_ffe3f8;
        ram_ffe3fa = 1;
        switch (ram_ffe3f8) {
        case 1:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 25;
            break;
        case 2:
            advance_scene = 0;
            CutsceneTextLoad("Greetings,", 220, 80);
            ram_ffe3fc = 34;
            break;
        case 3:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 34;
            break;
        case 4:
            advance_scene = 0;
            CutsceneTextLoad("My name is Toejam.", 220, 80);
            func_02827e(6, dat_030764, 6);
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 59;
            break;
        case 5:
            func_02827e(6, dat_030762, 6);
            ram_ffe3fc = 34;
            break;
        case 6:
            advance_scene = 0;
            CutsceneTextLoad("And this here is my Bro", 220, 72);
            func_027f5e("Big Rappin Earl.", 220, 84);
            func_02827e(7, dat_030766, 6);
            func_02827e(5, dat_030744, 3);
            ram_ffe3fc = 51;
            break;
        case 7:
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fa = 0;
            ram_ffe3fc = 51;
            break;
        case 8:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 9:
            advance_scene = 0;
            func_02827e(1, dat_030753, 6);
            CutsceneTextLoad("Yo, Wha'sup.", 78, 54);
            ram_ffe3fc = 34;
            ram_ffe3fa = 0;
            break;
        case 10:
            func_02827e(1, dat_03076c, 6);
            ram_ffe3fc = 8;
            break;
        case 11:
            func_02827e(5, dat_030748, 6);
            ram_ffe3fc = 8;
            func_027e42();
            break;
        case 12:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 8;
            break;
        case 13:
            advance_scene = 0;
            CutsceneTextLoad("Earl and myself are", 220, 72);
            func_027f5e("highly funky aliens", 220, 84);
            func_02827e(6, dat_030764, 6);
            ram_ffe3fc = 8;
            break;
        case 14:
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 4;
            break;
        case 15:
            func_02827e(7, dat_03076a, 6);
            ram_ffe3fc = 17;
            break;
        case 16:
            func_02827e(7, dat_030768, 6);
            func_02827e(6, dat_030762, 6);
            ram_ffe3fc = 51;
            break;
        case 17:
            func_02827e(7, dat_030766, 6);
            ram_ffe3fc = 17;
            break;
        case 18:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 8;
            break;
        case 19:
            advance_scene = 0;
            CutsceneTextLoad("from outer space.", 220, 80);
            ram_ffe3fc = 59;
            break;
        case 20:
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 8;
            break;
        case 21:
            advance_scene = 0;
            CutsceneTextLoad("Outer Space.", 78, 54);
            ram_ffe3fc = 42;
            break;
        case 22:
            func_02827e(1, dat_03076c, 6);
            ram_ffe3fc = 34;
            break;
        case 23:
            ram_ffe3fc = 8;
            func_027e42();
            break;
        case 24:
            advance_scene = 0;
            CutsceneTextLoad("More specifically,", 220, 80);
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 34;
            break;
        case 25:
            func_02827e(6, dat_030764, 6);
            ram_ffe3fc = 8;
            break;
        case 26:
            func_02827e(6, dat_030762, 6);
            func_02827e(7, dat_030766, 6);
            ram_ffe3fc = 34;
            break;
        case 27:
            advance_scene = 0;
            CutsceneTextLoad("we are from", 220, 72);
            func_027f5e("the Planet Funkotron.", 220, 84);
            ram_ffe3fc = 85;
            break;
        case 28:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 17;
            break;
        case 29:
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 8;
            break;
        case 30:
            advance_scene = 0;
            CutsceneTextLoad("FUNKOTRON!", 78, 54);
            ram_ffe3fc = 17;
            break;
        case 31:
            func_02827e(5, dat_030744, 3);
            ram_ffe3fc = 34;
            break;
        case 32:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 33:
            advance_scene = 0;
            CutsceneTextLoad("Thank-you Earl.", 220, 80);
            ram_ffe3fc = 34;
            break;
        case 34:
            func_02827e(1, dat_030753, 6);
            ram_ffe3fc = 34;
            ram_ffe3fa = 0;
            break;
        case 35:
            func_02827e(5, dat_030748, 6);
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 17;
            break;
        case 36:
            advance_scene = 0;
            CutsceneTextLoad("Check this out -", 220, 72);
            ram_ffe3fc = 34;
            break;
        case 37:
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 17;
            break;
        case 38:
            func_027e42();
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 17;
            break;
        case 39:
            ram_ffe3f8 = -1;
        }
    }
    func_027e42();
    func_02836c();
    func_0281a8();
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Enable background advancement during the final rocketship caption pause. */
short IntroPart2()
{
    short advance_period;

    CopyPaletteToCRAMSlot(PaletteShipLine3, 2);
    func_028a0c();
    ram_ffe3f8 = 0;
    ram_ffe3fc = 17;
    advance_period = 0;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        do {
            func_02791e(advance_period, 0);
            if (ram_ffe3fe = (GetCtrl1Input() | GetCtrl2Input()) & 0x70)
                ram_ffe3fc = 0;
        } while (ram_ffe3fe);
        if (--ram_ffe3fc > 0)
            continue;
        ram_ffe3fc = 85;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            CutsceneTextLoad("This is our highly funky,", 160, 30);
            func_028af0();
            break;
        case 2:
            CutsceneTextLoad("Ultra Cool,", 160, 30);
            ram_ffe3fc = 51;
            break;
        case 3:
            CutsceneTextLoad("Righteous Rapmaster Rocketship.", 160, 30);
            ram_ffe3fc = 102;
            break;
        case 4:
            CutsceneTextLoad("Notice the Megawatt Speakers", 160, 30);
            break;
        case 5:
            CutsceneTextLoad("With which Earl and I", 160, 30);
            break;
        case 6:
            CutsceneTextLoad("have been known to crank out", 160, 30);
            break;
        case 7:
            CutsceneTextLoad("some extremely funky tunes.", 160, 30);
            ram_ffe3fc = 102;
            break;
        case 8:
            CutsceneTextLoad("Crank it Earl!", 160, 30);
            ram_ffe3fc = 51;
            break;
        case 9:
            func_027e42();
            VBLANK_FLAG_7 = 6;
            advance_period = 1;
            ram_ffe3fc = 306;
            break;
        case 10:
            ram_ffe3f8 = -1;
            advance_period = 0;
        }
    }
    func_027e42();
    func_028b20();
    func_02791e(0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    return ram_ffe3f8 != -1;
}

/* Return to the two large characters for the problem announcement. */
short IntroPart3()
{
    short advance_scene;

    func_029ca4(7);
    VBLANK_FLAG_7 = 0;
    ram_ffe3fc = 17;
    ram_ffe3fa = 1;
    advance_scene = 0;
    ram_ffe3f8 = 0;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        do {
            func_02791e(0, ram_ffe3fa);
            if (ram_ffe3fe = (GetCtrl1Input() | GetCtrl2Input()) & 0x70)
                advance_scene = 1;
        } while (ram_ffe3fe);
        if (!advance_scene && --ram_ffe3fc > 0)
            continue;
        ++ram_ffe3f8;
        ram_ffe3fa = 1;
        switch (ram_ffe3f8) {
        case 1:
            func_02827e(5, dat_030736, 3);
            func_02827e(6, dat_030764, 6);
            func_02827e(7, dat_030759, 6);
            ram_ffe3fc = 17;
            break;
        case 2:
            func_02827e(6, dat_030762, 6);
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 17;
            break;
        case 3:
            advance_scene = 0;
            CutsceneTextLoad("Recently, we ran into", 220, 72);
            func_027f5e("a small problem", 220, 84);
            ram_ffe3fc = 85;
            break;
        case 4:
            func_02827e(5, dat_030744, 3);
            ram_ffe3fc = 34;
            break;
        case 5:
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 17;
            break;
        case 6:
            func_02827e(1, dat_030753, 6);
            advance_scene = 0;
            CutsceneTextLoad("Problem.", 78, 54);
            ram_ffe3fc = 59;
            break;
        case 7:
            func_027e42();
            ram_ffe3fc = 8;
            break;
        case 8:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_02836c();
    func_0281a8();
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Scroll the background while alternating the rocketship exhaust update. */
short IntroPart4()
{
    short exhaust_phase, scroll_position;

    func_0292de();
    func_028b80();
    func_00046a();
    func_0275e6(1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    func_028c8a(0, 280, 160);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3fc = 85;
    ram_ffe3f8 = 0;
    scroll_position = 0;
    exhaust_phase = 0;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        do {
            WaitForNextVBlank(1);
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position / 4;
            SpriteLoadingStuff(0, 0);
            func_00059e(0);
            WaitForNextVBlank(1);
            func_029450(scroll_position);
            if (exhaust_phase ^= 1)
                func_028df2(exhaust_phase);
            func_00cc82();
            ++scroll_position;
            if (ram_ffe3fe = (GetCtrl1Input() | GetCtrl2Input()) & 0x70)
                ram_ffe3fc = 0;
        } while (ram_ffe3fe);
        if (--ram_ffe3fc > 0)
            continue;
        ram_ffe3fc = 85;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            CutsceneTextLoad("We were cruisin in our Rocket,", 160, 30);
            break;
        case 2:
            CutsceneTextLoad("Thumpin out a funky beat,", 160, 30);
            break;
        case 3:
            CutsceneTextLoad("When big Earl said", 160, 30);
            func_027f5e("he'd like a shot", 160, 40);
            ram_ffe3fc = 119;
            break;
        case 4:
            CutsceneTextLoad("in the driver's seat . . .", 160, 30);
            break;
        case 5:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_029414();
    func_028c4c();
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Change Earl's driving poses and move the background in either direction. */
short IntroPart5()
{
    short scroll_direction, driving_pose, scroll_position;

    ram_ffe3a6 = 8;
    func_0273ae();
    func_0283c8();
    func_029510();
    func_0276f4(11, 7, 30, 20);
    func_0276f4(12, 8, 29, 19);
    func_0284a8(0, 104, 151);
    func_0284a8(1, 104, 151);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteIntroEarlDriving_bank3, 3);
    ram_ffe3f8 = 0;
    ram_ffe3fc = 42;
    driving_pose = 1;
    scroll_direction = 0;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_02791e(0, 0);
        func_029692(driving_pose);
        if (scroll_direction) {
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position;
            scroll_position += scroll_direction << 2;
        }
        if (--ram_ffe3fc != 0)
            continue;
        ram_ffe3fc = 42;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            func_028558(1, &IntroHeldFrameStreams[2].frame, 6);
            func_0284a8(2, 104, 151);
            ram_ffe3fc = 17;
            break;
        case 2:
            func_028558(2, dat_030720, 3);
            ram_ffe3fc = 51;
            break;
        case 3:
            func_028558(2, &dat_0307c2.frame, 6);
            ram_ffe3fc = 17;
            break;
        case 4:
            func_02859a(2);
            func_028558(1, &IntroHeldFrameStreams[1].frame, 6);
            break;
        case 5:
            func_028558(1, &IntroHeldFrameStreams[5].frame, 6);
            PlaySoundEffect((long)Sfx_Running, 50, 0);
            driving_pose = 2;
            scroll_direction = -1;
            break;
        case 6:
            func_028558(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 7:
            func_028558(1, &IntroHeldFrameStreams[6].frame, 6);
            PlaySoundEffect((long)Sfx_Running, 50, 0);
            driving_pose = 3;
            scroll_direction = 1;
            break;
        case 8:
            func_028558(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 9:
            func_028558(1, &IntroHeldFrameStreams[5].frame, 6);
            PlaySoundEffect((long)Sfx_Running, 50, 0);
            driving_pose = 2;
            scroll_direction = -1;
            break;
        case 10:
            func_028558(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 11:
            func_029692(0);
            ram_ffe3f8 = -1;
            break;
        }
    }
    func_02846c();
    func_029552();
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Step through the ship's dodging poses while the background scrolls. */
short IntroPart6()
{
    short pose_changed, driving_pose, scroll_position;

    func_028b80();
    func_029510();
    ram_ffe3f8 = 0;
    ram_ffe3fc = 42;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position / 4;
    ram_ffe3a6 = 9;
    func_0272a0(1);
    func_028c8a(0, 104, 151);
    pose_changed = 0;
    driving_pose = 0;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_02958e(pose_changed, driving_pose);
        pose_changed = 0;
        func_02791e(0, 0);
        VDP_COMMAND = 0x7c020003;
        VDP_DATA = scroll_position / 4;
        ++scroll_position;
        if (--ram_ffe3fc != 0)
            continue;
        ram_ffe3fc = 4;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            func_028c8a(1, 104, 151);
            break;
        case 2:
            func_028c8a(2, 104, 151);
            PlaySound(21, 5);
            break;
        case 3:
            func_028c8a(3, 104, 151);
            ram_ffe3fc = 51;
            pose_changed = 1;
            driving_pose = 1;
            break;
        case 4:
            func_028c8a(2, 104, 151);
            break;
        case 5:
            func_028c8a(1, 104, 151);
            pose_changed = 1;
            driving_pose = 2;
            PlaySound(22, 5);
            break;
        case 6:
            func_028c8a(0, 104, 151);
            ram_ffe3fc = 51;
            break;
        case 7:
            func_028c8a(1, 104, 151);
            break;
        case 8:
            func_028c8a(2, 104, 151);
            pose_changed = 1;
            driving_pose = 1;
            PlaySound(21, 5);
            break;
        case 9:
            func_028c8a(3, 104, 151);
            ram_ffe3fc = 51;
            break;
        case 10:
            ram_ffe3f8 = -1;
            break;
        }
    }
    func_028c4c();
    func_029552();
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Alternate camera shake during the final driving poses and asteroid entry. */
short IntroPart7()
{
    short shake_phase, scroll_direction, driving_pose, scroll_position;

    func_0283c8();
    func_029510();
    CopyPaletteToCRAMSlot(PaletteIntroEarlDriving_bank0, 0);
    shake_phase = 0;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 34;
    driving_pose = 1;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position / 4;
    scroll_direction = 0;
    ram_ffe3a6 = 10;
    func_0272a0(1);
    func_0284a8(0, 104, 151);
    func_0284a8(1, 104, 151);
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_0272a0(0);
        if (shake_phase) {
            if (shake_phase == 1) {
                TOEJAM_CAMERA_NEWX += 5;
                TOEJAM_CAMERA_NEWY += 5;
                shake_phase = 2;
            } else if (shake_phase == 2) {
                TOEJAM_CAMERA_NEWX -= 5;
                TOEJAM_CAMERA_NEWY -= 5;
                shake_phase = 1;
            }
        }
        VBLANK_FLAG_2 = 0;
        func_00cc82();
        WaitForNextVBlank(1);
        if (shake_phase) {
            VDP_COMMAND = 0x40000010;
            VDP_DATA = TOEJAM_CAMERA_NEWY;
            VDP_COMMAND = 0x7c000003;
            VDP_DATA = -TOEJAM_CAMERA_NEWX;
        }
        SpriteLoadingStuff(0, 0);
        func_00059e(0);
        func_029692(driving_pose);
        if (scroll_direction) {
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position / 4;
            scroll_position += scroll_direction << 3;
        }
        if (--ram_ffe3fc != 0)
            continue;
        ram_ffe3fc = 34;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            func_028558(1, &dat_0307a2.frame, 6);
            PlaySoundEffect((long)Sfx_Running, 50, 0);
            scroll_direction = -1;
            driving_pose = 2;
            break;
        case 2:
            break;
        case 3:
            func_028558(1, &dat_03079a.frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 4:
            func_028558(1, &dat_0307a4.frame, 6);
            PlaySoundEffect((long)Sfx_Running, 50, 0);
            scroll_direction = 1;
            driving_pose = 3;
            break;
        case 5:
            func_028558(1, &dat_03079a.frame, 6);
            driving_pose = 4;
            scroll_direction = 0;
            break;
        case 6:
            shake_phase = 1;
            func_0285da();
            PlaySound(10, 5);
            break;
        case 7:
            ram_ffe3f8 = -1;
            shake_phase = 0;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_028650();
    func_029552();
    func_02846c();
    TOEJAM_CAMERA_NEWX = 0;
    TOEJAM_CAMERA_NEWY = 0;
    VDP_COMMAND = 0x40000010;
    VDP_DATA = TOEJAM_CAMERA_NEWY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = TOEJAM_CAMERA_NEWX;
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Load the crash screen, then show and release the collision objects. */
short IntroPart8()
{
    ram_ffe3f8 = 0;
    ram_ffe3fc = 4;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteIntroCollision_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    VDP_COMMAND = 0x46600001;
    MoveDataToVRAM(CrashScreen, CrashScreenSize << 4);
    ram_ffd936 = 1;
    func_02743a();
    func_02747c();
    func_03b988(0, 0);
    func_03b988(20, 0);
    func_03b988(0, 14);
    func_03b988(20, 14);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_02791e(0, 0);
        if (--ram_ffe3fc == 0) {
            ++ram_ffe3f8;
            switch (ram_ffe3f8) {
            case 1:
                func_028670();
                PlaySoundEffect((long)Sfx_Falling, 50, 0);
                ram_ffe3fc = 59;
                break;
            case 2:
                func_0287c0();
                ram_ffe3f8 = -1;
                break;
            }
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_02791e(0, 0);
    ram_ffd936 = 0;
    func_00046a();
    func_0273ae();
    return ram_ffe3f8 != -1;
}

/* Scroll the second background plane through two timed scene states. */
short IntroPart9()
{
    short scroll_position;

    ram_ffe3a6 = 12;
    func_0272a0(1);
    func_0276f4(11, 7, 30, 20);
    func_0276f4(12, 8, 29, 19);
    func_029128();
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    ram_ffe3f8 = 0;
    scroll_position = 0;
    ram_ffe3fc = 42;
    func_0291be(170, 141);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_02791e(0, 0);
        VDP_COMMAND = 0x40020010;
        VDP_DATA = scroll_position;
        ++scroll_position;
        if (--ram_ffe3fc != 0)
            continue;
        ram_ffe3fc = 85;
        ++ram_ffe3f8;
        switch (ram_ffe3f8) {
        case 1:
            break;
        case 2:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_02919e();
    scroll_position = 0;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = scroll_position;
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}

/* Increase a scene Y argument from 20 to 160, then show the driving warning. */
short IntroPart10()
{
    short scene_y;

    func_0275e6(1);
    func_029128();
    ram_ffe3fc = 42;
    scene_y = 20;
    func_02994c(160, 180);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    ram_ffe3f8 = 1;
    while (ram_ffe3f8 != -1 &&
        !((GetCtrl1Input() | GetCtrl2Input()) & 0x80)) {
        func_02791e(0, 0);
        switch (ram_ffe3f8) {
        case 1:
            ++scene_y;
            func_0291be(160, scene_y);
            if (scene_y == 160) {
                ++ram_ffe3f8;
                func_02919e();
            }
            break;
        case 2:
            func_0298b0(160, 180);
            PlaySound(8, 50);
            ram_ffe3fc = 85;
            ++ram_ffe3f8;
            break;
        case 3:
            if (--ram_ffe3fc == 0)
                ++ram_ffe3f8;
            break;
        case 4:
            func_029930();
            CutsceneTextLoad("you should never let earl drive", 160, 30);
            ram_ffe3fc = 119;
            ++ram_ffe3f8;
            break;
        case 5:
            if (--ram_ffe3fc == 0)
                ++ram_ffe3f8;
            break;
        case 6:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_0299cc();
    func_02919e();
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}
