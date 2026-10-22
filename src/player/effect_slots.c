struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct timed_animation {
    char *sequence;
    char **frames;
    char period, spare;
};
extern struct object_state gPlayerInfo[40];
extern struct timed_animation dat_0af036;
extern char ram_ffe1be[];
/* ORIGINAL BUG: the allocator is consumed as int, although its accepted
 * definition returns short. Only the low byte of the result is retained. */
extern int func_00d904();
extern void PlaySound(), func_00d970();

/* Report whether either of the two effect slots is available. */
int func_01bdd0()
{
    if (!ram_ffe1be[0] || !ram_ffe1be[1])
        return 1;
    else
        return 0;
}

/* Claim the first available effect slot and start its animation. */
int func_01bde8(x, y, level)
short x, y, level;
{
    short effect_slot;
    struct object_state *effect;
    struct timed_animation *animation;

    if (!ram_ffe1be[0])
        effect_slot = 0;
    else if (!ram_ffe1be[1])
        effect_slot = 1;
    else
        return 1;
    if (ram_ffe1be[effect_slot] = func_00d904()) {
        effect = &gPlayerInfo[ram_ffe1be[effect_slot]];
        effect->x = x;
        effect->y = y;
        effect->height = 5;
        effect->level = level;
        effect->width = 32;
        effect->height_pixels = 32;
        effect->draw_attribute = 1;
        effect->draw_offset = 16;
        effect->tile |= 0x8000;
        animation = &dat_0af036;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame_duration = 1;
        effect->frame = -1;
        PlaySound(14, 50);
        return 0;
    }
    return 1;
}

/* Release a slot after its object's animation has stopped. */
void func_01bea6(effect_slot)
short effect_slot;
{
    if (ram_ffe1be[effect_slot] &&
        !gPlayerInfo[ram_ffe1be[effect_slot]].frame_duration) {
        func_00d970(ram_ffe1be[effect_slot]);
        ram_ffe1be[effect_slot] = 0;
    }
}
