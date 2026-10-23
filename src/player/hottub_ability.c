#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
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

#define NEXT_RANDOM gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex]

/* Advance entry, bathing, speech, healing, and exit poses at the hot tub. */
void HottubAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    short sprite = PLAYER_ANIM_UNCHANGED;
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if (gPlayerPowerups[player_number] != PRESENT_SPRING_SHOES &&
        abs(player->y - (earthling->y - 4)) < 5) {
        if (player->sprite_state == PLAYER_ANIM_WALK_RIGHT && earthling->x - player->x > 34) {
            sprite = PLAYER_ANIM_JUMP_WINDUP_RIGHT;
            gPlayerJumpCounters[player_number] = 0;
        } else if (player->sprite_state == PLAYER_ANIM_WALK_LEFT && player->x - earthling->x > 34) {
            sprite = PLAYER_ANIM_JUMP_WINDUP_LEFT;
            gPlayerJumpCounters[player_number] = 0;
        } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_WINDUP) {
            if (++gPlayerJumpCounters[player_number] > 4) {
                sprite = player->sprite_state + 4;
                gPlayerInfo[earthling->object].behavior_flags &= ~ENTITY_COLLIDABLE;
                player->height = 1;
            }
        } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_TAKEOFF) {
            if (!player->frame_duration) {
                StopPlayer(player);
                gPlayerInfo[earthling->object].behavior_flags |= ENTITY_COLLIDABLE;
                player->x = earthling->x;
                player->y = earthling->y;
                player->height = 35;
                sprite = player->sprite_state + 4;
                player->acceleration_height = -64;
            }
        } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_DESCEND) {
            if (player->height < 22) {
                StopPlayer(player);
                player->height = 22;
                player->draw_flags &= ~ENTITY_SHADOW;
                sprite = player->sprite_state + 4;
                PlaySoundEffect((long)Sfx_Splash, AUDIO_PRIORITY_NORMAL, 0);
            }
        } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_SPLASH) {
            if (!player->frame_duration) {
                sprite = PLAYER_ANIM_HOT_TUB;
                player->height = 0;
                gPlayerJumpCounters[player_number] = 0;
            }
        } else if (player->sprite_state == PLAYER_ANIM_HOT_TUB) {
            if (!player_number) {
                if (gPlayerProcessedInput[PLAYER_TOEJAM] & INPUT_LEFT) {
                    sprite = PLAYER_ANIM_HOT_TUB_EXIT;
                    player->behavior_flags |= ENTITY_MASK_PIXELS;
                } else if (!(GetRandomNumber() & 31)) {
                    player->x -= 20;
                    GenerateSpeech(PLAYER_TOEJAM, "chat", 0);
                    player->x += 20;
                } else if (!(GetRandomNumber() & 31)) {
                    if (NEXT_RANDOM & 1)
                        GenerateSpeech(PLAYER_TOEJAM, "giggle", 1);
                    else
                        GenerateSpeech(PLAYER_TOEJAM, "titter", 1);
                }
            } else if (player_number == PLAYER_EARL) {
                if (gPlayerProcessedInput[PLAYER_EARL] & INPUT_RIGHT) {
                    sprite = PLAYER_ANIM_HOT_TUB_EXIT;
                    player->behavior_flags |= ENTITY_MASK_PIXELS;
                } else if (!(GetRandomNumber() & 31)) {
                    player->x += 20;
                    GenerateSpeech(PLAYER_EARL, "chat", 0);
                    player->x -= 20;
                } else if (gPlayerInfo[PLAYER_TOEJAM].sprite_state != PLAYER_ANIM_HOT_TUB &&
                           !(GetRandomNumber() & 31)) {
                    if (NEXT_RANDOM & 1)
                        GenerateSpeech(PLAYER_EARL, "giggle", 1);
                    else
                        GenerateSpeech(PLAYER_EARL, "titter", 1);
                }
            }
            if (++gPlayerJumpCounters[player_number] > 5)
                ++gPlayerHealth[player_number];
        } else if (player->sprite_state == PLAYER_ANIM_HOT_TUB_EXIT && !player->frame_duration) {
            if (!player_number) {
                sprite = PLAYER_ANIM_FALL_RIGHT;
                player->x -= 55;
                player->height = 42;
            } else {
                sprite = PLAYER_ANIM_FALL_LEFT;
                player->x += 67;
                player->height = 39;
            }
            player->draw_flags |= ENTITY_SHADOW;
            player->behavior_flags &= ~ENTITY_MASK_PIXELS;
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
        }
    }
    if (sprite != PLAYER_ANIM_UNCHANGED)
        SetSprite(player, sprite);
}
