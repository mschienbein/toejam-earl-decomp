#include "fm_effects.h"
#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_intro.h"
#include "assets_jam.h"
#include "assets_palettes.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "cutscene_text_calls.h"
#include "hardware_calls.h"
#include "jam_calls.h"
#include "ram_camera.h"
#include "ram_jam.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "random.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Run the jam scene until Start has been held for 51 consecutive updates. */
void StartJamOut(song)
short song;
{
    short start_hold;

    while (GetCtrl1Input() | GetCtrl2Input())
        WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    PlaySoundEffect((long)Sfx_Jammin, 90, 0);
    gJamSelectedSong = song;
    gVBlankFunc = JamOutVBlank;
    gMusicAlternateTemplateBankEnabled = 1;
    VDP_COMMAND = 0x46600001L;
    MoveDataToVRAM(CrashScreen, CrashScreenSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    gSharedTileSpriteCopiesEnabled = 1;
    JamOutCreateCharacterCopies();
    VDP_CONTROL_WORD = 0x8c89;
    gCameraTileAttributes = 0;
    JamOutUpdateLayoutsAndScroll(1);
    start_hold = 0;
    if (song == 7) {
        gJamAudioRoute1ButtonASampleIndex = (int)GetRandomNumber() % 42;
        gJamAudioRoute1ButtonBSampleIndex = (int)GetRandomNumber() % 42;
        gJamAudioRoute1ButtonCSampleIndex = (int)GetRandomNumber() % 42;
        gJamAudioRoute2ButtonASampleIndex = (int)GetRandomNumber() % 42;
        gJamAudioRoute2ButtonBSampleIndex = (int)GetRandomNumber() % 42;
        gJamAudioRoute2ButtonCSampleIndex = (int)GetRandomNumber() % 42;
        gVBlankFunc = JamOutRandomSampleVBlank;
    }
    if (song) {
        --song;
        WaitForNextVBlank(1);
        gCurrentMusicSelection = !song;
        gRequestedMusicSelection = song;
    } else {
        AudioRequestFmChannelRelease();
        MusicRequestTrackPointerReset();
    }
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    CutsceneTextLoad("press and hold start to exit", 160, 30);
    while (start_hold < 51) {
        CutsceneStepFrameAndMusicCadence(3, 0);
        JamOutUpdateLayoutsAndScroll(0);
        JamOutUpdateLeadAnimations();
        if ((GetCtrl1Input() | GetCtrl2Input()) & 128)
            ++start_hold;
        else
            start_hold = 0;
        if (gJamPlayer1PendingAnimationInput) {
            if (gJamPlayer1PendingAnimationInput & INPUT_BUTTON_A) {
                if ((int)GetRandomNumber() & 1)
                    JamOutSetToejamSequence(JamOutCharacterAnimations, 6);
                else
                    JamOutSetToejamSequence(JamBassDrumAlternateSequence, 6);
            }
            if (gJamPlayer1PendingAnimationInput & INPUT_BUTTON_B) {
                if ((int)GetRandomNumber() & 1)
                    JamOutSetToejamSequence(JamSnareDrumSecondSequence, 6);
                else
                    JamOutSetToejamSequence(JamSnareDrumFirstSequence, 6);
            }
            if (gJamPlayer1PendingAnimationInput & INPUT_BUTTON_C) {
                JamOutSetToejamSequence(JamClapSequence, 3);
                gJamToejamAnimationLocked = 1;
            }
            if ((gJamPlayer1PendingAnimationInput & INPUT_DIRECTION_MASK) || (gJamPlayer1PendingAnimationInput & INPUT_START)) {
                JamOutSetToejamSequence(JamVocalOrScratchSequence, 2);
                gJamToejamAnimationLocked = 1;
            }
            gJamPlayer1PendingAnimationInput = 0;
        }
        if (gJamPlayer2PendingAnimationInput) {
            if (gJamPlayer2PendingAnimationInput & INPUT_BUTTON_A) {
                if ((int)GetRandomNumber() & 1)
                    JamOutSetEarlSequence(JamOutCharacterAnimations, 6);
                else
                    JamOutSetEarlSequence(JamBassDrumAlternateSequence, 6);
            }
            if (gJamPlayer2PendingAnimationInput & INPUT_BUTTON_B) {
                if ((int)GetRandomNumber() & 1)
                    JamOutSetEarlSequence(JamSnareDrumSecondSequence, 6);
                else
                    JamOutSetEarlSequence(JamSnareDrumFirstSequence, 6);
            }
            if (gJamPlayer2PendingAnimationInput & INPUT_BUTTON_C) {
                JamOutSetEarlSequence(JamClapSequence, 3);
                gJamEarlAnimationLocked = 1;
            }
            if ((gJamPlayer2PendingAnimationInput & INPUT_DIRECTION_MASK) || (gJamPlayer2PendingAnimationInput & INPUT_START)) {
                JamOutSetEarlSequence(JamVocalOrScratchSequence, 2);
                gJamEarlAnimationLocked = 1;
            }
            gJamPlayer2PendingAnimationInput = 0;
        }
    }
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    gVBlankFunc = 0;
    gMusicAlternateTemplateBankEnabled = 0;
    CutsceneReleaseTextEntities();
    ClearUpperVramTilemap();
    JamOutUpdateLayoutsAndScroll(1);
    ClearLowerVramTilemap();
    WaitForNextVBlank(1);
    gCurrentMusicSelection = 1;
    gRequestedMusicSelection = 0;
    JamOutReleaseCharacterCopies();
    gSharedTileSpriteCopiesEnabled = 0;
    CutsceneReleaseFlaggedEntityRange();
}

/* Alternate controller polling and route each player's new input to samples
 * or channel-template effects. gPlayerSelection 1 swaps the routed words. */
void JamOutVBlank()
{
    unsigned short saved_input;
    if (gCurrentMusicSelection != gRequestedMusicSelection) {
        MusicSelectSongTemplateBank(gRequestedMusicSelection);
        gCurrentMusicSelection = gRequestedMusicSelection;
    } else
        MusicUpdateSequencer();
    ++gFrameCounter;
    if (gFrameCounterLowByte & 1) {
        gPlayerProcessedInput[PLAYER_TOEJAM] = gControllerPortsSwapped ? GetCtrl2Input() : GetCtrl1Input();
        if (gPlayerProcessedInput[PLAYER_TOEJAM] != gJamPlayer1PreviousInput) {
            gJamAudioRoute1PendingInput = gJamPlayer1PendingAnimationInput = gPlayerProcessedInput[PLAYER_TOEJAM];
            gJamPlayer1PreviousInput = gPlayerProcessedInput[PLAYER_TOEJAM];
        }
    } else {
        gPlayerProcessedInput[PLAYER_EARL] = gControllerPortsSwapped ? GetCtrl1Input() : GetCtrl2Input();
        if (gPlayerProcessedInput[PLAYER_EARL] != gJamPlayer2PreviousInput) {
            gJamAudioRoute2PendingInput = gJamPlayer2PendingAnimationInput = gPlayerProcessedInput[PLAYER_EARL];
            gJamPlayer2PreviousInput = gPlayerProcessedInput[PLAYER_EARL];
        }
    }
    if (gPlayerSelection == 1) {
        saved_input = gJamAudioRoute1PendingInput;
        gJamAudioRoute1PendingInput = gJamAudioRoute2PendingInput;
        gJamAudioRoute2PendingInput = saved_input;
        ++gFrameCounter;
    }
    if (gFrameCounterLowByte & 1) {
        if (gJamAudioRoute1PendingInput) {
            if (gJamAudioRoute1PendingInput & INPUT_BUTTON_A) {
                if (!gJamSelectedSong)
                    gMusicAnimationEvent = 1;
                PlaySoundEffect((long)Sfx_JamBassDrum, 90, 0);
            }
            if (gJamAudioRoute1PendingInput & INPUT_BUTTON_B)
                PlaySoundEffect((long)Sfx_JamSnareDrum, 90, 0);
            if (gJamAudioRoute1PendingInput & INPUT_BUTTON_C)
                PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            if (gJamAudioRoute1PendingInput & INPUT_DIRECTION_MASK) {
                if (gJamAudioRoute1PendingInput & INPUT_UP)
                    PlaySoundEffect((long)Sfx_JamToejam, 90, 0);
                if (gJamAudioRoute1PendingInput & INPUT_DOWN)
                    PlaySoundEffect((long)Sfx_JamBigEarl, 90, 0);
                if (gJamAudioRoute1PendingInput & INPUT_LEFT)
                    PlaySoundEffect((long)Sfx_JamRap, 90, 0);
                if (gJamAudioRoute1PendingInput & INPUT_RIGHT)
                    PlaySoundEffect((long)Sfx_Jammin, 90, 0);
            }
            if (gJamAudioRoute1PendingInput & INPUT_START)
                PlaySoundEffect((long)Sfx_JamRecordScratch, 90, 0);
            gJamAudioRoute1PendingInput = 0;
        }
    } else {
        if (gJamAudioRoute2PendingInput) {
            if (gJamAudioRoute2PendingInput & INPUT_BUTTON_A) {
                AudioRequestChannelTemplateEffect(FM_EFFECT_JAM_BUTTON_A);
                if (!gJamSelectedSong)
                    gMusicAnimationEvent = 1;
            }
            if (gJamAudioRoute2PendingInput & INPUT_BUTTON_B)
                AudioRequestChannelTemplateEffect(FM_EFFECT_JAM_BUTTON_B);
            if (gJamAudioRoute2PendingInput & INPUT_BUTTON_C)
                AudioRequestChannelTemplateEffect(FM_EFFECT_JAM_BUTTON_C);
            if ((gJamAudioRoute2PendingInput & INPUT_DIRECTION_MASK) || (gJamAudioRoute2PendingInput & INPUT_START))
                AudioRequestChannelTemplateEffect(FM_EFFECT_JAM_DIRECTION);
            gJamAudioRoute2PendingInput = 0;
        }
    }
    if (gPlayerSelection == 1)
        --gFrameCounter;
}

/* Poll both controllers, select sample groups, and cycle song/template modes. */
void JamOutRandomSampleVBlank()
{
    unsigned short saved_input;
    if (gCurrentMusicSelection != gRequestedMusicSelection) {
        MusicSelectSongTemplateBank(gRequestedMusicSelection);
        gCurrentMusicSelection = gRequestedMusicSelection;
    } else
        MusicUpdateSequencer();
    ++gFrameCounter;
    gPlayerProcessedInput[PLAYER_TOEJAM] = gControllerPortsSwapped ? GetCtrl2Input() : GetCtrl1Input();
    if (gPlayerProcessedInput[PLAYER_TOEJAM] != gJamPlayer1PreviousInput) {
        gJamAudioRoute1PendingInput = gJamPlayer1PendingAnimationInput = gPlayerProcessedInput[PLAYER_TOEJAM];
        gJamPlayer1PreviousInput = gPlayerProcessedInput[PLAYER_TOEJAM];
    }
    gPlayerProcessedInput[PLAYER_EARL] = gControllerPortsSwapped ? GetCtrl1Input() : GetCtrl2Input();
    if (gPlayerProcessedInput[PLAYER_EARL] != gJamPlayer2PreviousInput) {
        gJamAudioRoute2PendingInput = gJamPlayer2PendingAnimationInput = gPlayerProcessedInput[PLAYER_EARL];
        gJamPlayer2PreviousInput = gPlayerProcessedInput[PLAYER_EARL];
    }
    if (gPlayerSelection == 1) {
        saved_input = gJamAudioRoute1PendingInput;
        gJamAudioRoute1PendingInput = gJamAudioRoute2PendingInput;
        gJamAudioRoute2PendingInput = saved_input;
        ++gFrameCounter;
    }
    if (gJamAudioRoute1PendingInput) {
        if (gJamAudioRoute1PendingInput & INPUT_START) {
            if (gJamSongCycleIndex == 7) {
                AudioRequestFmChannelRelease();
                MusicRequestTrackPointerReset();
                gJamSongCycleIndex = 0;
            } else
                gRequestedMusicSelection = gJamSongCycleIndex++;
        }
        if (gJamAudioRoute1PendingInput & INPUT_BUTTON_A) {
            if (!gJamSongCycleIndex)
                gMusicAnimationEvent = 1;
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute1ButtonASampleIndex], 90, 0);
        }
        if (gJamAudioRoute1PendingInput & INPUT_BUTTON_B)
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute1ButtonBSampleIndex], 90, 0);
        if (gJamAudioRoute1PendingInput & INPUT_BUTTON_C)
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute1ButtonCSampleIndex], 90, 0);
        if (gJamAudioRoute1PendingInput & INPUT_DIRECTION_MASK) {
            gJamAudioRoute1ButtonASampleIndex = (int)GetRandomNumber() % 42;
            gJamAudioRoute1ButtonBSampleIndex = (int)GetRandomNumber() % 42;
            gJamAudioRoute1ButtonCSampleIndex = (int)GetRandomNumber() % 42;
        }
        gJamAudioRoute1PendingInput = 0;
    }
    if (gJamAudioRoute2PendingInput) {
        if (gJamAudioRoute2PendingInput & INPUT_START) {
            if (++gJamTemplateBankCycleIndex == 3)
                gJamTemplateBankCycleIndex = 0;
            if (!gJamTemplateBankCycleIndex) {
                AudioRequestFmChannelRelease();
                MusicRequestTrackPointerReset();
            } else {
                if (gJamTemplateBankCycleIndex == 1)
                    gMusicAlternateTemplateBankEnabled = 0;
                else if (gJamTemplateBankCycleIndex == 2)
                    gMusicAlternateTemplateBankEnabled = 1;
                gRequestedMusicSelection = gCurrentMusicSelection;
                gCurrentMusicSelection = !gRequestedMusicSelection;
            }
        }
        if (gJamAudioRoute2PendingInput & INPUT_BUTTON_A) {
            if (!gJamSongCycleIndex)
                gMusicAnimationEvent = 1;
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute2ButtonASampleIndex], 90, 0);
        }
        if (gJamAudioRoute2PendingInput & INPUT_BUTTON_B)
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute2ButtonBSampleIndex], 90, 0);
        if (gJamAudioRoute2PendingInput & INPUT_BUTTON_C)
            PlaySoundEffect(PCMPlaybackPointers[gJamAudioRoute2ButtonCSampleIndex], 90, 0);
        if (gJamAudioRoute2PendingInput & INPUT_DIRECTION_MASK) {
            gJamAudioRoute2ButtonASampleIndex = (int)GetRandomNumber() % 42;
            gJamAudioRoute2ButtonBSampleIndex = (int)GetRandomNumber() % 42;
            gJamAudioRoute2ButtonCSampleIndex = (int)GetRandomNumber() % 42;
        }
        gJamAudioRoute2PendingInput = 0;
    }
    if (gPlayerSelection == 1)
        --gFrameCounter;
}

