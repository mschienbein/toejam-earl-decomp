#include "psg_effects.h"
#include "music_ids.h"
#include "entities.h"
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

    gRequestedMusicSelection = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gMusicAnimationChannel = 0;
    gCamera1PendingX = 0;
    gCamera1PendingY = 0;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = gCamera1PendingX;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = gCamera1PendingX;
    ClearUpperVramTilemap();
    gCameraTileAttributes = VDP_TILE_PRIORITY;
    VDP_COMMAND = 0x70000002;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    InitializeTitleTransitionTilemap();
    CutsceneDrawRandomizedScreenBorder(0);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    if (IntroPart1() || IntroPart2() || IntroPart3() ||
        IntroPart4() || IntroPart5() || IntroPart6() || IntroPart7() ||
        IntroPart8() || IntroPart9() || IntroPart10() || IntroPart11())
        interrupted = 1;
    else
        interrupted = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    ClearUpperVramTilemap();
    ClearLowerVramTilemap();
    CutsceneReleaseFlaggedEntityRange();
    gCamera1PendingX = 0;
    gCamera1PendingY = 0;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = gCamera1PendingX;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = gCamera1PendingX;
    return interrupted;
}

/* Initialize the large-character scene and select its background. */
void IntroBeginLargePlayerScene(background)
short background;
{
    IntroAllocateLargePlayerParts();
    gSceneBackgroundPairIndex = background;
    IntroPositionLargePlayerParts();
    AnimateTitleTransitionBackground(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
}

/* Text and animation advance on the shared countdown or a released face button. */
short IntroPart1()
{
    short advance_scene;

    IntroBeginLargePlayerScene(7);
    advance_scene = 0;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 51;
    gCutsceneMusicActorAnimationEnabled = 1;
    while ((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)
        CutsceneStepFrameAndMusicCadence(0, gCutsceneMusicActorAnimationEnabled);
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        do {
            CutsceneStepFrameAndMusicCadence(0, gCutsceneMusicActorAnimationEnabled);
            if (gCutsceneAdvanceButtonMask = (GetCtrl1Input() | GetCtrl2Input()) & INPUT_ACTION_MASK)
                advance_scene = 1;
        } while (gCutsceneAdvanceButtonMask);
        if (!advance_scene && --gCutsceneScriptCountdown > 0)
            continue;
        ++gCutsceneScriptState;
        gCutsceneMusicActorAnimationEnabled = 1;
        switch (gCutsceneScriptState) {
        case 1:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 25;
            break;
        case 2:
            advance_scene = 0;
            CutsceneTextLoad("Greetings,", 220, 80);
            gCutsceneScriptCountdown = 34;
            break;
        case 3:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 34;
            break;
        case 4:
            advance_scene = 0;
            CutsceneTextLoad("My name is Toejam.", 220, 80);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 59;
            break;
        case 5:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 6:
            advance_scene = 0;
            CutsceneTextLoad("And this here is my Bro", 220, 72);
            CutsceneLoadSecondTextLine("Big Rappin Earl.", 220, 84);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSixteen, 6);
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveAscendingFrames, 3);
            gCutsceneScriptCountdown = 51;
            break;
        case 7:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneMusicActorAnimationEnabled = 0;
            gCutsceneScriptCountdown = 51;
            break;
        case 8:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 9:
            advance_scene = 0;
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameThree, 6);
            CutsceneTextLoad("Yo, Wha'sup.", 78, 54);
            gCutsceneScriptCountdown = 34;
            gCutsceneMusicActorAnimationEnabled = 0;
            break;
        case 10:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameOne, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 11:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveDescendingFrames, 6);
            gCutsceneScriptCountdown = 8;
            CutsceneReleaseTextEntities();
            break;
        case 12:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 8;
            break;
        case 13:
            advance_scene = 0;
            CutsceneTextLoad("Earl and myself are", 220, 72);
            CutsceneLoadSecondTextLine("highly funky aliens", 220, 84);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 14:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 4;
            break;
        case 15:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameEighteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 16:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            gCutsceneScriptCountdown = 51;
            break;
        case 17:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSixteen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 18:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 8;
            break;
        case 19:
            advance_scene = 0;
            CutsceneTextLoad("from outer space.", 220, 80);
            gCutsceneScriptCountdown = 59;
            break;
        case 20:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 21:
            advance_scene = 0;
            CutsceneTextLoad("Outer Space.", 78, 54);
            gCutsceneScriptCountdown = 42;
            break;
        case 22:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameOne, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 23:
            gCutsceneScriptCountdown = 8;
            CutsceneReleaseTextEntities();
            break;
        case 24:
            advance_scene = 0;
            CutsceneTextLoad("More specifically,", 220, 80);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 25:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 26:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSixteen, 6);
            gCutsceneScriptCountdown = 34;
            break;
        case 27:
            advance_scene = 0;
            CutsceneTextLoad("we are from", 220, 72);
            CutsceneLoadSecondTextLine("the Planet Funkotron.", 220, 84);
            gCutsceneScriptCountdown = 85;
            break;
        case 28:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 17;
            break;
        case 29:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 8;
            break;
        case 30:
            advance_scene = 0;
            CutsceneTextLoad("FUNKOTRON!", 78, 54);
            gCutsceneScriptCountdown = 17;
            break;
        case 31:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveAscendingFrames, 3);
            gCutsceneScriptCountdown = 34;
            break;
        case 32:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 33:
            advance_scene = 0;
            CutsceneTextLoad("Thank-you Earl.", 220, 80);
            gCutsceneScriptCountdown = 34;
            break;
        case 34:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameThree, 6);
            gCutsceneScriptCountdown = 34;
            gCutsceneMusicActorAnimationEnabled = 0;
            break;
        case 35:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveDescendingFrames, 6);
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 36:
            advance_scene = 0;
            CutsceneTextLoad("Check this out -", 220, 72);
            gCutsceneScriptCountdown = 34;
            break;
        case 37:
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 38:
            CutsceneReleaseTextEntities();
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            gCutsceneScriptCountdown = 17;
            break;
        case 39:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
        }
    }
    CutsceneReleaseTextEntities();
    IntroReleaseLargePlayerSpritesAndRefresh();
    IntroReleaseLargePlayerParts();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Enable background advancement during the final rocketship caption pause. */
