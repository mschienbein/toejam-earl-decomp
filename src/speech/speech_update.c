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
void func_01e4e4()
{
    struct object_state *bubble;
    short slot, entity_number, distance_x, distance_y;
    char player_number;

    for (slot = 0; slot < 4; ++slot) {
        if (entity_number = ram_ffe1ee[slot]) {
            bubble = &gPlayerInfo[entity_number];
            if (--ram_ffe1f2[slot] < 0 || !(bubble->draw_flags & 6)) {
                func_00d970(entity_number);
                ram_ffe1ee[slot] = 0;
            }
        }
    }
    for (slot = 0; slot < 2; ++slot) {
        if (ram_ffe1f8[slot] && !--ram_ffe1f8[slot])
            GenerateSpeech(ram_ffe204[slot], ram_ffe1fc[slot], 1);
    }
    if ((GetRandomNumber() & 0x1fff) == 1) {
        player_number = GetRandomNumber() & 1;
        if (func_018ce2(gPlayerInfo[player_number].draw_flags & 6)) {
            slot = func_01e38e(0, player_number);
            if (slot != -1) {
                PlaySoundEffect((long)Sfx_Achoo, 50, 0);
                if (SPLIT_SCREEN)
                    ram_ffe1f8[slot] = 0;
            }
        }
    }
    if (gPlayerInfo[0].level == gPlayerInfo[1].level) {
        distance_x = abs(gPlayerInfo[0].x - gPlayerInfo[1].x);
        distance_y = abs(gPlayerInfo[0].y - gPlayerInfo[1].y);
        if (ram_ffe1f6 && !SPLIT_SCREEN && distance_x < 260 && distance_y < 174) {
            /* ORIGINAL BUG: These dialogue calls omit their speaker argument. */
            if (func_008356(0) && !(GetRandomNumber() & 3))
                func_01e38e(3);
            else if (func_008356(1) && !(GetRandomNumber() & 3))
                func_01e38e(4);
            else
                func_01e38e(1);
            ram_ffe1f6 = 0;
        } else if (distance_x > 640 || distance_y > 448)
            ram_ffe1f6 = 1;
        /* ORIGINAL BUG: The observed bound visits only the first player. */
        for (slot = 0; slot < 1; ++slot) {
            if (ram_ffe1dc[slot] && distance_x < 30 && distance_y < 20 &&
                (GetRandomNumber() & 0xff) == 2) {
                if (!slot)
                    func_01e38e(22);
                else
                    func_01e38e(23);
            }
        }
    } else
        ram_ffe1f6 = 1;
}
