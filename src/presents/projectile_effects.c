#define TJE_LOCAL_FUNC_01E736
#define TJE_LOCAL_FUNC_00D904
#define TJE_LOCAL_FUNC_00D9DC
#define TJE_LOCAL_FUNC_015DA6

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_map.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this byte-store consumer keeps an int result view of the
 * short allocator. The shared short declaration adds normalization absent
 * from the ROM; the historical missing declaration is [INFERENCE]. */
extern int func_01e736();

/* ORIGINAL BUG: this consumer keeps a int result view of func_00d904,
 * whose canonical return is short. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern int func_00d904();

/* ORIGINAL BUG: this consumer keeps a int result view of func_00d9dc,
 * whose canonical return is unsigned char. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern int func_00d9dc();

/* ORIGINAL BUG: this consumer keeps a int result view of func_015da6,
 * whose canonical return is short. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern int func_015da6();
/* Process all twelve tomato handles, changing landed or collided tomatoes
 * into their impact animation and releasing completed objects. */
void ProcessTomatoRain()
{
    short slot, object_number;
    struct object_state *projectile;
    struct timed_animation *animation;

    for (slot = 0; slot < 12; ++slot) {
        if (object_number = ram_ffe1c0[slot]) {
            projectile = &gPlayerInfo[object_number];
            if (projectile->height < 10 && !(projectile->draw_flags & 6)) {
                func_00d970(object_number);
                ram_ffe1c0[slot] = 0;
            } else if (projectile->sprite_state == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & 0x8000) {
                        projectile->draw_flags &= ~64;
                        object_number = 0;
                        if (projectile->draw_flags & 4)
                            object_number = 1;
                        object_number = func_00d9dc(object_number, projectile->x, projectile->y);
                        if (object_number == 144) {
                            projectile->tile &= 0x7fff;
                            projectile->sprite_ready = 0;
                            projectile->sprite_state = 21;
                        } else {
                            projectile->velocity_x = 0;
                            projectile->velocity_y = 0;
                            projectile->height = 0;
                            projectile->velocity_height = 0;
                            projectile->acceleration_height = 0;
                            if (object_number != 128)
                                animation = &dat_0af022;
                            else
                                animation = &dat_0af02c;
                            projectile->sequence = animation->sequence;
                            projectile->frames = animation->frames;
                            projectile->frame_phase = animation->period;
                            projectile->sprite_state = 0;
                            projectile->frame = -1;
                            projectile->frame_duration = 1;
                            if (!gMusicUnknownE472)
                                func_03e4f8(15);
                        }
                    }
                } else if (projectile->height <= 25) {
                    object_number = func_025e72(projectile, ram_ffe1cc[slot]);
                    if (object_number) {
                        if (--object_number <= 1 && !ELEVATOR_FLAG[object_number] &&
                            !P1_MENU_OPEN[object_number] && !ram_ffe236[object_number]) {
                            if (ram_ffe1cc[slot] <= 1)
                                TOEJAMS_HEALTH[object_number] -= 4;
                            else
                                TOEJAMS_HEALTH[object_number] -= 6;
                        } else if (!func_015da6(object_number))
                            DamageEarthling(gPlayerInfo[object_number].earthling);
                        projectile->velocity_x = 0;
                        projectile->velocity_y = 0;
                        projectile->velocity_height = 0;
                        projectile->acceleration_height = 0;
                        projectile->draw_flags &= ~64;
                        animation = &dat_0af018;
                        projectile->sequence = animation->sequence;
                        projectile->frames = animation->frames;
                        projectile->frame_phase = animation->period;
                        projectile->frame = -1;
                        projectile->frame_duration = 1;
                        projectile->sprite_state = 0;
                        func_03e4f8(2);
                    }
                }
            } else if (!projectile->frame_duration && projectile->sprite_state != 21) {
                func_00d970(object_number);
                ram_ffe1c0[slot] = 0;
            }
        }
    }
    func_01cd9e();
}

/* Choose a counter in [50, 80] and clear its companion byte. */
void SetTomatoRainTomatoes(index)
int index;
{
    gPlayer1TomatoRainTimer[index] = GetRandomNumber() % 31 + 50;
    TOMATO_RAIN_TIMER[index] = 0;
}

/* While a player's rain counter is active, allocate a falling tomato after
 * the inter-tomato delay expires. Allocation failure leaves the count intact. */