/* Alternate completed lead animations and release their input lock. */
void JamOutUpdateLeadAnimations()
{
    struct object_state *object;
    if (gJamToejamEntities[0]) {
        object = &gPlayerInfo[gJamToejamEntities[0]];
        if (!object->frame_duration) {
            if (gJamToejamAnimationSequence == JamLeadIdleTransitionSequence) {
                if (gJamEarlAnimationSequence == JamLeadIdleLoopSequence)
                    object->frame = gJamEarlAnimationFrame;
                object->sequence = JamLeadIdleLoopSequence;
            } else
                object->sequence = JamLeadIdleTransitionSequence;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            gJamToejamAnimationSequence = object->sequence;
        }
        gJamToejamAnimationFrame = object->frame;
    }
    if (gJamEarlEntities[0]) {
        object = &gPlayerInfo[gJamEarlEntities[0]];
        if (!object->frame_duration) {
            if (gJamEarlAnimationSequence == JamLeadIdleTransitionSequence) {
                if (gJamToejamAnimationSequence == JamLeadIdleLoopSequence)
                    object->frame = gJamToejamAnimationFrame;
                object->sequence = JamLeadIdleLoopSequence;
            } else
                object->sequence = JamLeadIdleTransitionSequence;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            gJamEarlAnimationSequence = object->sequence;
        }
        gJamEarlAnimationFrame = object->frame;
    }
    if (gJamToejamAnimationSequence == JamLeadIdleTransitionSequence || gJamToejamAnimationSequence == JamLeadIdleLoopSequence)
        gJamToejamAnimationLocked = 0;
    if (gJamEarlAnimationSequence == JamLeadIdleTransitionSequence || gJamEarlAnimationSequence == JamLeadIdleLoopSequence)
        gJamEarlAnimationLocked = 0;
}

