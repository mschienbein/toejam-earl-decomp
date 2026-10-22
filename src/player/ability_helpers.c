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
void func_023c6e(item)
struct level_item *item;
{
    item->kind = 82;
    item->level = 0;
    item->x = 1464;
    item->y = 854;
    ++item;
    item->kind = 82;
    item->level = 0;
    item->x = 1536;
    item->y = 928;
    ++item;
    item->kind = 82;
    item->level = 0;
    item->x = 1336;
    item->y = 1110;
}

/* Supply the fixed level-zero positions for these earthling kinds. */
void func_023cb4(kind, x, y)
short kind;
short *x, *y;
{
    switch (kind) {
    case 23:
        *x = 1536;
        *y = 1024;
        break;
    case 24:
        *x = 1398;
        *y = 980;
        break;
    case 5:
        *x = 1536;
        *y = 944;
        break;
    }
}

/* Finish the lemonade animation, burp, and open present 17 after speech ends. */
void LemonadeAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if (earthling->state == 1 &&
        !gPlayerInfo[earthling->object].frame_duration)
        SetEarthlingState(earthling, 0);
    if (ram_ffe396[player_number] == 1 && !TOEJAM_SPEECH_COUNTER) {
        OpenSelectedPresent(player_number, 17);
        ram_ffe396[player_number] = 2;
    }
    if (player->sprite_state == 118 && !player->frame_duration) {
        SetSprite(player, GetActivePresentSprite(player));
        GenerateSpeech(player_number, "Burp", 1);
        PlaySoundEffect((long)Sfx_Burp, 50, 0);
        if (!ram_ffe396[player_number])
            ram_ffe396[player_number] = 1;
    } else if (player->sprite_state == 8 && player->y > earthling->y) {
        SetSprite(player, 118);
        SetEarthlingState(earthling, 1);
    }
}
