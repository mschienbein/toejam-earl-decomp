#define TJE_LOCAL_FUNC_01E736
#define TJE_LOCAL_FUNC_00D904

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_audio.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_earthlings.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* ORIGINAL BUG: this byte-store consumer keeps an int result view of the
 * short allocator. The shared short declaration adds normalization absent
 * from the ROM; the historical missing declaration is [INFERENCE]. */
extern int func_01e736();

/* Interaction choices use one display object and one result byte per player. */
/* ORIGINAL BUG: this caller consumes an int result from the short-returning
 * allocator. Declaring short adds a MOVE.W D0,D0 absent from the ROM. */
extern int func_00d904();

/* Allocate the two-choice display, copying the player's current level. */
void func_020f6c(player_number)
short player_number;
{
    struct object_state *display;
    struct animation_descriptor *animation;

    ram_ffe234[player_number] = 0;
    if (ram_ffe232[player_number] = func_01e736(0)) {
        display = &gPlayerInfo[ram_ffe232[player_number]];
        display->level = (&gPlayerInfo[player_number])->level;
        display->flags_4d |= 8;
        display->parent = player_number;
        display->flag_58 = 10;
        display->offset_height = 10;
        display->tile |= 0x8000;
        display->width = 32;
        display->height_pixels = 32;
        display->sprite_count = 1;
        display->tile_count = 16;
        animation = AnimationDescriptors_0AF06E;
        display->sequence = animation->sequence;
        display->frames = animation->frames;
        display->frame = -1;
        display->frame_duration = 1;
        ram_ffe236[player_number] = 1;
    }
}

/* Resolve A-button choices, cancelling while the player is in sprite 5 or 6. */
void CheckAButton()
{
    short player_number;
    struct object_state *display;
    char choice, cancel;

    for (player_number = 0; player_number <= 1; ++player_number) {
        if (ram_ffe236[player_number]) {
            display = &gPlayerInfo[ram_ffe232[player_number]];
            choice = display->sprite_state;
            cancel = (&gPlayerInfo[player_number])->sprite_state == 5 ||
                     (&gPlayerInfo[player_number])->sprite_state == 6;
            if (cancel || ((P1_PROCESSED_INPUT[player_number] & 64) &&
                           !(P1_PREV_PROCESSED_INPUT[player_number] & 64))) {
                if (cancel)
                    ram_ffe234[player_number] = 0;
                else if (display->sprite_state == 1)
                    ram_ffe234[player_number] = 1;
                else
                    ram_ffe234[player_number] = 2;
                func_00d970(ram_ffe232[player_number]);
                ram_ffe232[player_number] = 0;
                ram_ffe236[player_number] = 0;
                P1_PREV_PROCESSED_INPUT[player_number] = 64;
                P1_PROCESSED_INPUT[player_number] = 64;
                ram_ffe238[player_number] = 1;
            } else {
                if (P1_PROCESSED_INPUT[player_number] & 1)
                    choice = 0;
                else if (P1_PROCESSED_INPUT[player_number] & 2)
                    choice = 1;
                if (choice != display->sprite_state) {
                    display->frame = choice - 1;
                    display->frame_duration = 1;
                    display->sprite_state = choice;
                }
                P1_PROCESSED_INPUT[player_number] = 0;
            }
        }
    }
}

