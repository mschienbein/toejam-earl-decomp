/* tu33 after the resistant opening function: tomato and other projectile effects. */
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer, x, y;
    char reserved_0a[5], second_object, reserved_10[2];
};
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4], frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[26], sprite_reset, sprite, level;
    unsigned char flags_4d;
    struct earthling_state *earthling;
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct animation_descriptor {
    char *sequence;
    char **frames;
};
struct timed_animation {
    char *sequence;
    char **frames;
    char period, spare;
};
extern struct object_state gPlayerInfo[40];
extern struct animation_descriptor AnimationDescriptors_0AEFF6[];
extern struct animation_descriptor dat_0aeffe, dat_0af006;
extern struct timed_animation dat_0af018, dat_0af022, dat_0af02c;
extern struct timed_animation AnimationDescriptors_0AF00E[];
extern struct timed_animation AnimationDescriptors_0AF076[], dat_0af080, dat_0af08a;
extern unsigned char dat_08bfe6[4][10];
extern char ram_ffe1c0[12], ram_ffe1cc[12], ram_ffe1d8[4];
extern char ram_ffe1dc[], ram_ffe1e2[], ram_ffe1e4[], ram_ffe1e6[], ram_ffde58[];
extern short ram_ffe1de[];
extern char gPlayer1TomatoRainTimer[2], TOMATO_RAIN_TIMER[2];
extern char gPlayerLives[], ELEVATOR_FLAG[], P1_MENU_OPEN[], gPlayer1Powerup[];
extern char ram_ffe236[], TOEJAMS_HEALTH[];
extern unsigned char P1_PROCESSED_INPUT[], P1_PREV_PROCESSED_INPUT[];
extern short gMusicUnknownE472;
extern char Sfx_ChickenCluck[];
extern unsigned short GetRandomNumber();
/* ORIGINAL BUG: these callsites consume int results although accepted definitions
 * return short (allocator), unsigned char (terrain), and short (decoy hit).
 * The allocator result is stored as a byte; terrain is read as a word; the decoy
 * result is tested as a long. [INFERENCE] Original declarations were missing. */
extern int func_00d904(), func_00d9dc(), func_015da6();
extern int func_025e72(), func_01e736(), abs(), func_00dd1e(), Player_Youch();
extern void func_00d970(), func_03e4f8(), DamageEarthling(), PlaySound();
extern void PlaySoundEffect(), SetEarthlingState(), func_01c2a2(), func_01cd9e();

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
            } else if (projectile->sprite == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & 0x8000) {
                        projectile->draw_flags &= ~64;
                        object_number = 0;
                        if (projectile->draw_flags & 4)
                            object_number = 1;
                        object_number = func_00d9dc(object_number, projectile->x, projectile->y);
                        if (object_number == 144) {
                            projectile->tile &= 0x7fff;
                            projectile->sprite_reset = 0;
                            projectile->sprite = 21;
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
                            projectile->sprite = 0;
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
                        projectile->sprite = 0;
                        func_03e4f8(2);
                    }
                }
            } else if (!projectile->frame_duration && projectile->sprite != 21) {
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
        projectile->draw_attribute = 1;
        projectile->draw_offset = 16;
        projectile->tile = 0x8000;
        animation = AnimationDescriptors_0AEFF6;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= 64;
        projectile->sprite = -1;
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
        projectile->draw_attribute = 1;
        projectile->draw_offset = 2;
        projectile->tile = 0x8000;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 9;
        projectile->frame = -1;
        projectile->frame_duration = 1;
        projectile->draw_flags |= 64;
        projectile->sprite = -1;
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
            effect->reserved_56 = player_number;
            effect->level = player->level;
            effect->tile |= 0x8000;
            effect->width = 32;
            effect->height_pixels = 32;
            effect->draw_attribute = 1;
            effect->draw_offset = 16;
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
 * ORIGINAL BUG: accepted jump_motion.c and rocket_motion.c callers declare this
 * as int, although the definition returns unsigned char; they retain the low byte. */
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

    player_number = player->value_5e;
    if (!ram_ffe1dc[player_number])
        return;
    effect = &gPlayerInfo[ram_ffe1dc[player_number]];
    effect->level = player->level;
    if (--ram_ffe1de[player_number] <= 0 || gPlayerLives[player_number] < 0 ||
        player->sprite == 5 || player->sprite == 6) {
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
            } else if (projectile->sprite == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & 0x8000) {
                        projectile->draw_flags &= ~64;
                        hit = 0;
                        if (projectile->draw_flags & 4)
                            hit = 1;
                        hit = func_00d9dc(hit, projectile->x, projectile->y);
                        if (hit == 0x90) {
                            projectile->tile &= ~0x8000;
                            projectile->sprite_reset = 0;
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
        effect->draw_attribute = 1;
        effect->draw_offset = 16;
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
        effect->draw_attribute = 1;
        effect->draw_offset = 16;
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

    player_number = player->value_5e;
    if (gPlayer1Powerup[player_number] != 6 || ELEVATOR_FLAG[player_number] ||
        player->sprite == 5 || player->sprite == 6) {
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
            source.reserved = 1;
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
