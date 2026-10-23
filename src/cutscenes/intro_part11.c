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
    IntroAllocateLargePlayerParts();
    gSceneBackgroundPairIndex = 7;
    InitializeTitleTransitionTilemap();
    CutsceneDrawRandomizedScreenBorder(0);
    IntroPositionLargePlayerParts();
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    advance_pressed = 0;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 51;
    gCutsceneMusicActorAnimationEnabled = 1;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
           !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        do {
            CutsceneStepFrameAndMusicCadence(0, gCutsceneMusicActorAnimationEnabled);
            if (gCutsceneAdvanceButtonMask = (GetCtrl1Input() | GetCtrl2Input()) & INPUT_ACTION_MASK)
                advance_pressed = 1;
        } while (gCutsceneAdvanceButtonMask);
        if (!advance_pressed && --gCutsceneScriptCountdown > 0)
            continue;
        ++gCutsceneScriptState;
        gCutsceneMusicActorAnimationEnabled = 1;
        switch (gCutsceneScriptState) {
        case 1:
            advance_pressed = 0;
            CutsceneTextLoad("Yo, so here we are", 220, 80);
            gCutsceneScriptCountdown = 42;
            break;
        case 2:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 3:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 25;
            break;
        case 4:
            advance_pressed = 0;
            CutsceneTextLoad("chillin' out on", 220, 72);
            CutsceneLoadSecondTextLine("the most insane planet", 220, 84);
            gCutsceneScriptCountdown = 34;
            break;
        case 5:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameEighteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 6:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 7:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSixteen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 8:
            advance_pressed = 0;
            CutsceneTextLoad("in the Galaxy.", 220, 80);
            gCutsceneScriptCountdown = 68;
            break;
        case 9:
            advance_pressed = 0;
            CutsceneTextLoad("Planet Earth!", 220, 80);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 51;
            break;
        case 10:
            CutsceneReleaseTextEntities();
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 34;
            break;
        case 11:
            advance_pressed = 0;
            CutsceneTextLoad("thanks to earl . . .", 220, 80);
            gCutsceneScriptCountdown = 34;
            break;
        case 12:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveAscendingFrames, 3);
            gCutsceneScriptCountdown = 34;
            break;
        case 13:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 17;
            break;
        case 14:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameNineteen, 6);
            IntroPositionLargePlayerPart(3, 148, 112);
            gCutsceneScriptCountdown = 8;
            break;
        case 15:
            IntroSetLargePlayerPartSequence(3, IntroLargePlayerPartThreeLoopAnimation, 3);
            gCutsceneMusicActorAnimationEnabled = 0;
            gCutsceneScriptCountdown = 42;
            break;
        case 16:
            IntroReleaseLargePlayerPartSprites(3);
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 17:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameOne, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 18:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveDescendingFrames, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 19:
            advance_pressed = 0;
            CutsceneTextLoad("All we have to do now,", 220, 80);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameEighteen, 6);
            gCutsceneScriptCountdown = 85;
            break;
        case 20:
            advance_pressed = 0;
            CutsceneTextLoad("is find the 10 pieces", 220, 72);
            CutsceneLoadSecondTextLine("of our Rocketship,", 220, 84);
            gCutsceneScriptCountdown = 25;
            break;
        case 21:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 25;
            break;
        case 22:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 23:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameEighteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 24:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 25;
            break;
        case 25:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 17;
            break;
        case 26:
            advance_pressed = 0;
            CutsceneTextLoad("and we can", 220, 72);
            CutsceneLoadSecondTextLine("Jet Outta here.", 220, 84);
            gCutsceneScriptCountdown = 34;
            break;
        case 27:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 68;
            break;
        case 28:
            advance_pressed = 0;
            CutsceneTextLoad("no problem.", 220, 80);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameEighteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 29:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 30:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 31:
            advance_pressed = 0;
            CutsceneTextLoad("no problem.", 78, 54);
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameThree, 6);
            gCutsceneMusicActorAnimationEnabled = 0;
            gCutsceneScriptCountdown = 85;
            break;
        case 32:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    IntroReleaseLargePlayerSpritesAndRefresh();
    IntroReleaseLargePlayerParts();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}
