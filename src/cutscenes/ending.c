#include "music_ids.h"
#define TJE_LOCAL_ENDINGGETFIRSTPLAYERANIMATIONFRAME

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
extern int EndingGetFirstPlayerAnimationFrame();

/* Reset both camera scrolls, load the text and level tiles, and play the ending. */
void PlayEndingSequence()
{
    gRequestedMusicSelection = 0;
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    DisableBlankingGraphics();
    gCamera1PendingX = 0;
    gCamera1PendingY = 0;
    VDP_CONTROL_WORD = 0x8228;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = gCamera1PendingX;
    VDP_COMMAND = 0x40020010L;
    VDP_DATA = gCamera1PendingY;
    VDP_COMMAND = 0x7c020003L;
    VDP_DATA = gCamera1PendingX;
    ClearUpperVramTilemap();
    gCameraTileAttributes = 0x8000;
    VDP_COMMAND = 0x70000002L;
    MoveDataToVRAM(ImageTextBase, FontDataSize << 4);
    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank3, 3);
    DrawStarfieldPlane();
    EndingPlayDialogueAndDeparture();
    EndingPlayUpwardRocketshipDeparture();
    EndingPlayScrollingCredits();
    EndingPlayDescendingRocketshipScene();
    EndingPlayBouncingRocketshipScene();
    CutsceneReleaseTextEntities();
    ClearLowerVramTilemap();
    CutsceneReleaseFlaggedEntityRange();
}