/* A mailbox offers mail order until activated, then checks its biting animation. */
void AI_MailboxTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct special_earthling_state *mailbox;
    struct object_state *player, *object;

    if (ram_ffe236[player_number])
        return;
    if (ram_ffe226[0].level == earthling->level)
        mailbox = ram_ffe226;
    else
        mailbox = &ram_ffe226[1];
    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (mailbox->mode != 1) {
        if ((!player->sprite_state || player->sprite_state == 4 ||
             (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
             (player->sprite_state == 57 && !player->velocity_y)) &&
            func_00dd1e(object, player, 30, 15)) {
            if (!mailbox->contact[player_number]) {
                GenerateSpeech(earthling->object, "Mail Order?", 0);
                func_020f6c(player_number);
                mailbox->contact[player_number] = 1;
            } else if (mailbox->contact[player_number] == 1) {
                if (ram_ffe234[player_number] == 2)
                    func_0098f6(player_number);
                mailbox->contact[player_number] = 2;
                ram_ffe234[player_number] = 0;
            }
        } else
            mailbox->contact[player_number] = 0;
        return;
    }
    if ((earthling->state & 252) == 4) {
        if (func_00dd1e(object, player, 20, 10))
            Player_Youch(player_number, earthling);
        if (!--mailbox->timer) {
            SetEarthlingState(earthling, 0);
            object->frame = -1;
            mailbox->timer = 175;
        } else if (!TOEJAM_SPEECH_COUNTER) {
            PlaySoundEffect((long)Sfx_MailboxRoar, 50, 0);
            earthling->value_0a = 1;
        }
    } else if (!earthling->state) {
        if (mailbox->timer)
            --mailbox->timer;
        else if (player->sprite_state != 5 && player->sprite_state != 6 &&
                 func_00dd1e(object, player, 100, 40))
            SetEarthlingState(earthling, 3);
        else if ((GetRandomNumber() & 255) == 1)
            SetEarthlingState(earthling, 1);
    } else if (earthling->state == 1) {
        if (!object->frame_duration) {
            SetEarthlingState(earthling, 0);
            object->frame = -1;
        }
    } else if (earthling->state == 3) {
        if (!object->frame_duration) {
            SetEarthlingState(earthling, 4);
            earthling->timer = 0;
            mailbox->timer = 33;
            earthling->value_0a = 0;
        }
    }
}