/* Switch object layouts every 170 or 68 updates and maintain plane scroll. */
void JamOutUpdateLayoutsAndScroll(reset)
short reset;
{
    if (reset) {
        gJamLayoutPhase = 0;
        gJamLayoutCountdown = 170;
        gJamHorizontalScroll = 0;
        gJamVerticalScroll = 0;
        gJamScrollStep = 0;
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = gJamHorizontalScroll;
        VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
        VDP_DATA = gJamVerticalScroll;
        JamOutShowLeadPairLayout();
    } else {
        if (gJamScrollStep) {
            gJamHorizontalScroll -= gJamScrollStep;
            gJamVerticalScroll += gJamScrollStep;
        } else {
            gJamHorizontalScroll = 0;
            gJamVerticalScroll = 0;
        }
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = gJamHorizontalScroll;
        VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
        VDP_DATA = gJamVerticalScroll;
        if (!--gJamLayoutCountdown) {
            gJamLayoutCountdown = 170;
            if (++gJamLayoutPhase >= 6)
                gJamLayoutPhase = 0;
            gJamToejamAnimationLocked = 0;
            gJamEarlAnimationLocked = 0;
            switch (gJamLayoutPhase) {
            case 0:
                JamOutShowLeadPairLayout();
                goto stop_scroll;
            case 1:
                JamOutShowFourPairLayout();
            /* The first two layouts converge on this scroll reset in the ROM.
             * Separate resets produce MCC68K's opposite tail-sharing order. */
            stop_scroll:
                gJamScrollStep = 0;
                break;
            case 2:
                CutsceneReleaseTextEntities();
                JamOutShowNinePairLayout();
                gJamLayoutCountdown = 68;
                break;
            case 3:
                gJamScrollStep = 1;
                gJamLayoutCountdown = 68;
                break;
            case 4:
                gJamScrollStep = -1;
                gJamLayoutCountdown = 68;
                break;
            case 5:
                gJamScrollStep = 0;
                gJamLayoutCountdown = 68;
                break;
            }
        }
    }
}

