#include "entities.h"
#include "earthlings.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_system.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"

#define NEXT_RANDOM PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0]

/* Advance entry, bathing, speech, healing, and exit poses at the hot tub. */
void HottubAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    short sprite = -1;
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if (gPlayer1Powerup[player_number] != 1 &&
        abs(player->y - (earthling->y - 4)) < 5) {
        if (player->sprite_state == 11 && earthling->x - player->x > 34) {
            sprite = 31;
            TOEJAM_JUMP_COUNTER[player_number] = 0;
        } else if (player->sprite_state == 10 && player->x - earthling->x > 34) {
            sprite = 30;
            TOEJAM_JUMP_COUNTER[player_number] = 0;
        } else if (player->flags_52 & 0x80) {
            if (++TOEJAM_JUMP_COUNTER[player_number] > 4) {
                sprite = player->sprite_state + 4;
                gPlayerInfo[earthling->object].flags_4d &= ~1;
                player->height = 1;
            }
        } else if (player->flags_52 & 0x100) {
            if (!player->frame_duration) {
                StopPlayer(player);
                gPlayerInfo[earthling->object].flags_4d |= 1;
                player->x = earthling->x;
                player->y = earthling->y;
                player->height = 35;
                sprite = player->sprite_state + 4;
                player->acceleration_height = -64;
            }
        } else if (player->flags_52 & 0x200) {
            if (player->height < 22) {
                StopPlayer(player);
                player->height = 22;
                player->draw_flags &= ~64;
                sprite = player->sprite_state + 4;
                PlaySoundEffect((long)Sfx_Splash, 50, 0);
            }
        } else if (player->flags_52 & 0x400) {
            if (!player->frame_duration) {
                sprite = 119;
                player->height = 0;
                TOEJAM_JUMP_COUNTER[player_number] = 0;
            }
        } else if (player->sprite_state == 119) {
            if (!player_number) {
                if (P1_PROCESSED_INPUT[0] & 4) {
                    sprite = 120;
                    player->flags_4d |= 32;
                } else if (!(GetRandomNumber() & 31)) {
                    player->x -= 20;
                    GenerateSpeech(0, "chat", 0);
                    player->x += 20;
                } else if (!(GetRandomNumber() & 31)) {
                    if (NEXT_RANDOM & 1)
                        GenerateSpeech(0, "giggle", 1);
                    else
                        GenerateSpeech(0, "titter", 1);
                }
            } else if (player_number == 1) {
                if (P1_PROCESSED_INPUT[1] & 8) {
                    sprite = 120;
                    player->flags_4d |= 32;
                } else if (!(GetRandomNumber() & 31)) {
                    player->x += 20;
                    GenerateSpeech(1, "chat", 0);
                    player->x -= 20;
                } else if (gPlayerInfo[0].sprite_state != 119 &&
                           !(GetRandomNumber() & 31)) {
                    if (NEXT_RANDOM & 1)
                        GenerateSpeech(1, "giggle", 1);
                    else
                        GenerateSpeech(1, "titter", 1);
                }
            }
            if (++TOEJAM_JUMP_COUNTER[player_number] > 5)
                ++TOEJAMS_HEALTH[player_number];
        } else if (player->sprite_state == 120 && !player->frame_duration) {
            if (!player_number) {
                sprite = 23;
                player->x -= 55;
                player->height = 42;
            } else {
                sprite = 22;
                player->x += 67;
                player->height = 39;
            }
            player->draw_flags |= 64;
            player->flags_4d &= ~32;
            player->acceleration_height = -32;
            ram_ffda30[player_number] = 2;
        }
    }
    if (sprite != -1)
        SetSprite(player, sprite);
}