/* The wizard charges one buck and advances health one unit at a time. */
void AI_WizardTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == 4 ||
           (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
           player->sprite_state == 71 ||
           (player->sprite_state == 57 && !player->velocity_y)) &&
          func_00dd1e(object, player, 35, 15))) {
        ram_ffe23a[player_number] = 0;
        return;
    }
    if (ram_ffe23a[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (ram_ffe236[player_number] ||
        (gPlayerInfo[0].level == gPlayerInfo[1].level &&
         ram_ffe23a[player_number ^ 1] && ram_ffe23a[player_number ^ 1] != 2))
        return;
    if (!ram_ffe23a[player_number]) {
        if ((earthling->state == 4 || earthling->state == 5) &&
            !object->frame_duration)
            earthling->state += 2;
        if (earthling->state >= 6)
            next_state = func_0196b0(earthling);
        else if (earthling->state <= 1 && object->frame > 4) {
            GenerateSpeech(earthling->object, "Got a buck?", 0);
            func_020f6c(player_number);
            ram_ffe23a[player_number] = 1;
        }
    } else if (!object->frame_duration) {
        if (ram_ffe23a[player_number] == 1) {
            if (ram_ffe234[player_number] == 2) {
                if (gPlayerBucks[player_number] > 0) {
                    next_state = 2;
                    ram_ffe23a[player_number] = 3;
                    --gPlayerBucks[player_number];
                    SetSprite(player, 71);
                    PlaySoundEffect((long)Sfx_Hallelujah, 92, 1);
                } else
                    PlaySound(26, 50);
            } else {
                ram_ffe23a[player_number] = 2;
                next_state = 5;
                object->velocity_x = 320;
            }
            ram_ffe234[player_number] = 0;
        } else if (ram_ffe23a[player_number] == 3) {
            if (TOEJAMS_HEALTH[player_number] < func_00ae90(player_number))
                ++TOEJAMS_HEALTH[player_number];
            else {
                ram_ffe23a[player_number] = 2;
                next_state = 3;
                SetSprite(player, GetActivePresentSprite(player));
            }
        } else if (earthling->state == 3) {
            next_state = 5;
            object->velocity_x = 320;
        }
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* The wiseman opens the identification menu after a positive response. */
void AI_WisemanTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == 4 ||
           (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
           (player->sprite_state == 57 && !player->velocity_y)) &&
          func_00dd1e(object, player, 35, 15))) {
        ram_ffe23c[player_number] = 0;
        return;
    }
    if (ram_ffe23c[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (ram_ffe236[player_number] ||
        (gPlayerInfo[0].level == gPlayerInfo[1].level &&
         ram_ffe23c[player_number ^ 1] && ram_ffe23c[player_number ^ 1] != 2))
        return;
    if (!ram_ffe23c[player_number]) {
        if (earthling->state >= 2)
            next_state = func_0196b0(earthling);
        else if (earthling->state <= 1) {
            GenerateSpeech(earthling->object, "Got 2 bucks?", 0);
            func_020f6c(player_number);
            ram_ffe23c[player_number] = 1;
        }
    } else if (ram_ffe23c[player_number] == 1) {
        if (ram_ffe234[player_number] == 2) {
            PlaySoundEffect((long)Sfx_WisemanThink, 64, 1);
            func_009934(player_number);
        }
        ram_ffe23c[player_number] = 3;
        ram_ffe234[player_number] = 0;
    } else if (ram_ffe23c[player_number] == 3 && !P1_MENU_OPEN[player_number]) {
        ram_ffe23c[player_number] = 2;
        next_state = 2;
        object->velocity_x = -320;
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* Three bucks starts the singer's animation and the nearby-earthling ability. */
void AI_OperaSingerTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == 4 ||
           (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
           player->sprite_state == 69 ||
           (player->sprite_state == 57 && !player->velocity_y)) &&
          func_00dd1e(object, player, 35, 15))) {
        ram_ffe23e[player_number] = 0;
        return;
    }
    if (ram_ffe23e[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (ram_ffe236[player_number] ||
        (gPlayerInfo[0].level == gPlayerInfo[1].level &&
         ram_ffe23e[player_number ^ 1] && ram_ffe23e[player_number ^ 1] != 2))
        return;
    if (!ram_ffe23e[player_number]) {
        if (earthling->state >= 4)
            next_state = 0;
        else if (earthling->state <= 1) {
            GenerateSpeech(earthling->object, "Got 3 bucks?", 0);
            func_020f6c(player_number);
            ram_ffe23e[player_number] = 1;
        }
    } else if (ram_ffe23e[player_number] == 1) {
        if (ram_ffe234[player_number] == 2) {
            if (gPlayerBucks[player_number] >= 3) {
                next_state = 3;
                ram_ffe23e[player_number] = 3;
                gPlayerBucks[player_number] -= 3;
                SetSprite(player, 69);
                player_number ^= 1;
                player = &gPlayerInfo[player_number];
                if (gPlayerLives[player_number] >= 0 && !SPLIT_SCREEN &&
                    (!player->sprite_state || player->sprite_state == 4 ||
                     (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
                     (player->flags_52 & 0x830400c)))
                    SetSprite(player, 69);
                PlaySoundEffect((long)Sfx_OperaSing, 90, 1);
            } else
                PlaySound(26, 50);
        } else {
            ram_ffe23e[player_number] = 2;
            next_state = 0;
        }
        /* The index is the other player after a successful paid response. */
        ram_ffe234[player_number] = 0;
    } else if (ram_ffe23e[player_number] == 3) {
        if (!object->frame_duration) {
            ram_ffe23e[player_number] = 2;
            next_state = 0;
            SetSprite(player, GetActivePresentSprite(player));
            player_number ^= 1;
            player = &gPlayerInfo[player_number];
            if (gPlayerLives[player_number] >= 0 && !SPLIT_SCREEN &&
                player->sprite_state == 69)
                SetSprite(player, GetActivePresentSprite(player));
        } else
            DoVikingAbility(player->draw_flags & 6);
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* These earthling states preserve the last hit point in DamageEarthling. */
int NiceEarthlingCheck(earthling)
struct earthling_state *earthling;
{
    if (earthling->kind == 17) {
        if (earthling->state == 3)
            return 1;
    } else if (earthling->kind == 10) {
        if (earthling->state == 2)
            return 1;
    } else if (earthling->kind == 11 && earthling->state <= 1 &&
               (ram_ffe23c[0] == 3 || ram_ffe23d == 3))
        return 1;
    return 0;
}

/* Place a player animation at camera offset (80,224), retaining draw mask 2. */
int func_021c60(player_number)
short player_number;
{
    struct object_state *object;
    struct timed_animation *animation;

    if (ram_ffe242 = func_00d904()) {
        object = &gPlayerInfo[ram_ffe242];
        object->x = TOEJAM_CAMERA_NEWX + 80;
        object->y = TOEJAM_CAMERA_NEWY + 224;
        object->height = 112;
        object->level = gLevel912C;
        object->width = 32;
        object->height_pixels = 48;
        object->sprite_count = 4;
        object->tile_count = 40;
        object->tile |= 0x8000;
        if (!player_number)
            animation = &PlayerAnimationDescriptors[5];
        else
            animation = &rom_0ae9de[5];
        object->sequence = animation->sequence;
        object->frames = animation->frames;
        object->frame_phase = animation->period;
        object->frame_duration = 1;
        object->frame = -1;
        func_00beb2(ram_ffe242);
        if (object->draw_flags & 2)
            return 0;
        func_00d970(ram_ffe242);
    }
    return 1;
}

/* An eliminated player may ask the other player for a life. */
void func_021d32(player_number)
short player_number;
{
    short other_player;
    struct object_state *player;

    if (gLevel912C == 26)
        return;
    other_player = player_number ^ 1;
    player = &gPlayerInfo[other_player];
    if (!ram_ffe242 && gPlayerLives[other_player] > 0 &&
        !ram_ffe236[other_player] && !ELEVATOR_FLAG[other_player] &&
        (!player->sprite_state || player->sprite_state == 4 ||
         (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
         (player->flags_52 & 0x400c)) &&
        !(player->flags_52 & 0x20000000) &&
        (MovePadInputIntoD0(player_number) & 112) &&
        !func_021c60(player_number)) {
        SetSprite(player, GetActivePresentSprite(player));
        player = &gPlayerInfo[ram_ffe242];
        player->height += 30;
        func_01e012(64);
        if (PLAYER_INDICATOR != 2)
            GenerateSpeech(ram_ffe242, "Can I play?", 0);
        else
            GenerateSpeech(ram_ffe242, "Got a life?", 0);
        player->height -= 30;
        func_020f6c(other_player);
        ram_ffe240 = 1;
    }
    if (ram_ffe240) {
        if (!ram_ffe234[other_player])
            return;
        func_00d970(ram_ffe242);
        ram_ffe242 = 0;
        if (ram_ffe234[other_player] == 2) {
            while (VBLANK_FLAG_2)
                ;
            ClearSkipVHBlankFlags();
            gMode2RegSoftcopy &= 0x3c;
            VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
            if (PLAYER_INDICATOR == 2) {
                --gPlayerLives[other_player];
                func_00b3e4(other_player);
            }
            CopyLevelBetweenPlayers(0);
            gLevel912E = gLevel912C;
            player = &gPlayerInfo[player_number];
            player->draw_flags = 1;
            player->flags_4d = 1;
            player->sprite_state = 0;
            func_00baa2(player_number);
            StopPlayer(player);
            player->mode_15 = 0;
            PLAYER_INDICATOR = 2;
            SPLIT_SCREEN = 1;
            func_010ab6(player_number);
        }
        ram_ffe240 = 0;
        ram_ffe234[other_player] = 0;
    }
}