/* Copies mirror the lead object's frame, sprite size and tile attributes. */
void JamOutSynchronizeEntityCopies()
{
    short object_number, piece, piece_count;
    struct object_state *object, *parent;
    struct hardware_sprite *parent_sprite, *sprite;
    for (object_number = 0, object = gPlayerInfo; object_number < 40;
         ++object_number, ++object) {
        if ((object->draw_flags & ENTITY_ACTIVE) && (object->behavior_flags & ENTITY_SHARED_TILES)) {
            parent = &gPlayerInfo[object->owner];
            object->frame = parent->frame;
            object->sprite_ready = parent->sprite_ready;
            object->current_frame = parent->current_frame;
            piece_count = object->current_frame->piece_count;
            for (piece = 0; piece < piece_count; ++piece) {
                parent_sprite = &gHardwareSpriteTable[parent->primary_sprites[piece]];
                sprite = &gHardwareSpriteTable[object->primary_sprites[piece]];
                sprite->size = parent_sprite->size;
                sprite->tile = parent_sprite->tile;
            }
        }
    }
}

/* Allocate nine copies per character, sharing the lead copy's tile storage. */
void JamOutCreateCharacterCopies()
{
    short slot, tile_count, sprite_count;
    char parent_number;
    unsigned short lead_tile;
    struct object_state *object;
    tile_count = 16;
    sprite_count = 1;
    for (slot = 0; slot < 9; ++slot) {
        if (gJamToejamEntities[slot] = (int)AllocateEntity()) {
            object = &gPlayerInfo[gJamToejamEntities[slot]];
            object->tile = VDP_TILE_PRIORITY;
            object->tile_count = tile_count;
            object->sprite_count = sprite_count;
            object->frames = Spr_JamoutToejam;
            object->x = 160;
            object->y = -50;
            object->sequence = JamLeadIdleLoopSequence;
            object->frame_phase = 6;
            object->frame_duration = 1;
            object->frame = -1;
            if (slot)
                object->behavior_flags |= ENTITY_SHARED_TILES;
            object->sprite_ready = 0;
            AllocateEntityViewportSprites(object, 0);
            if (!slot) {
                lead_tile = object->tile;
                parent_number = gJamToejamEntities[slot];
            } else {
                object->tile = lead_tile;
                object->owner = parent_number;
            }
        }
    }
    tile_count = 19;
    sprite_count = 2;
    for (slot = 0; slot < 9; ++slot) {
        if (gJamEarlEntities[slot] = (int)AllocateEntity()) {
            object = &gPlayerInfo[gJamEarlEntities[slot]];
            object->tile = VDP_TILE_PRIORITY;
            object->tile_count = tile_count;
            object->sprite_count = sprite_count;
            object->frames = Spr_JamoutEarl;
            object->x = 160;
            object->y = -50;
            object->sequence = JamLeadIdleLoopSequence;
            object->frame_phase = 6;
            object->frame_duration = 1;
            object->frame = -1;
            if (slot)
                object->behavior_flags |= ENTITY_SHARED_TILES;
            object->sprite_ready = 0;
            AllocateEntityViewportSprites(object, 0);
            if (!slot) {
                lead_tile = object->tile;
                parent_number = gJamEarlEntities[slot];
            } else {
                object->tile = lead_tile;
                object->owner = parent_number;
            }
        }
    }
}