void CheckTomatoRainTimer(player_number)
int player_number;
{
    short slot;
    struct object_state *projectile, *player;
    struct animation_descriptor *animation;

    if (!gPlayer1TomatoRainTimer[player_number] || --TOMATO_RAIN_TIMER[player_number] > 0)
        return;
    TOMATO_RAIN_TIMER[player_number] = GetRandomNumber() % 5 + 4;
    slot = 4;
    do {
        if (!ram_ffe1c0[slot])
            break;
        ++slot;
    } while (slot < 12);
    if (slot == 12)
        return;
    if (ram_ffe1c0[slot] = func_00d904()) {
        ram_ffe1cc[slot] = -1;
        player = &gPlayerInfo[player_number];
        projectile = &gPlayerInfo[ram_ffe1c0[slot]];
        projectile->x = GetRandomNumber() % 201 + -100 + player->x;
        projectile->y = GetRandomNumber() % 101 + -50 + player->y;
        projectile->height = 224;
        projectile->velocity_height = -384;
        projectile->acceleration_height = -18;
        projectile->level = player->level;
        projectile->width = 9;
        projectile->height_pixels = 7;
        projectile->sprite_count = 1;
        projectile->tile_count = 16;
        projectile->tile = 0x8000;
        animation = AnimationDescriptors_0AEFF6;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= 64;
        projectile->sprite_state = -1;
        --gPlayer1TomatoRainTimer[player_number];
    }
}

/* Release the first occupied rain/earthling tomato handle, if one exists. */
int func_01c928()
{
    short slot;

    for (slot = 4; slot < 12; ++slot) {
        if (ram_ffe1c0[slot]) {
            func_00d970(ram_ffe1c0[slot]);
            ram_ffe1c0[slot] = 0;
            return 1;
        }
    }
    return 0;
}

/* Allocate one of four projectiles and launch it left or right from the earthling. */
void func_01c96a(earthling, source)
struct earthling_state *earthling;
struct object_state *source;
{
    short slot;
    struct object_state *projectile;
    struct animation_descriptor *animation;

