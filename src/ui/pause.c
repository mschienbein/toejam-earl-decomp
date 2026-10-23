#include "psg_effects.h"
#include "fm_effects.h"
#include "music_ids.h"
#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "engine_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ship.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Level 26 uses a pause loop with a two-way option and a reset action. */
void RunFinalLevelPauseMenu(player_number)
short player_number;
{
    short reset_selected = PAUSE_CHOICE_CONTINUE;

    DrawPauseChoicePanel(PAUSE_CHOICE_CONTINUE);
    PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
    WaitForNextVBlank(1);
    PlaySound(PSG_TONE_PHRASE, 99);
    AudioRequestChannelTemplateEffect(FM_EFFECT_CLEAR);
    AudioRequestFmChannelRelease();
    MusicRequestTrackPointerReset();
    while (ReadMappedPlayerInput(player_number) & INPUT_START)
        ;
    for (;;) {
        if (!reset_selected && (ReadMappedPlayerInput(player_number) & INPUT_DOWN)) {
            DrawPauseChoicePanel(PAUSE_CHOICE_RESET);
            reset_selected = PAUSE_CHOICE_RESET;
            continue;
        }
        if (reset_selected && (ReadMappedPlayerInput(player_number) & INPUT_UP)) {
            DrawPauseChoicePanel(PAUSE_CHOICE_CONTINUE);
            reset_selected = PAUSE_CHOICE_CONTINUE;
            continue;
        }
        if (ReadMappedPlayerInput(player_number) & (INPUT_START | INPUT_BUTTON_A))
            break;
    }
    DrawPauseChoicePanel(PAUSE_CHOICE_HIDDEN);
    if (reset_selected)
        RestartFromResetEntry();
    gCurrentMusicSelection = MUSIC_SELECTION_INVALID;
    while (ReadMappedPlayerInput(player_number) & (INPUT_START | INPUT_BUTTON_A))
        ;
}

/* Display the pause object, wait for a Start transition, and recognize the
 * four observed input combinations. The last combination updates a ten-byte
 * table; its gameplay meaning is not established here. */
void GamePaused(player_number)
short player_number;
{
    unsigned short input_stage = 0;
    unsigned short object_number, viewport, viewport_mask;
    short entry_number, minimum_entry;
    unsigned char first_input, second_input;
    struct object_state *object;
    struct animation_descriptor *animation;

    if (gPlayer1MapLevel == LEVEL_FINAL) {
        RunFinalLevelPauseMenu(player_number);
        return;
    }
    if (gPlayerInventoryOpen[player_number])
        return;
    if (object_number = AllocateEntityWithEviction(0)) {
        object = &gPlayerInfo[object_number];
        object->x = gCamera1PendingX + 160;
        object->y = gCamera1PendingY + 225;
        object->level = gPlayer1MapLevel;
        viewport = VIEWPORT_PRIMARY;
        viewport_mask = ENTITY_PRIMARY_VIEW;
        if (gSplitScreenEnabled) {
            if (player_number == PLAYER_TOEJAM)
                object->height = 168;
            else {
                object->height = 168;
                object->level = gPlayer2MapLevel;
                object->x = gCamera2PendingX + 160;
                object->y = gCamera2PendingY + 225;
                viewport = VIEWPORT_SECONDARY;
                viewport_mask = ENTITY_SECONDARY_VIEW;
            }
        } else
            object->height = 112;
        object->sprite_count = 4;
        object->tile_count = 48;
        object->tile |= VDP_TILE_PRIORITY;
        animation = &PauseDisplayAnimation;
        object->sequence = animation->sequence;
        object->frames = animation->frames;
        object->frame_duration = 1;
        object->frame = SPRITE_FRAME_BEFORE_FIRST;
        AllocateEntityViewportSprites(object, viewport);
        while (!(object->draw_flags & viewport_mask)) {
            EntityReleaseOneByPriority();
            AllocateEntityViewportSprites(object, viewport);
        }
        do {
            UpdateAndQueueEntitySprites();
        } while (!object->sprite_ready);
        PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
        WaitForNextVBlank(1);
        PlaySound(PSG_TONE_PHRASE, 99);
        AudioRequestChannelTemplateEffect(FM_EFFECT_CLEAR);
        AudioRequestFmChannelRelease();
        MusicRequestTrackPointerReset();
        while ((ReadMappedPlayerInput(PLAYER_TOEJAM) | ReadMappedPlayerInput(PLAYER_EARL)) & INPUT_START)
            ;
        for (;;) {
            first_input = ReadMappedPlayerInput(PLAYER_TOEJAM);
            second_input = ReadMappedPlayerInput(PLAYER_EARL);
            if ((first_input | second_input) & INPUT_START)
                break;
            switch (input_stage) {
            case 0:
                if (first_input == (INPUT_ACTION_MASK | INPUT_UP) || second_input == (INPUT_ACTION_MASK | INPUT_UP))
                    ++input_stage;
                continue;
            case 1:
                if (first_input == (INPUT_BUTTON_A | INPUT_RIGHT) || second_input == (INPUT_BUTTON_A | INPUT_RIGHT))
                    ++input_stage;
                continue;
            case 2:
                if (first_input == (INPUT_BUTTON_B | INPUT_DOWN) || second_input == (INPUT_BUTTON_B | INPUT_DOWN))
                    ++input_stage;
                continue;
            case 3:
                if (first_input == (INPUT_BUTTON_C | INPUT_LEFT) || second_input == (INPUT_BUTTON_C | INPUT_LEFT)) {
                    entry_number = 0;
                    do {
                        if (gShipPieceLevelAssignments[entry_number])
                            break;
                        ++entry_number;
                    } while (entry_number < 10);
                    minimum_entry = entry_number;
                    for (++entry_number; entry_number < 10; ++entry_number)
                        if (gShipPieceLevelAssignments[entry_number] &&
                            gShipPieceLevelAssignments[entry_number] < gShipPieceLevelAssignments[minimum_entry])
                            minimum_entry = entry_number;
                    entry_number = 0;
                    do {
                        if (entry_number != minimum_entry)
                            gShipPieceLevelAssignments[entry_number] = 0;
                        ++entry_number;
                    } while (entry_number < 10);
                    if (gShipPieceLevelAssignments[minimum_entry] != gPlayer1MapLevel)
                        gShipPieceLevelAssignments[minimum_entry] = gPlayer1MapLevel + 1;
                    PlaySound(PSG_TONE_PHRASE, 99);
                    ++input_stage;
                }
                continue;
            }
        }
        gCurrentMusicSelection = MUSIC_SELECTION_INVALID;
        FreeEntity(object_number);
        while ((ReadMappedPlayerInput(PLAYER_TOEJAM) | ReadMappedPlayerInput(PLAYER_EARL)) & INPUT_START)
            ;
    }
}