/* Release every stored Jam Out character-copy entity.
 * MATCHING QUALIFICATION: assigning the handle to an unused temporary retains
 * the ROM's extra word reloads; discarding that assignment changes the code. */
void JamOutReleaseCharacterCopies()
{
    short slot, object_number;
    for (slot = 0; slot < 9; ++slot) {
        if (gJamToejamEntities[slot]) {
            object_number = gJamToejamEntities[slot];
            FreeEntity(gJamToejamEntities[slot]);
        }
        if (gJamEarlEntities[slot]) {
            object_number = gJamEarlEntities[slot];
            FreeEntity(gJamEarlEntities[slot]);
        }
    }
}

/* Move one allocated ToeJam copy. */
void JamOutPositionToejamCopy(slot, x, y)
int slot, x, y;
{
    struct object_state *object;
    if (gJamToejamEntities[slot]) {
        object = &gPlayerInfo[gJamToejamEntities[slot]];
        object->x = x;
        object->y = y;
    }
}

/* Move one allocated Earl copy. */
void JamOutPositionEarlCopy(slot, x, y)
int slot, x, y;
{
    struct object_state *object;
    if (gJamEarlEntities[slot]) {
        object = &gPlayerInfo[gJamEarlEntities[slot]];
        object->x = x;
        object->y = y;
    }
}

