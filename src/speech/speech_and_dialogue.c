#include "hardware.h"
#include "player_animation.h"
#include "assets_ui.h"
#include "ram_sprites.h"
#include "sprite_calls.h"
#include "ui_calls.h"
#include "speech.h"

/* Center at most twelve text glyphs in a thirteen-tile transfer, including
 * the initial speech-bubble tile, and enqueue its VRAM destination.
 * MATCHING QUALIFICATION: The initial tile uses a long-pointer cast of its
 * element address, consistent with the tile copier's actual source type.
 * The input byte uses a redundant char-pointer cast of its element address
 * and an explicit int promotion of the short index. These deliberately
 * retained address-expression boundaries select the original LEA and ADDA
 * operations. Plain pointer addition, plain subscripting, and removing
 * either input boundary did not match; no historical spelling is claimed. */
void QueueSpeechTextTiles(text, length, tile)
char *text;
short length, tile;
{
    short index, padding;

    AppendSpriteTilesToBuffer((long *)&RuntimeTextFontTiles[SPEECH_BUBBLE_PREFIX_GLYPH * VDP_TILE_BYTES], 1);
    {
        int unused_columns;
        unused_columns = SPEECH_TEXT_MAX_GLYPHS - length;
        padding = unused_columns / 2;
    }
    for (index = 0; index < padding; ++index)
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    for (index = 0; index < length; ++index) {
        char glyph = TextMapCharacterToGlyph(*(char *)&text[(int)index]);
        if (glyph == SPEECH_BUBBLE_PREFIX_GLYPH)
            ++glyph;
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles + VDP_TILE_BYTES * glyph, 1);
    }
    if (length & 1)
        ++padding;
    for (index = 0; index < padding; ++index)
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = SPEECH_TEXT_UPLOAD_TILES;
}

#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_ui.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Set the lifetime override consumed by the next allocated speech bubble. */
void SetNextSpeechLifetime(lifetime)
short lifetime;
{
    gNextSpeechLifetimeOverride = lifetime;
}

/* Allocate one of four speech bubbles, replacing the oldest when necessary.
 * Attached bubbles retain their speaker so later updates can follow it. */
short GenerateSpeech(speaker, text, replace_attached)
short speaker;
char *text;
char replace_attached;
{
    struct object_state *source, *bubble;
    short slot, entity_number, index, length, viewport;

    source = &gPlayerInfo[speaker];
    if (!(source->draw_flags & ENTITY_EITHER_VIEW))
        return 0;
    if (speaker <= PLAYER_EARL && source->sprite_state == PLAYER_ANIM_SLEEP)
        return 0;
    if (replace_attached) {
        for (slot = 0; slot < 4; ++slot) {
            if (entity_number = gSpeechBubbleEntities[slot]) {
                bubble = &gPlayerInfo[entity_number];
                if ((bubble->behavior_flags & ENTITY_ATTACHED) && bubble->owner == speaker) {
                    FreeEntity(entity_number);
                    gSpeechBubbleEntities[slot] = 0;
                }
            }
        }
    }
    for (slot = 0; slot < 4; ++slot)
        if (!gSpeechBubbleEntities[slot])
            break;
    if (slot == 4) {
        slot = 0;
        for (index = 1; index < 4; ++index)
            if (gSpeechBubbleLifetimes[index] < gSpeechBubbleLifetimes[slot])
                slot = index;
        FreeEntity(gSpeechBubbleEntities[slot]);
    }
    for (length = 0; text[length]; ++length)
        ;
    if (length > SPEECH_TEXT_MAX_GLYPHS)
        length = SPEECH_TEXT_MAX_GLYPHS;
    if (gSpeechBubbleEntities[slot] = (int)AllocateEntity()) {
        bubble = &gPlayerInfo[gSpeechBubbleEntities[slot]];
        bubble->x = source->x;
        bubble->y = source->y + 15;
        bubble->height = source->height + 15;
        bubble->level = source->level;
        bubble->width = length << 3;
        bubble->height_pixels = 16;
        bubble->sprite_count = 4;
        bubble->tile_count = SPEECH_TEXT_UPLOAD_TILES;
        if (gPlayer1MapLevel < LEVEL_FINAL)
            bubble->tile = VDP_TILE_PRIORITY;
        bubble->current_frame = &SpeechBubbleTextFrame;
        if (source->draw_flags & ENTITY_PRIMARY_VIEW) {
            AllocateEntityViewportSprites(bubble, 0);
            viewport = 0;
        } else {
            AllocateEntityViewportSprites(bubble, 1);
            viewport = 1;
        }
        if (!(bubble->draw_flags & ENTITY_EITHER_VIEW)) {
            FreeEntity(gSpeechBubbleEntities[slot]);
            gSpeechBubbleEntities[slot] = 0;
            return 0;
        }
        bubble->draw_flags |= ENTITY_SKIP_PHYSICS;
        while (gSpriteGraphicsUploadPending)
            ;
        QueueSpeechTextTiles(text, length, bubble->tile & VDP_TILE_INDEX_MASK);
        PrepareEntitySpriteFrame(bubble, &SpeechBubbleTextFrame, 1, viewport);
        bubble->sprite_ready = 1;
        if (replace_attached) {
            bubble->behavior_flags |= ENTITY_ATTACHED;
            bubble->owner = speaker;
            bubble->attachment_y_or_latch = 15;
            bubble->attachment_height = 15;
        }
        if (gNextSpeechLifetimeOverride) {
            gSpeechBubbleLifetimes[slot] = gNextSpeechLifetimeOverrideLowByte;
            gNextSpeechLifetimeOverride = 0;
        } else
            gSpeechBubbleLifetimes[slot] = 40;
        return gSpeechBubbleEntities[slot];
    }
    return 0;
}