/* Advance the celebration dialogue and the accelerating exit animation. */
short EndingPlayDialogueAndDeparture()
{
    short character_x, character_y, mouth_timer, speed, clap_played;

    CutsceneAllocateRocketshipPair();
    EndingCreatePoseComposite();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ClearLowerVramTilemap();
    EndingDrawNineMetatileBackground();
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = -30;
    character_y = 127;
    character_x = 92;
    speed = 0;
    clap_played = 0;
    CutsceneSetForegroundRocketshipPose(3, character_x, character_y);
    EndingSetPoseComposite(0, character_x, 127);
    EndingCreateActivePlayerEntities(character_x, 127);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    mouth_timer = 0;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 34;
    while (gCutsceneScriptState != -1) {
        WaitForNextVBlank(1);
        UploadQueuedSpriteGraphics(0, 0);
        SelectViewportSpriteChain(0);
        WaitForNextVBlank(1);
        UpdateAndQueueEntitySprites();
        switch (gCutsceneScriptState) {
        case 0:
            if (!--gCutsceneScriptCountdown) {
                if (gPlayerSelection == 2)
                    CutsceneLoadOriginalPaletteTextLine("we did it", 90, 95);
                else
                    CutsceneLoadOriginalPaletteTextLine("i did it", 90, 95);
                gCutsceneScriptCountdown = 51;
                ++gCutsceneScriptState;
            }
            break;
        case 1:
            if (!--gCutsceneScriptCountdown) {
                CutsceneReleaseTextEntities();
                EndingCreateElevatedThreePieceComposite(character_x, 127);
                gCutsceneScriptCountdown = 323;
                ++gCutsceneScriptState;
            }
            break;
        case 2:
            if (gPlayerSelection == 2 && !clap_played &&
                28 == EndingGetFirstPlayerAnimationFrame()) {
                clap_played = 1;
                PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            }
            if (!--gCutsceneScriptCountdown) {
                ++gCutsceneScriptState;
                EndingReleaseActivePlayerEntities();
                EndingRestartElevatedCompositeSequence();
                gCutsceneScriptCountdown = 34;
            }
            break;
        case 3:
            if (!--gCutsceneScriptCountdown) {
                ++gCutsceneScriptState;
                gCutsceneScriptCountdown = 51;
            }
            break;
        case 4:
            if (!--gCutsceneScriptCountdown)
                ++gCutsceneScriptState;
            break;
        case 5:
            if (--character_y < 107) {
                gCutsceneScriptCountdown = 17;
                ++gCutsceneScriptState;
            }
            if (character_y == 112)
                EndingSetPoseComposite(1, character_x, 127);
            CutscenePositionRocketshipPair(character_x, character_y);
            break;
        case 6:
            if (!--gCutsceneScriptCountdown)
                ++gCutsceneScriptState;
            break;
        case 7:
            if (gPlayerSelection == 2)
                CutsceneLoadOriginalPaletteTextLine("crank it earl!", 160, 30);
            else
                CutsceneLoadOriginalPaletteTextLine(" crank it!", 160, 30);
            gCutsceneScriptCountdown = 51;
            ++gCutsceneScriptState;
        case 8:
            if (!--gCutsceneScriptCountdown)
                ++gCutsceneScriptState;
            break;
        case 9:
            EndingReleaseElevatedThreePieceComposite();
            CutsceneReleaseTextEntities();
            EndingCreateSixPieceComposite();
            EndingPositionSixPieceComposite(character_x, -50);
            gRequestedMusicSelection = MUSIC_TRANSITION;
            gCutsceneScriptCountdown = 221;
            ++gCutsceneScriptState;
            break;
        case 10:
            if (gMusicAnimationEvent) {
                EndingPositionSixPieceComposite(character_x, character_y);
                mouth_timer = 4;
                gMusicAnimationEvent = 0;
            }
            if (!--mouth_timer)
                EndingPositionSixPieceComposite(character_x, -50);
            if (!--gCutsceneScriptCountdown) {
                ++gCutsceneScriptState;
                PlaySoundEffect((long)Sfx_Running, 50, 0);
                EndingReleaseSixPieceComposite();
            }
            break;
        case 11:
            if ((character_x -= speed) < -30)
                ++gCutsceneScriptState;
            ++speed;
            CutscenePositionRocketshipPair(character_x, character_y);
            EndingSetPoseComposite(1, character_x, 127);
            break;
        case 12:
            gCutsceneScriptState = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    EndingReleaseElevatedThreePieceComposite();
    EndingReleaseActivePlayerEntities();
    CutsceneReleaseRocketshipPair();
    EndingReleasePoseComposite();
    EndingReleaseSixPieceComposite();
    CutsceneReleaseTextEntities();
    CutsceneStepFrameAndMusicCadence(0, 0);
    ClearLowerVramTilemap();
    speed = 0;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = speed;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    return 0;
}

/* Move the character upward and left while displaying the driving exchange. */
short EndingPlayUpwardRocketshipDeparture()
{
    short character_x, character_y, move_timer, move_period;

    CutsceneDrawRandomizedScreenBorder(1);
    CutsceneAllocateRocketshipPair();
    gRequestedMusicSelection = 0;
    character_y = 195;
    character_x = 112;
    move_timer = 15;
    move_period = move_timer;
    CutsceneSetRocketshipPairPose(0, character_x, character_y);
    CutsceneCreatePlanetEntity(160, 180);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 17;
    while (gCutsceneScriptState != -1) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        switch (gCutsceneScriptState) {
        case 0:
            if (!--gCutsceneScriptCountdown)
                ++gCutsceneScriptState;
            break;
        case 1:
            if (move_timer & 1)
                --character_y;
            if (!--move_timer) {
                if (!(move_period >>= 1))
                    move_period = 1;
                move_timer = move_period;
                --character_x;
                if (character_x == 107 && gPlayerSelection == 2)
                    CutsceneTextLoad("Can I drive?", character_x + 70,
                                     character_y - 85);
            }
            CutscenePositionRocketshipPair(character_x, character_y);
            if (character_y <= 132)
                ++gCutsceneScriptState;
            break;
        case 2:
            --character_x;
            CutscenePositionRocketshipPair(character_x, character_y);
            if (character_x == 57 && gPlayerSelection == 2)
                CutsceneTextLoad("No!", character_x + 35, character_y - 80);
            if (character_x <= -30)
                ++gCutsceneScriptState;
            break;
        case 3:
            gCutsceneScriptState = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    CutsceneReleasePlanetEntity();
    CutsceneReleaseRocketshipPair();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return 0;
}

/* Scroll the background while alternating credit pages and blank intervals. */
short EndingPlayScrollingCredits()
{
    short scroll, animation_phase;

    CutsceneCreateDriftingEntities();
    CutsceneAllocateRocketshipPair();
    DrawStarfieldPlane();
    CutsceneDrawRandomizedScreenBorder(1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank3, 3);
    CutsceneSetRocketshipPairPose(0, 280, 160);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptCountdown = 85;
    gCutsceneScriptState = 0;
    scroll = 0;
    animation_phase = 0;
    while (gCutsceneScriptState != -1) {
        WaitForNextVBlank(1);
        VDP_COMMAND = 0x7c020003L;
        VDP_DATA = scroll / 4;
        UploadQueuedSpriteGraphics(0, 0);
        SelectViewportSpriteChain(0);
        WaitForNextVBlank(1);
        CutsceneUpdateDriftingEntities(scroll);
        if (!(animation_phase = (animation_phase + 1) % 3))
            CutsceneMoveRocketshipPairLeft(1);
        UpdateAndQueueEntitySprites();
        ++scroll;
        if (--gCutsceneScriptCountdown)
            continue;
        gCutsceneScriptCountdown = 102;
        ++gCutsceneScriptState;
        switch (gCutsceneScriptState) {
        case 1:
            CutsceneTextLoad("game design -- greg johnson", 160, 35);
            CutsceneLoadSecondTextLine("game program -- mark voorsanger", 160, 48);
            gCutsceneScriptCountdown = 136;
            break;
        case 2:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 3:
            CutsceneTextLoad("additional programming         ", 160, 35);
            CutsceneLoadSecondTextLine("             -- robert leyland", 160, 45);
            break;
        case 4:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 5:
            CutsceneTextLoad("music direction -- mark miller", 160, 35);
            CutsceneLoadSecondTextLine("music composition -- john baker", 160, 48);
            break;
        case 6:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 7:
            CutsceneTextLoad("artwork -- greg johnson", 160, 35);
            CutsceneLoadSecondTextLine("           avril harrison", 160, 45);
            break;
        case 8:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 9:
            CutsceneTextLoad("sound fx -- robert leyland", 160, 35);
            CutsceneLoadSecondTextLine("            mark miller ", 160, 45);
            break;
        case 10:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 11:
            CutsceneTextLoad("invaluable aid -- paul reiche", 160, 35);
            CutsceneLoadSecondTextLine("                  fred ford", 160, 45);
            break;
        case 12:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 13:
            gCutsceneScriptCountdown = 8;
            break;
        case 14:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 15:
            CutsceneTextLoad("producer -- scott berfield", 160, 35);
            CutsceneLoadSecondTextLine("awesome support -- hugh bowen", 160, 48);
            break;
        case 16:
            CutsceneReleaseTextEntities();
            gCutsceneScriptCountdown = 8;
            break;
        case 17:
            CutsceneTextLoad("a johnson-voorsanger production", 160, 35);
            gCutsceneScriptCountdown = 136;
            break;
        case 18:
            gCutsceneScriptState = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseTextEntities();
    CutsceneReleaseDriftingEntities();
    CutsceneReleaseRocketshipPair();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return 0;
}

/* Move the character left, then curve downward out of the scene. */
short EndingPlayDescendingRocketshipScene()
{
    short character_x, character_y, move_period, move_timer;

    CutsceneDrawRandomizedScreenBorder(1);
    CutsceneAllocateRocketshipPair();
    gCutsceneScriptCountdown = 42;
    character_y = 132;
    character_x = 280;
    move_timer = 20;
    move_period = move_timer;
    CutsceneSetRocketshipPairPose(0, character_x, character_y);
    CopyPaletteToCRAMSlot(EndingPaletteA, 3);
    EndingCreateStationaryComposite(170, 180);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptState = 0;
    while (gCutsceneScriptState != -1) {
        CutsceneStepFrameAndMusicCadence(0, 0);
        switch (gCutsceneScriptState) {
        case 0:
            if (--character_x == 182)
                ++gCutsceneScriptState;
            CutscenePositionRocketshipPair(character_x, character_y);
            break;
        case 1:
            --character_x;
            if (!--move_timer) {
                if (!(move_period >>= 1))
                    move_period = 1;
                move_timer = move_period;
                ++character_y;
            }
            CutscenePositionRocketshipPair(character_x, character_y);
            if (character_x == 122)
                ++gCutsceneScriptState;
            break;
        case 2:
            if (character_y < 185)
                ++character_y;
            else if (++move_period & 1)
                ++character_y;
            CutscenePositionRocketshipPair(character_x, character_y);
            if (character_y >= 195)
                ++gCutsceneScriptState;
            break;
        case 3:
            gCutsceneScriptState = -1;
            break;
        }
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    EndingReleaseStationaryComposite();
    CutsceneReleaseRocketshipPair();
    CutsceneStepFrameAndMusicCadence(0, 0);
    return 0;
}

/* Scroll the plane and move the character vertically while swapping companions. */
short EndingPlayBouncingRocketshipScene()
{
    short character_y, character_x, velocity_y, acceleration_y, scroll;
    short movement_frame;

    CutsceneAllocateRocketshipPair();
    EndingCreatePoseComposite();
    EndingCreateRightOffsetComposite();
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
    CopyPaletteToCRAMSlot(EndingPaletteB, 3);
    EndingFillRepeatedMetatileBackground();
    character_y = 72;
    character_x = 152;
    scroll = 0;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = scroll;
    velocity_y = 0;
    acceleration_y = 1;
    CutsceneSetForegroundRocketshipPose(3, character_x, character_y);
    EndingSetPoseComposite(1, character_x, 127);
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gCutsceneScriptState = 0;
    gCutsceneScriptCountdown = 17;
    while (gCutsceneScriptState != -1) {
        gSpriteGraphicsUploadPending = 0;
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(1);
        UploadQueuedSpriteGraphics(0, 0);
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = scroll;
        SelectViewportSpriteChain(0);
        scroll += 5;
        if (++gCutsceneScriptState & 1)
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
            CutscenePositionRocketshipPair(character_x, 127);
            EndingPositionRightOffsetComposite(character_x, 127);
            EndingSetPoseComposite(0, character_x, 127);
        } else {
            CutscenePositionRocketshipPair(character_x, character_y);
            EndingPositionRightOffsetComposite(character_x, -50);
            if (character_y < 107)
                EndingSetPoseComposite(1, character_x, 127);
            else
                EndingSetPoseComposite(0, character_x, 127);
        }
        if (character_x < -30)
            gCutsceneScriptState = -1;
    }
    gMode2RegSoftcopy &= 60;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneReleaseRocketshipPair();
    EndingReleasePoseComposite();
    EndingReleaseRightOffsetComposite();
    CutsceneStepFrameAndMusicCadence(0, 0);
    ClearLowerVramTilemap();
    scroll = 0;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = scroll;
    return 0;
}