/* Put a ToeJam copy at the common off-screen location. */
void JamOutHideToejamCopy(slot)
int slot;
{
    struct object_state *object;
    if (gJamToejamEntities[slot]) {
        object = &gPlayerInfo[gJamToejamEntities[slot]];
        object->x = 160;
        object->y = -50;
    }
}

/* Put an Earl copy at the common off-screen location. */
void JamOutHideEarlCopy(slot)
int slot;
{
    struct object_state *object;
    if (gJamEarlEntities[slot]) {
        object = &gPlayerInfo[gJamEarlEntities[slot]];
        object->x = 160;
        object->y = -50;
    }
}

/* Change the lead ToeJam animation unless its current action is locked. */
void JamOutSetToejamSequence(sequence, period)
char *sequence;
short period;
{
    struct object_state *object;
    if (gJamToejamAnimationLocked)
        return;
    if (gJamToejamEntities[0]) {
        object = &gPlayerInfo[gJamToejamEntities[0]];
        object->sequence = sequence;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = period;
    }
    gJamToejamAnimationSequence = sequence;
}

/* Change the lead Earl animation unless its current action is locked. */
void JamOutSetEarlSequence(sequence, period)
char *sequence;
short period;
{
    struct object_state *object;
    if (gJamEarlAnimationLocked)
        return;
    if (gJamEarlEntities[0]) {
        object = &gPlayerInfo[gJamEarlEntities[0]];
        object->sequence = sequence;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = period;
    }
    gJamEarlAnimationSequence = sequence;
}