/* Release active speech objects and discard both pending replies. */
void ClearSpeechBubblesAndReplies()
{
    short slot;

    for (slot = 0; slot < 4; ++slot) {
        if (gSpeechBubbleEntities[slot]) {
            FreeEntity(gSpeechBubbleEntities[slot]);
            gSpeechBubbleEntities[slot] = 0;
        }
    }
    gDelayedSpeechReplyTimers[0] = 0;
    gSecondSpeechReplyTimer = 0;
    gReunionDialoguePending = 0;
}

/* Report either an existing bubble or a pending reply for this speaker. */
int HasSpeakerSpeech(speaker)
short speaker;
{
    short slot;
    struct object_state *bubble;

    for (slot = 0; slot < 4; ++slot) {
        if (gSpeechBubbleEntities[slot]) {
            bubble = &gPlayerInfo[gSpeechBubbleEntities[slot]];
            if ((bubble->draw_flags & ENTITY_SKIP_PHYSICS) && bubble->owner == speaker)
                return 1;
        }
    }
    if ((gDelayedSpeechReplyTimers[0] && gDelayedSpeechReplySpeakers[0] == speaker) ||
        (gSecondSpeechReplyTimer && gSecondDelayedSpeechReplySpeaker == speaker))
        return 1;
    return 0;
}

#include "ram_player.h"
#include "ram_ui.h"
#include "ram_camera.h"
#include "speech.h"
#include "ram_speech.h"
#include "assets_ui.h"

