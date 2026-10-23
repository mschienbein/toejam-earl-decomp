#include "player_animation.h"
#include "present_types.h"
#include "earthling_types.h"
#include "entities.h"
#include "earthlings.h"
#include "presents.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "speech.h"
#include "sprite_calls.h"

/* Place the three level-zero items at their fixed coordinates. */
void InitializeLevelZeroFixedItems(item)
struct level_item *item;
{
    item->kind = ITEM_LARGE_UNIDENTIFIED;
    item->level = 0;
    item->x = 1464;
    item->y = 854;
    ++item;
    item->kind = ITEM_LARGE_UNIDENTIFIED;
    item->level = 0;
    item->x = 1536;
    item->y = 928;
    ++item;
    item->kind = ITEM_LARGE_UNIDENTIFIED;
    item->level = 0;
    item->x = 1336;
    item->y = 1110;
}

/* Supply the fixed level-zero positions for these earthling kinds. */
void GetLevelZeroEarthlingPosition(kind, x, y)
short kind;
short *x, *y;
{
    switch (kind) {
    case EARTHLING_LEMONADE_STAND:
        *x = 1536;
        *y = 1024;
        break;
    case EARTHLING_HOT_TUB:
        *x = 1398;
        *y = 980;
        break;
    case EARTHLING_HULA_DANCER:
        *x = 1536;
        *y = 944;
        break;
    }
}

/* Finish the lemonade animation, burp, and grant an extra life after speech ends. */
void LemonadeAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if (earthling->state == 1 &&
        !gPlayerInfo[earthling->object].frame_duration)
        SetEarthlingState(earthling, 0);
    if (gPlayerLemonadeRewardStates[player_number] == 1 && !gPcmEffectCooldownTicks) {
        OpenSelectedPresent(player_number, PRESENT_EXTRA_LIFE);
        gPlayerLemonadeRewardStates[player_number] = 2;
    }
    if (player->sprite_state == PLAYER_ANIM_DRINK_LEMONADE && !player->frame_duration) {
        SetSprite(player, GetActivePresentSprite(player));
        GenerateSpeech(player_number, "Burp", 1);
        PlaySoundEffect((long)Sfx_Burp, AUDIO_PRIORITY_NORMAL, 0);
        if (!gPlayerLemonadeRewardStates[player_number])
            gPlayerLemonadeRewardStates[player_number] = 1;
    } else if (player->sprite_state == PLAYER_ANIM_WALK_UP && player->y > earthling->y) {
        SetSprite(player, PLAYER_ANIM_DRINK_LEMONADE);
        SetEarthlingState(earthling, 1);
    }
}