    slot = 0;
    do {
        if (!ram_ffe1d8[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4)
        return;
    if (ram_ffe1d8[slot] = func_00d904()) {
        projectile = &gPlayerInfo[ram_ffe1d8[slot]];
        projectile->x = source->x;
        projectile->y = source->y - 5;
        projectile->height = source->height;
        if (earthling->state == 10) {
            projectile->x -= 22;
            projectile->velocity_x = -896;
            animation = &dat_0aeffe;
        } else {
            projectile->x += 22;
            projectile->velocity_x = 896;
            animation = &dat_0af006;
        }
        projectile->acceleration_height = -32;
        projectile->level = source->level;
        projectile->width = 16;
        projectile->height_pixels = 8;
        projectile->sprite_count = 1;
        projectile->tile_count = 2;
        projectile->tile = 0x8000;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 9;
        projectile->frame = -1;
        projectile->frame_duration = 1;
        projectile->draw_flags |= 64;
        projectile->sprite_state = -1;
        func_03e4f8(4);
    }
}

/* Return one plus the first active player's or decoy's object index in range. */
short func_01ca64(projectile)
struct object_state *projectile;
{
    short pass, index, reach;
    struct object_state *target;

    target = gPlayerInfo;
    for (pass = 0; pass < 2; ++pass) {
        for (index = 0; index <= 1; ++index, ++target) {
            if (pass) {
                if (!ram_ffde58[index])
                    continue;
                target = &gPlayerInfo[ram_ffde58[index]];
            }
            if (!(target->draw_flags & 1) || !(target->flags_4d & 1) ||
                projectile->level != target->level)
                continue;
            reach = projectile->width / 2 + target->width / 2 - 5;
            if (reach > abs(projectile->x - target->x) &&
                abs(projectile->y - target->y) < reach * 3 / 4 &&
                target->height + target->height_pixels - 10 > projectile->height) {
                if (!pass)
                    return index + 1;
                else
                    return ram_ffde58[index] + 1;
            }
        }
    }
    return 0;
}

/* Allocate the player's control-effect object and refresh its timer and mapping. */
void func_01cb86(player_number)
short player_number;
{
    struct object_state *effect, *player;
    struct timed_animation *animation;

    if (!ram_ffe1dc[player_number]) {
        if (ram_ffe1dc[player_number] = func_01e736(0)) {
            effect = &gPlayerInfo[ram_ffe1dc[player_number]];
            player = &gPlayerInfo[player_number];
            effect->flags_4d |= 8;
            effect->parent = player_number;
            effect->level = player->level;
            effect->tile |= 0x8000;
            effect->width = 32;
            effect->height_pixels = 32;
            effect->sprite_count = 1;
            effect->tile_count = 16;
            animation = AnimationDescriptors_0AF00E;
            effect->sequence = animation->sequence;
            effect->frames = animation->frames;
            effect->frame_phase = animation->period;
            effect->frame = -1;
            effect->frame_duration = 1;
        }
    }
    if (ram_ffe1dc[player_number]) {
        ram_ffe1de[player_number] = 240;
        ram_ffe1e2[player_number] = GetRandomNumber() & 3;
    }
}

/* Keep action bits and remap a nonzero direction nibble through the selected row.
 * Callers use the shared unsigned-char return declaration. */
unsigned char func_01cc62(player_number)
short player_number;
{
    unsigned char input, direction;

    input = P1_PROCESSED_INPUT[player_number];
    if (!(input & 15))
        return input;
    direction = dat_08bfe6[ram_ffe1e2[player_number]][(input & 15) - 1];
    return (input & 240) | direction;
}

/* Expire the control-effect object, or occasionally play its animation sound. */
void func_01ccd2(player)
struct object_state *player;
{
    short player_number;
    struct object_state *effect;

    player_number = player->entity_number;
    if (!ram_ffe1dc[player_number])
        return;
    effect = &gPlayerInfo[ram_ffe1dc[player_number]];
    effect->level = player->level;
    if (--ram_ffe1de[player_number] <= 0 || gPlayerLives[player_number] < 0 ||
        player->sprite_state == 5 || player->sprite_state == 6) {
        func_00d970(ram_ffe1dc[player_number]);
        ram_ffe1dc[player_number] = 0;
    } else if (!ELEVATOR_FLAG[player_number] && (effect->frame_duration & 2) &&
               ((short)GetRandomNumber() & 1)) {
        PlaySound(2, 20);
    }
}

/* Update four projectiles, applying player/decoy hits or removing landed objects. */
void func_01cd9e()
{
    short slot, object_number, hit;
    struct object_state *projectile;

    for (slot = 0; slot < 4; ++slot) {
        if (object_number = ram_ffe1d8[slot]) {
            projectile = &gPlayerInfo[object_number];
            if (!(projectile->draw_flags & 6)) {
                if (projectile->height < 5) {
                    func_00d970(object_number);
                    ram_ffe1d8[slot] = 0;
                }
            } else if (projectile->sprite_state == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & 0x8000) {
                        projectile->draw_flags &= ~64;
                        hit = 0;
                        if (projectile->draw_flags & 4)
                            hit = 1;
                        hit = func_00d9dc(hit, projectile->x, projectile->y);
                        if (hit == 0x90) {
                            projectile->tile &= ~0x8000;
                            projectile->sprite_ready = 0;
                        } else {
                            func_00d970(object_number);
                            ram_ffe1d8[slot] = 0;
                        }
                    }
                } else if (projectile->height <= 25) {
                    if (hit = func_01ca64(projectile)) {
                        if (--hit <= 1 && !ELEVATOR_FLAG[hit] && !P1_MENU_OPEN[hit])
                            func_01cb86(hit);
                        else
                            func_015da6(hit);
                        func_00d970(object_number);
                        ram_ffe1d8[slot] = 0;
                    }
                }
            }
        }
    }
}

/* Place an active terrain effect one pixel behind the supplied player. */
void func_01cec2(player, player_number)
struct object_state *player;
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = func_01e736(0)) {
        ram_ffe1e4[player_number] = object_number;
        effect = &gPlayerInfo[object_number];
        effect->x = player->x;
        effect->y = player->y - 1;
        effect->level = player->level;
        effect->tile |= 0x8000;
        effect->width = 24;
        effect->height_pixels = 20;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        animation = AnimationDescriptors_0AF076;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->flags_4d |= 1;
        func_03e4f8(13);
    }
}

/* Replace the previous retiring effect and switch the current one to its final animation. */
void func_01cf66(player_number)
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = ram_ffe1e6[player_number])
        func_00d970(object_number);
    object_number = ram_ffe1e4[player_number];
    ram_ffe1e6[player_number] = object_number;
    ram_ffe1e4[player_number] = 0;
    effect = &gPlayerInfo[object_number];
    animation = &dat_0af080;
    effect->sequence = animation->sequence;
    effect->frames = animation->frames;
    effect->frame_phase = animation->period;
    effect->frame = -1;
    effect->frame_duration = 1;
}