/* Speak the opening line and, when eligible, schedule its delayed reply. */
int StartDialogue(dialogue_number, speaker)
short dialogue_number, speaker;
{
    short selected_speaker;
    short slot = 0;
    short reply_speaker;
    struct dialogue_descriptor *dialogue;

    if (gDelayedSpeechReplyTimers[0]) {
        slot = 1;
        if (gDelayedSpeechReplyTimers[1]) {
            slot = -1;
            return slot;
        }
    }
    dialogue = &DialogueDescriptors[dialogue_number];
    if (dialogue->first_speaker <= PLAYER_EARL)
        speaker = dialogue->first_speaker;
    else if (gDelayedSpeechReplyTimers[slot ^ 1] && gDelayedSpeechReplySpeakers[slot ^ 1] == speaker) {
        slot = -1;
        return slot;
    }
    /* MATCHING QUALIFICATION: This redundant short-to-short copy deliberately
     * separates the selected-speaker lifetime after the descriptor/duplicate
     * check. MCC68K coalesces it with the incoming word load; using the parameter
     * throughout or copying it at entry does not match. No new state is added. */
    selected_speaker = speaker;
    if (dialogue->reply_speaker <= PLAYER_EARL)
        gDelayedSpeechReplySpeakers[slot] = dialogue->reply_speaker;
    else if (dialogue->reply_speaker == DIALOGUE_REPLY_FIRST_SPEAKER)
        gDelayedSpeechReplySpeakers[slot] = selected_speaker;
    else
        gDelayedSpeechReplySpeakers[slot] = selected_speaker ^ 1;
    reply_speaker = gDelayedSpeechReplySpeakers[slot];
    if (selected_speaker <= PLAYER_EARL && (gPlayerLives[selected_speaker] < 0 || gPlayerInventoryOpen[selected_speaker])) {
        slot = -1;
        return slot;
    }
    if (dialogue->first_line)
        GenerateSpeech(selected_speaker, dialogue->first_line, 1);
    if (dialogue->reply && (!gSplitScreenEnabled || selected_speaker == reply_speaker) &&
        (reply_speaker > PLAYER_EARL ||
         (gPlayerLives[reply_speaker] >= 0 && !gPlayerInventoryOpen[reply_speaker]))) {
        gDelayedSpeechReplyTimers[slot] = dialogue->delay;
        gDelayedSpeechReplyTexts[slot] = dialogue->reply;
    }
    return slot;
}

#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"

/* Expire bubbles, deliver delayed replies, and trigger the observed random
 * sneeze and player-reunion conversations. */
void UpdateSpeechAndDialogue()
{
    struct object_state *bubble;
    short slot, entity_number, distance_x, distance_y;
    char player_number;

    for (slot = 0; slot < 4; ++slot) {
        if (entity_number = gSpeechBubbleEntities[slot]) {
            bubble = &gPlayerInfo[entity_number];
            if (--gSpeechBubbleLifetimes[slot] < 0 || !(bubble->draw_flags & ENTITY_EITHER_VIEW)) {
                FreeEntity(entity_number);
                gSpeechBubbleEntities[slot] = 0;
            }
        }
    }
    for (slot = 0; slot < 2; ++slot) {
        if (gDelayedSpeechReplyTimers[slot] && !--gDelayedSpeechReplyTimers[slot])
            GenerateSpeech(gDelayedSpeechReplySpeakers[slot], gDelayedSpeechReplyTexts[slot], 1);
    }
    if ((GetRandomNumber() & 0x1fff) == 1) {
        player_number = GetRandomNumber() & 1;
        if (EarthlingResetStateTwoByDrawMask(gPlayerInfo[player_number].draw_flags & ENTITY_EITHER_VIEW)) {
            slot = StartDialogue(0, player_number);
            if (slot != -1) {
                PlaySoundEffect((long)Sfx_Achoo, AUDIO_PRIORITY_NORMAL, 0);
                if (gSplitScreenEnabled)
                    gDelayedSpeechReplyTimers[slot] = 0;
            }
        }
    }
    if (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level) {
        distance_x = abs(gPlayerInfo[PLAYER_TOEJAM].x - gPlayerInfo[PLAYER_EARL].x);
        distance_y = abs(gPlayerInfo[PLAYER_TOEJAM].y - gPlayerInfo[PLAYER_EARL].y);
        if (gReunionDialoguePending && !gSplitScreenEnabled && distance_x < 260 && distance_y < 174) {
            /* ORIGINAL BUG: These dialogue calls omit their speaker argument. */
            if (PlayerHasOddElevatorState(0) && !(GetRandomNumber() & 3))
                StartDialogue(3);
            else if (PlayerHasOddElevatorState(1) && !(GetRandomNumber() & 3))
                StartDialogue(4);
            else
                StartDialogue(1);
            gReunionDialoguePending = 0;
        } else if (distance_x > 640 || distance_y > 448)
            gReunionDialoguePending = 1;
        /* ORIGINAL BUG: The observed bound visits only the first player. */
        for (slot = 0; slot < 1; ++slot) {
            if (gPlayerControlRemapEntities[slot] && distance_x < 30 && distance_y < 20 &&
                (GetRandomNumber() & 0xff) == 2) {
                if (!slot)
                    StartDialogue(22);
                else
                    StartDialogue(23);
            }
        }
    } else
        gReunionDialoguePending = 1;
}
