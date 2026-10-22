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
    short reset_selected = 0;

    DrawPauseChoicePanel(0);
    PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
    WaitForNextVBlank(1);
    PlaySound(PSG_TONE_PHRASE, 99);
    AudioRequestChannelTemplateEffect(FM_EFFECT_CLEAR);
    AudioRequestFmChannelRelease();
    MusicRequestTrackPointerReset();
    while (ReadMappedPlayerInput(player_number) & 128)
        ;
    for (;;) {
        if (!reset_selected && (ReadMappedPlayerInput(player_number) & 2)) {
            DrawPauseChoicePanel(1);
            reset_selected = 1;
            continue;
        }
        if (reset_selected && (ReadMappedPlayerInput(player_number) & 1)) {
            DrawPauseChoicePanel(0);
            reset_selected = 0;
            continue;
        }
        if (ReadMappedPlayerInput(player_number) & 192)
            break;
    }
    DrawPauseChoicePanel(-1);
    if (reset_selected)
        RestartFromResetEntry();
    gCurrentMusicSelection = MUSIC_SELECTION_INVALID;
    while (ReadMappedPlayerInput(player_number) & 192)
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

    if (gPlayer1MapLevel == 26) {
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
        viewport = 0;
        viewport_mask = 2;
        if (gSplitScreenEnabled) {
            if (player_number == PLAYER_TOEJAM)
                object->height = 168;
            else {
                object->height = 168;
                object->level = gPlayer2MapLevel;
                object->x = gCamera2PendingX + 160;
                object->y = gCamera2PendingY + 225;
                viewport = 1;
                viewport_mask = 4;
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
        object->frame = -1;
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
        while ((ReadMappedPlayerInput(0) | ReadMappedPlayerInput(1)) & 128)
            ;
        for (;;) {
            first_input = ReadMappedPlayerInput(0);
            second_input = ReadMappedPlayerInput(1);
            if ((first_input | second_input) & 128)
                break;
            switch (input_stage) {
            case 0:
                if (first_input == 113 || second_input == 113)
                    ++input_stage;
                continue;
            case 1:
                if (first_input == 72 || second_input == 72)
                    ++input_stage;
                continue;
            case 2:
                if (first_input == 18 || second_input == 18)
                    ++input_stage;
                continue;
            case 3:
                if (first_input == 36 || second_input == 36) {
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
        while ((ReadMappedPlayerInput(0) | ReadMappedPlayerInput(1)) & 128)
            ;
    }
}
