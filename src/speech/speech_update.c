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
                PlaySoundEffect((long)Sfx_Achoo, 50, 0);
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
