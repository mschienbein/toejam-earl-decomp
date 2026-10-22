struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer;
};
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
extern struct object_state gPlayerInfo[40];
extern unsigned short PREGEN_RANDOMS[59], ram_ff91e0;
extern int func_025f42();
extern void func_015da6();

/* [INFERENCE] Return -1 after consuming a type-18 collision, otherwise choose
 * whether to move from state 6/7 or the low bit of the next random value. */
char func_0196b0(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    short collision;

    if (earthling->reserved == 18 && earthling->object &&
        (object = &gPlayerInfo[earthling->object],
         collision = func_025f42(object, object->x, object->y, 0))) {
        func_015da6(collision - 1);
        return -1;
    }
    if (earthling->state == 6)
        return 0;
    if (earthling->state == 7)
        return 1;
    if (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
        (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
        return 0;
    return 1;
}
