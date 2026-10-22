#include "entities.h"
#include "sprites.h"
#include "revision.h"
#include "assets_audio.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "player.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"

/* Start the book-reading sprite and its sixty-tick countdown. */
void OpenSchoolBook(state)
struct object_state *state;
{
    SetSprite(state, 116);
    ram_ffda3e[state->entity_number] = 60;
    StopPlayer(state);
    state->mode_15 = 0;
    state->height = 0;
}

/* Enter sleep after the reading countdown or prolonged idle time. While
 * sleeping, changed pad input can allocate successive whisper/shout effects;
 * completing the final effect restores the active-present sprite. */
void ProcessSleep(state)
struct object_state *state;
{
    short next_sprite = -1;
    short player, effect;
    unsigned char input;
    struct object_state *wake_effect;
    struct timed_animation *animation;

    player = state->entity_number;
    if (ELEVATOR_FLAG[player] || state->level == 26)
        return;
    if (state->sprite_state == 116) {
        if (--ram_ffda3e[player] <= 0) {
            next_sprite = 117;
            ram_ffda40[player] = GetRandomNumber() % 7 + 4;
        }
    } else if (state->sprite_state == 117) {
        input = MovePadInputIntoD0(player);
        if (effect = ram_ffda42[player]) {
            wake_effect = &gPlayerInfo[effect];
            if ((!ram_ffda40[player] && --P1_IDLE_COUNTER[player] <= 0) ||
                !wake_effect->frame_duration) {
                func_00d970(effect);
                ram_ffda42[player] = 0;
                if (!ram_ffda40[player])
                    next_sprite = GetActivePresentSprite(state);
            }
        } else if (input && input != P1_KEYDOWN[player] &&
                   PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] % 100 < 20) {
            if (effect = func_00d904()) {
                ram_ffda42[player] = effect;
                wake_effect = &gPlayerInfo[effect];
                --ram_ffda40[player];
                wake_effect->x = state->x;
                wake_effect->y = state->y;
                wake_effect->level = state->level;
                wake_effect->width = 64;
                wake_effect->height_pixels = 32;
                wake_effect->sprite_count = 2;
                wake_effect->tile_count = 32;
                wake_effect->tile = 0x8000;
                animation = &AnimationDescriptors_0AF0B8[player];
                wake_effect->frames = animation->frames;
                wake_effect->frame_duration = 1;
                wake_effect->frame = -1;
                if (ram_ffda40[player]) {
                    wake_effect->frame_phase = 15;
                    if (ram_ffda40[player] & 1)
                        wake_effect->sequence = InlineAnimationPrograms_AF0CC;
                    else
                        wake_effect->sequence = &InlineAnimationPrograms_AF0CC[2];
                    PlaySoundEffect((long)Sfx_WakeUpWhisper, 50, 0);
                } else {
                    wake_effect->sequence = animation->sequence;
                    wake_effect->frame_phase = animation->period;
                    PlaySoundEffect((long)Sfx_WakeUpShout, 88, 0);
                    P1_IDLE_COUNTER[player] = 30;
                }
            }
        } else if (!TOEJAM_SPEECH_COUNTER)
            PlaySoundEffect((long)Sfx_Snoring, 30, 0);
        P1_KEYDOWN[player] = input;
    } else {
        if (effect = ram_ffda42[player]) {
            func_00d970(effect);
            ram_ffda42[player] = 0;
        }
        if (!state->sprite_state
#if REVISION >= 2
            && !ram_ffe236[player]
#endif
        ) {
            if (++P1_IDLE_COUNTER[player] > 300) {
                next_sprite = 117;
                ram_ffda40[player] = GetRandomNumber() % 7 + 4;
            }
        } else
            P1_IDLE_COUNTER[player] = 0;
    }
    if (next_sprite != -1)
        SetSprite(state, next_sprite);
}