/* Allocate a temporary terrain-effect object at the player position. */
void func_01cfea(player, player_number)
struct object_state *player;
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = func_00d904()) {
        ram_ffe1e6[player_number] = object_number;
        effect = &gPlayerInfo[object_number];
        effect->x = player->x;
        effect->y = player->y;
        effect->level = player->level;
        effect->tile |= 0x8000;
        effect->width = 24;
        effect->height_pixels = 20;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        animation = &dat_0af08a;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->flags_4d |= 1;
        func_03e4f8(13);
    }
}

/* Spawn terrain effects on a fresh action press and apply nearby effect damage. */
void func_01d084(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    short player_number;
    char object_number;
    struct object_state *effect;
    struct earthling_state source;

    player_number = player->entity_number;
    if (gPlayer1Powerup[player_number] != 6 || ELEVATOR_FLAG[player_number] ||
        player->sprite_state == 5 || player->sprite_state == 6) {
        if (ram_ffe1e4[player_number])
            func_01cf66(player_number);
    } else if (!(P1_PREV_PROCESSED_INPUT[player_number] & 64) &&
               (P1_PROCESSED_INPUT[player_number] & 64)) {
        if (terrain == 0x10 || terrain == 0x50) {
            if (ram_ffe1e4[player_number])
                func_01cf66(player_number);
            func_01cec2(player, player_number);
        } else if (terrain < 0x80 && !ram_ffe1e6[player_number]) {
            func_01cfea(player, player_number);
        }
    } else if (object_number = ram_ffe1e4[player_number]) {
        effect = &gPlayerInfo[object_number];
        if (!effect->frame_duration && (effect->draw_flags & 6)) {
            source.kind = 1;
            source.x = effect->x;
            source.y = effect->y;
            source.object = object_number;
            if (!ELEVATOR_FLAG[player_number] && func_00dd1e(effect, player, 25, 10))
                Player_Youch(player_number, &source);
            if (!ELEVATOR_FLAG[player_number ^ 1] &&
                func_00dd1e(effect, &gPlayerInfo[player_number ^ 1], 25, 10))
                Player_Youch(player_number ^ 1, &source);
        }
    }
    if (object_number = ram_ffe1e6[player_number]) {
        effect = &gPlayerInfo[object_number];
        if (!effect->frame_duration) {
            func_00d970(object_number);
            ram_ffe1e6[player_number] = 0;
        }
    }
}

/* Shoot at frame two; on animation completion restore the matching facing state. */
void func_01d288(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    short next_state;

    if (object->frame == 2) {
        ++object->frame;
        func_01c96a(earthling, object);
    } else if (!object->frame_duration) {
        if (earthling->state == 10)
            next_state = 0;
        else
            next_state = 1;
        SetEarthlingState(earthling, next_state);
    }
}

/* During the early firing frames, pick an eligible player or active decoy ahead. */
void func_01d2cc(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    register short player_number;
    short decoy, attempt, distance;
    struct object_state *target;

    if (object->frame < 16 && (object->frame & 3) == 2) {
        ++object->frame;
        player_number = GetRandomNumber() & 1;
        for (attempt = 0; attempt < 2; player_number ^= 1, ++attempt) {
            decoy = ram_ffde58[player_number];
            target = decoy && (gPlayerInfo[decoy].draw_flags & 6) ?
                     &gPlayerInfo[decoy] : &gPlayerInfo[player_number];
            if (gPlayerLives[player_number] < 0)
                continue;
            if (earthling->state == 8) {
                if ((distance = earthling->y - target->y) > 40 && distance < 200)
                    break;
            } else if (earthling->state == 9) {
                if ((distance = target->y - earthling->y) > 40 && distance < 200)
                    break;
            } else if (earthling->state == 10) {
                if ((distance = earthling->x - target->x) > 50 && distance < 320)
                    break;
            } else {
                if ((distance = target->x - earthling->x) > 50 && distance < 320)
                    break;
            }
        }
        if (attempt != 2)
            func_01c2a2(target, earthling);
    } else if (!object->frame_duration) {
        SetEarthlingState(earthling, 0);
        PlaySoundEffect((long)Sfx_ChickenCluck, 50, 0);
    }
}