/* Show the two lead objects and upload the 16 by 22 packed background. */
void JamOutShowLeadPairLayout()
{
    short row, column, tile;
    for (row = 1; row < 9; ++row) {
        JamOutHideToejamCopy(row);
        JamOutHideEarlCopy(row);
    }
    JamOutPositionEarlCopy(0, 174, 127);
    JamOutPositionToejamCopy(0, 144, 127);
    InitializeTitleTransitionTilemap();
    ClearUpperVramTilemap();
    for (row = 0; row < 16; ++row) {
        VDP_COMMAND = 0x60120003L + ((row + 6) << 23);
        for (column = 0; column < 22; ++column) {
            tile = CrashPackedTilemapA[row][column];
            if (tile & 64)
                tile |= 0x800;
            if (tile & 128)
                tile |= 0x1000;
            tile &= 0xff3f;
            VDP_DATA = tile + 0xa233;
        }
    }
}

/* Upload the 14 by 20 packed background at a tile coordinate. */
void JamOutDrawPackedBackgroundBlock(x, y)
short x, y;
{
    short row, column, tile;
    for (row = 0; row < 14; ++row) {
        VDP_COMMAND = (((y + row) << 23) + VDP_WRITE_PLANE_B) + (x << 17);
        for (column = 0; column < 20; ++column) {
            tile = CrashPackedTilemapB[row][column];
            if (tile & 64)
                tile |= 0x800;
            if (tile & 128)
                tile |= 0x1000;
            tile &= 0xff3f;
            VDP_DATA = tile + 0xa233;
        }
    }
}

/* Place four object pairs and draw four copies of the packed background. */
void JamOutShowFourPairLayout()
{
    short slot;
    for (slot = 4; slot < 9; ++slot) {
        JamOutHideToejamCopy(slot);
        JamOutHideEarlCopy(slot);
    }
    JamOutPositionToejamCopy(0, 65, 70);
    JamOutPositionEarlCopy(0, 95, 70);
    JamOutPositionToejamCopy(1, 225, 70);
    JamOutPositionEarlCopy(1, 255, 70);
    JamOutPositionToejamCopy(2, 65, 182);
    JamOutPositionEarlCopy(2, 95, 182);
    JamOutPositionToejamCopy(3, 225, 182);
    JamOutPositionEarlCopy(3, 255, 182);
    ClearUpperVramTilemap();
    JamOutDrawPackedBackgroundBlock(0, 0);
    JamOutDrawPackedBackgroundBlock(20, 0);
    JamOutDrawPackedBackgroundBlock(0, 14);
    JamOutDrawPackedBackgroundBlock(20, 14);
}

/* Arrange all nine copies of each character in a three by three grid. */
void JamOutShowNinePairLayout()
{
    JamOutPositionToejamCopy(0, 38, 53);
    JamOutPositionEarlCopy(0, 68, 53);
    JamOutPositionToejamCopy(1, 144, 53);
    JamOutPositionEarlCopy(1, 174, 53);
    JamOutPositionToejamCopy(2, 250, 53);
    JamOutPositionEarlCopy(2, 280, 53);
    JamOutPositionToejamCopy(3, 38, 127);
    JamOutPositionEarlCopy(3, 68, 127);
    JamOutPositionToejamCopy(4, 144, 127);
    JamOutPositionEarlCopy(4, 174, 127);
    JamOutPositionToejamCopy(5, 250, 127);
    JamOutPositionEarlCopy(5, 280, 127);
    JamOutPositionToejamCopy(6, 38, 201);
    JamOutPositionEarlCopy(6, 68, 201);
    JamOutPositionToejamCopy(7, 144, 201);
    JamOutPositionEarlCopy(7, 174, 201);
    JamOutPositionToejamCopy(8, 250, 201);
    JamOutPositionEarlCopy(8, 280, 201);
    ClearUpperVramTilemap();
    InitializeTitleTransitionTilemap();
}