short IntroPart2()
{
    short advance_period;

    CopyPaletteToCRAMSlot(PaletteShipLine3, 2);
    IntroCreateHiddenFrameEntities();
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 17;
    advance_period = 0;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        do {
            CutsceneStepFrameAndMusicCadence(advance_period, 0);
            if (gCutsceneAdvanceButtonMask = (GetCtrl1Input() | GetCtrl2Input()) & INPUT_ACTION_MASK)
                gCutsceneScriptCountdown = 0;
        } while (gCutsceneAdvanceButtonMask);
        if (--gCutsceneScriptCountdown > 0)
            continue;
        gCutsceneScriptCountdown = 85;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            CutsceneTextLoad("This is our highly funky,", 160, 30);
            IntroRevealFrameEntities();
            break;
        case 2:
            CutsceneTextLoad("Ultra Cool,", 160, 30);
            gCutsceneScriptCountdown = 51;
            break;
        case 3:
            CutsceneTextLoad("Righteous Rapmaster Rocketship.", 160, 30);
            gCutsceneScriptCountdown = 102;
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
            gCutsceneScriptCountdown = 102;
            break;
        case 8:
            CutsceneTextLoad("Crank it Earl!", 160, 30);
            gCutsceneScriptCountdown = 51;
            break;
        case 9:
            CutsceneReleaseTextEntities();
            gRequestedMusicSelection = MUSIC_TRANSITION;
            advance_period = 1;
            gCutsceneScriptCountdown = 306;
            break;
        case 10:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            advance_period = 0;
        }
    }
    CutsceneReleaseTextEntities();
    IntroReleaseFrameEntitiesAndRefresh();
    CutsceneStepFrameAndMusicCadence(0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Return to the two large characters for the problem announcement. */
short IntroPart3()
{
    short advance_scene;

    IntroBeginLargePlayerScene(7);
    gRequestedMusicSelection = 0;
    gCutsceneScriptCountdown = 17;
    gCutsceneMusicActorAnimationEnabled = 1;
    advance_scene = 0;
    gCutsceneScriptState = 0;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        do {
            CutsceneStepFrameAndMusicCadence(0, gCutsceneMusicActorAnimationEnabled);
            if (gCutsceneAdvanceButtonMask = (GetCtrl1Input() | GetCtrl2Input()) & INPUT_ACTION_MASK)
                advance_scene = 1;
        } while (gCutsceneAdvanceButtonMask);
        if (!advance_scene && --gCutsceneScriptCountdown > 0)
            continue;
        ++gCutsceneScriptState;
        gCutsceneMusicActorAnimationEnabled = 1;
        switch (gCutsceneScriptState) {
        case 1:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveFramePulse, 3);
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFifteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenAscendingFrames, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 2:
            IntroSetLargePlayerPartSequence(6, IntroLargePlayerPartSixHeldFrameFourteen, 6);
            IntroSetLargePlayerPartSequence(7, IntroLargePlayerPartSevenHeldFrameSeventeen, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 3:
            advance_scene = 0;
            CutsceneTextLoad("Recently, we ran into", 220, 72);
            CutsceneLoadSecondTextLine("a small problem", 220, 84);
            gCutsceneScriptCountdown = 85;
            break;
        case 4:
            IntroSetLargePlayerPartSequence(5, IntroLargePlayerPartFiveAscendingFrames, 3);
            gCutsceneScriptCountdown = 34;
            break;
        case 5:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameTwo, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 6:
            IntroSetLargePlayerPartSequence(1, IntroLargePlayerPartOneHeldFrameThree, 6);
            advance_scene = 0;
            CutsceneTextLoad("Problem.", 78, 54);
            gCutsceneScriptCountdown = 59;
            break;
        case 7:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 8:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    IntroReleaseLargePlayerSpritesAndRefresh();
    IntroReleaseLargePlayerParts();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Scroll the background while alternating the rocketship exhaust update. */
short IntroPart4()
{
    short exhaust_phase, scroll_position;

    CutsceneCreateDriftingEntities();
    CutsceneAllocateRocketshipPair();
    DrawStarfieldPlane();
    CutsceneDrawRandomizedScreenBorder(1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    CutsceneSetRocketshipPairPose(0, 280, 160);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptCountdown = 85;
    gCutsceneScriptState = 0;
    scroll_position = 0;
    exhaust_phase = 0;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        do {
            WaitForNextVBlank(1);
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position / 4;
            UploadQueuedSpriteGraphics(0, 0);
            SelectViewportSpriteChain(PLAYER_TOEJAM);
            WaitForNextVBlank(1);
            CutsceneUpdateDriftingEntities(scroll_position);
            if (exhaust_phase ^= 1)
                CutsceneMoveRocketshipPairLeft(exhaust_phase);
            UpdateAndQueueEntitySprites();
            ++scroll_position;
            if (gCutsceneAdvanceButtonMask = (GetCtrl1Input() | GetCtrl2Input()) & INPUT_ACTION_MASK)
                gCutsceneScriptCountdown = 0;
        } while (gCutsceneAdvanceButtonMask);
        if (--gCutsceneScriptCountdown > 0)
            continue;
        gCutsceneScriptCountdown = 85;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            CutsceneTextLoad("We were cruisin in our Rocket,", 160, 30);
            break;
        case 2:
            CutsceneTextLoad("Thumpin out a funky beat,", 160, 30);
            break;
        case 3:
            CutsceneTextLoad("When big Earl said", 160, 30);
            CutsceneLoadSecondTextLine("he'd like a shot", 160, 40);
            gCutsceneScriptCountdown = 119;
            break;
        case 4:
            CutsceneTextLoad("in the driver's seat . . .", 160, 30);
            break;
        case 5:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    CutsceneReleaseDriftingEntities();
    CutsceneReleaseRocketshipPair();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Change Earl's driving poses and move the background in either direction. */
short IntroPart5()
{
    short scroll_direction, driving_pose, scroll_position;

    gSceneBackgroundPairIndex = 8;
    InitializeTitleTransitionTilemap();
    IntroAllocateDrivingParts();
    IntroAllocateFlyingEntitySlots();
    CutsceneDrawRandomizedRectangleBorder(11, 7, 30, 20);
    CutsceneDrawRandomizedRectangleBorder(12, 8, 29, 19);
    IntroPositionDrivingPart(0, 104, 151);
    IntroPositionDrivingPart(1, 104, 151);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteIntroEarlDriving_bank3, 3);
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 42;
    driving_pose = 1;
    scroll_direction = 0;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        IntroUpdateFlyingEntityMotion(driving_pose);
        if (scroll_direction) {
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position;
            scroll_position += scroll_direction << 2;
        }
        if (--gCutsceneScriptCountdown != 0)
            continue;
        gCutsceneScriptCountdown = 42;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[2].frame, 6);
            IntroPositionDrivingPart(2, 104, 151);
            gCutsceneScriptCountdown = 17;
            break;
        case 2:
            IntroSetDrivingPartSequence(2, IntroDrivingActorTwoLoopAnimation, 3);
            gCutsceneScriptCountdown = 51;
            break;
        case 3:
            IntroSetDrivingPartSequence(2, &IntroHeldFrameThree.frame, 6);
            gCutsceneScriptCountdown = 17;
            break;
        case 4:
            IntroReleaseDrivingPartSprites(2);
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[1].frame, 6);
            break;
        case 5:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[5].frame, 6);
            PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            driving_pose = 2;
            scroll_direction = -1;
            break;
        case 6:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 7:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[6].frame, 6);
            PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            driving_pose = 3;
            scroll_direction = 1;
            break;
        case 8:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 9:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[5].frame, 6);
            PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            driving_pose = 2;
            scroll_direction = -1;
            break;
        case 10:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameStreams[1].frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 11:
            IntroUpdateFlyingEntityMotion(0);
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    IntroReleaseDrivingParts();
    IntroReleaseFlyingEntitySlots();
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Step through the ship's dodging poses while the background scrolls. */
short IntroPart6()
{
    short pose_changed, driving_pose, scroll_position;

    CutsceneAllocateRocketshipPair();
    IntroAllocateFlyingEntitySlots();
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 42;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position / 4;
    gSceneBackgroundPairIndex = 9;
    AnimateTitleTransitionBackground(1);
    CutsceneSetRocketshipPairPose(0, 104, 151);
    pose_changed = 0;
    driving_pose = 0;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        IntroUpdateRightwardFlyingEntity(pose_changed, driving_pose);
        pose_changed = 0;
        CutsceneStepFrameAndMusicCadence(0, 0);
        VDP_COMMAND = 0x7c020003;
        VDP_DATA = scroll_position / 4;
        ++scroll_position;
        if (--gCutsceneScriptCountdown != 0)
            continue;
        gCutsceneScriptCountdown = 4;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            CutsceneSetRocketshipPairPose(1, 104, 151);
            break;
        case 2:
            CutsceneSetRocketshipPairPose(2, 104, 151);
            PlaySound(PSG_TONE_RISE, 5);
            break;
        case 3:
            CutsceneSetRocketshipPairPose(3, 104, 151);
            gCutsceneScriptCountdown = 51;
            pose_changed = 1;
            driving_pose = 1;
            break;
        case 4:
            CutsceneSetRocketshipPairPose(2, 104, 151);
            break;
        case 5:
            CutsceneSetRocketshipPairPose(1, 104, 151);
            pose_changed = 1;
            driving_pose = 2;
            PlaySound(PSG_LONG_TONE_RISE, 5);
            break;
        case 6:
            CutsceneSetRocketshipPairPose(0, 104, 151);
            gCutsceneScriptCountdown = 51;
            break;
        case 7:
            CutsceneSetRocketshipPairPose(1, 104, 151);
            break;
        case 8:
            CutsceneSetRocketshipPairPose(2, 104, 151);
            pose_changed = 1;
            driving_pose = 1;
            PlaySound(PSG_TONE_RISE, 5);
            break;
        case 9:
            CutsceneSetRocketshipPairPose(3, 104, 151);
            gCutsceneScriptCountdown = 51;
            break;
        case 10:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    CutsceneReleaseRocketshipPair();
    IntroReleaseFlyingEntitySlots();
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position;
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Alternate camera shake during the final driving poses and asteroid entry. */
short IntroPart7()
{
    short shake_phase, scroll_direction, driving_pose, scroll_position;

    IntroAllocateDrivingParts();
    IntroAllocateFlyingEntitySlots();
    CopyPaletteToCRAMSlot(PaletteIntroEarlDriving_bank0, 0);
    shake_phase = 0;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 34;
    driving_pose = 1;
    scroll_position = 0;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = scroll_position / 4;
    scroll_direction = 0;
    gSceneBackgroundPairIndex = 10;
    AnimateTitleTransitionBackground(1);
    IntroPositionDrivingPart(0, 104, 151);
    IntroPositionDrivingPart(1, 104, 151);
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        AnimateTitleTransitionBackground(0);
        if (shake_phase) {
            if (shake_phase == 1) {
                gCamera1PendingX += 5;
                gCamera1PendingY += 5;
                shake_phase = 2;
            } else if (shake_phase == 2) {
                gCamera1PendingX -= 5;
                gCamera1PendingY -= 5;
                shake_phase = 1;
            }
        }
        gSpriteGraphicsUploadPending = 0;
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(1);
        if (shake_phase) {
            VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
            VDP_DATA = gCamera1PendingY;
            VDP_COMMAND = 0x7c000003;
            VDP_DATA = -gCamera1PendingX;
        }
        UploadQueuedSpriteGraphics(0, 0);
        SelectViewportSpriteChain(PLAYER_TOEJAM);
        IntroUpdateFlyingEntityMotion(driving_pose);
        if (scroll_direction) {
            VDP_COMMAND = 0x7c020003;
            VDP_DATA = scroll_position / 4;
            scroll_position += scroll_direction << 3;
        }
        if (--gCutsceneScriptCountdown != 0)
            continue;
        gCutsceneScriptCountdown = 34;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameFive.frame, 6);
            PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            scroll_direction = -1;
            driving_pose = 2;
            break;
        case 2:
            break;
        case 3:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameOne.frame, 6);
            driving_pose = 1;
            scroll_direction = 0;
            break;
        case 4:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameSix.frame, 6);
            PlaySoundEffect((long)Sfx_Running, AUDIO_PRIORITY_NORMAL, 0);
            scroll_direction = 1;
            driving_pose = 3;
            break;
        case 5:
            IntroSetDrivingPartSequence(1, &IntroHeldFrameOne.frame, 6);
            driving_pose = 4;
            scroll_direction = 0;
            break;
        case 6:
            shake_phase = 1;
            IntroCreateAsteroidEntity();
            PlaySound(PSG_NOISE_FADE_RATE0, 5);
            break;
        case 7:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            shake_phase = 0;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    IntroReleaseAsteroidEntity();
    IntroReleaseFlyingEntitySlots();
    IntroReleaseDrivingParts();
    gCamera1PendingX = 0;
    gCamera1PendingY = 0;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c000003;
    VDP_DATA = gCamera1PendingX;
    VDP_COMMAND = 0x7c020003;
    VDP_DATA = gCamera1PendingX;
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Load the crash screen, then show and release the collision objects. */
short IntroPart8()
{
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 4;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteIntroCollision_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    VDP_COMMAND = 0x46600001;
    MoveDataToVRAM(CrashScreen, CrashScreenSize << 4);
    gSharedTileSpriteCopiesEnabled = 1;
    ClearUpperVramTilemap();
    ClearLowerVramTilemap();
    JamOutDrawPackedBackgroundBlock(0, 0);
    JamOutDrawPackedBackgroundBlock(20, 0);
    JamOutDrawPackedBackgroundBlock(0, 14);
    JamOutDrawPackedBackgroundBlock(20, 14);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        if (--gCutsceneScriptCountdown == 0) {
            ++gCutsceneScriptState;
            switch (gCutsceneScriptState) {
            case 1:
                IntroCreateCollisionSceneEntities();
                PlaySoundEffect((long)Sfx_Falling, AUDIO_PRIORITY_NORMAL, 0);
                gCutsceneScriptCountdown = 59;
                break;
            case 2:
                IntroReleaseCollisionSceneEntities();
                gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
                break;
            }
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneStepFrameAndMusicCadence(0, 0);
    gSharedTileSpriteCopiesEnabled = 0;
    DrawStarfieldPlane();
    InitializeTitleTransitionTilemap();
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Scroll the second background plane through two timed scene states. */
short IntroPart9()
{
    short scroll_position;

    gSceneBackgroundPairIndex = 12;
    AnimateTitleTransitionBackground(1);
    CutsceneDrawRandomizedRectangleBorder(11, 7, 30, 20);
    CutsceneDrawRandomizedRectangleBorder(12, 8, 29, 19);
    IntroCreateFallingRocketship();
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    gCutsceneScriptState = 0;
    scroll_position = 0;
    gCutsceneScriptCountdown = 42;
    IntroPositionFallingRocketship(170, 141);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        VDP_COMMAND = 0x40020010;
        VDP_DATA = scroll_position;
        ++scroll_position;
        if (--gCutsceneScriptCountdown != 0)
            continue;
        gCutsceneScriptCountdown = 85;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            break;
        case 2:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    IntroReleaseFallingRocketship();
    scroll_position = 0;
    VDP_COMMAND = 0x40020010;
    VDP_DATA = scroll_position;
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}

/* Increase a scene Y argument from 20 to 160, then show the driving warning. */
short IntroPart10()
{
    short scene_y;

    CutsceneDrawRandomizedScreenBorder(1);
    IntroCreateFallingRocketship();
    gCutsceneScriptCountdown = 42;
    scene_y = 20;
    CutsceneCreatePlanetEntity(160, 180);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptState = 1;
    while (gCutsceneScriptState != CUTSCENE_SCRIPT_DONE &&
        !((GetCtrl1Input() | GetCtrl2Input()) & INPUT_START)) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        switch (gCutsceneScriptState) {
        case 1:
            ++scene_y;
            IntroPositionFallingRocketship(160, scene_y);
            if (scene_y == 160) {
                ++gCutsceneScriptState;
                IntroReleaseFallingRocketship();
            }
            break;
        case 2:
            IntroCreateRocketshipImpact(160, 180);
            PlaySound(PSG_NOISE_RISE, AUDIO_PRIORITY_NORMAL);
            gCutsceneScriptCountdown = 85;
            ++gCutsceneScriptState;
            break;
        case 3:
            if (--gCutsceneScriptCountdown == 0)
                ++gCutsceneScriptState;
            break;
        case 4:
            IntroReleaseRocketshipImpact();
            CutsceneTextLoad("you should never let earl drive", 160, 30);
            gCutsceneScriptCountdown = 119;
            ++gCutsceneScriptState;
            break;
        case 5:
            if (--gCutsceneScriptCountdown == 0)
                ++gCutsceneScriptState;
            break;
        case 6:
            gCutsceneScriptState = CUTSCENE_SCRIPT_DONE;
            break;
        }
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    CutsceneReleasePlanetEntity();
    IntroReleaseFallingRocketship();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return gCutsceneScriptState != CUTSCENE_SCRIPT_DONE;
}
