#include "hardware.h"
#include "assets_intro.h"
#include "assets_palettes.h"
#include "core_calls.h"
#include "cutscene_text_calls.h"
#include "hardware_calls.h"
#include "intro_calls.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_system.h"
#include "reset_calls.h"
#include "title_calls.h"

/* Advance the final intro dialogue on a timed delay or a released A/B/C press.
 * Start stops the scene early; the return value reports that early exit. */
short IntroPart11()
{
    short advance_pressed;

    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    func_028104();
    ram_ffe3a6 = 7;
    func_0273ae();
    func_0275e6(0);
    func_0282fc();
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    advance_pressed = 0;
    ram_ffe3f8 = 0;
    ram_ffe3fc = 51;
    ram_ffe3fa = 1;
    while (ram_ffe3f8 != -1 &&
           !((GetCtrl1Input() | GetCtrl2Input()) & 128)) {
        do {
            func_02791e(0, ram_ffe3fa);
            if (ram_ffe3fe = (GetCtrl1Input() | GetCtrl2Input()) & 112)
                advance_pressed = 1;
        } while (ram_ffe3fe);
        if (!advance_pressed && --ram_ffe3fc > 0)
            continue;
        ++ram_ffe3f8;
        ram_ffe3fa = 1;
        switch (ram_ffe3f8) {
        case 1:
            advance_pressed = 0;
            CutsceneTextLoad("Yo, so here we are", 220, 80);
            ram_ffe3fc = 42;
            break;
        case 2:
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 17;
            break;
        case 3:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 25;
            break;
        case 4:
            advance_pressed = 0;
            CutsceneTextLoad("chillin' out on", 220, 72);
            func_027f5e("the most insane planet", 220, 84);
            ram_ffe3fc = 34;
            break;
        case 5:
            func_02827e(6, dat_030764, 6);
            func_02827e(7, dat_03076a, 6);
            ram_ffe3fc = 17;
            break;
        case 6:
            func_02827e(7, dat_030768, 6);
            func_02827e(6, dat_030762, 6);
            ram_ffe3fc = 34;
            break;
        case 7:
            func_02827e(7, dat_030766, 6);
            ram_ffe3fc = 34;
            break;
        case 8:
            advance_pressed = 0;
            CutsceneTextLoad("in the Galaxy.", 220, 80);
            ram_ffe3fc = 68;
            break;
        case 9:
            advance_pressed = 0;
            CutsceneTextLoad("Planet Earth!", 220, 80);
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 51;
            break;
        case 10:
            func_027e42();
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 34;
            break;
        case 11:
            advance_pressed = 0;
            CutsceneTextLoad("thanks to earl . . .", 220, 80);
            ram_ffe3fc = 34;
            break;
        case 12:
            func_02827e(5, dat_030744, 3);
            ram_ffe3fc = 34;
            break;
        case 13:
            func_02827e(1, dat_03076e, 6);
            func_027e42();
            ram_ffe3fc = 17;
            break;
        case 14:
            func_02827e(1, dat_030760, 6);
            func_0281e4(3, 148, 112);
            ram_ffe3fc = 8;
            break;
        case 15:
            func_02827e(3, dat_03075d, 3);
            ram_ffe3fa = 0;
            ram_ffe3fc = 42;
            break;
        case 16:
            func_0282c0(3);
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 8;
            break;
        case 17:
            func_02827e(1, dat_03076c, 6);
            ram_ffe3fc = 8;
            break;
        case 18:
            func_02827e(5, dat_030748, 6);
            ram_ffe3fc = 17;
            break;
        case 19:
            advance_pressed = 0;
            CutsceneTextLoad("All we have to do now,", 220, 80);
            func_02827e(7, dat_03076a, 6);
            ram_ffe3fc = 85;
            break;
        case 20:
            advance_pressed = 0;
            CutsceneTextLoad("is find the 10 pieces", 220, 72);
            func_027f5e("of our Rocketship,", 220, 84);
            ram_ffe3fc = 25;
            break;
        case 21:
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 25;
            break;
        case 22:
            func_02827e(6, dat_030764, 6);
            ram_ffe3fc = 17;
            break;
        case 23:
            func_02827e(6, dat_030762, 6);
            func_02827e(7, dat_03076a, 6);
            ram_ffe3fc = 17;
            break;
        case 24:
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 25;
            break;
        case 25:
            func_027e42();
            ram_ffe3fc = 17;
            break;
        case 26:
            advance_pressed = 0;
            CutsceneTextLoad("and we can", 220, 72);
            func_027f5e("Jet Outta here.", 220, 84);
            ram_ffe3fc = 34;
            break;
        case 27:
            func_02827e(5, dat_030736, 3);
            ram_ffe3fc = 68;
            break;
        case 28:
            advance_pressed = 0;
            CutsceneTextLoad("no problem.", 220, 80);
            func_02827e(6, dat_030764, 6);
            func_02827e(7, dat_03076a, 6);
            ram_ffe3fc = 17;
            break;
        case 29:
            func_02827e(6, dat_030762, 6);
            func_02827e(7, dat_030768, 6);
            ram_ffe3fc = 34;
            break;
        case 30:
            func_02827e(1, dat_03076e, 6);
            ram_ffe3fc = 17;
            break;
        case 31:
            advance_pressed = 0;
            CutsceneTextLoad("no problem.", 78, 54);
            func_02827e(1, dat_030753, 6);
            ram_ffe3fa = 0;
            ram_ffe3fc = 85;
            break;
        case 32:
            ram_ffe3f8 = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_027e42();
    func_02836c();
    func_0281a8();
    func_02791e(0, 0);
    return ram_ffe3f8 != -1;
}
