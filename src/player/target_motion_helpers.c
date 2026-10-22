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
    char reserved_30[26], value_4a, sprite_state, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
extern void func_03e4f8();

/* Follow the vertical delta only outside a two-unit dead zone, while always
 * moving horizontally and returning the corresponding facing state. */
char func_01b02e(state, delta_x, delta_y)
struct object_state *state;
short delta_x, delta_y;
{
    if (delta_y < -2)
        state->velocity_y = -256;
    else if (delta_y > 2)
        state->velocity_y = 256;
    else
        state->velocity_y = 0;
    if (delta_x < 0) {
        state->velocity_x = -256;
        return 6;
    } else {
        state->velocity_x = 256;
        return 7;
    }
}

/* Trigger event 9 once when the target has either observed status bit. */
void func_01b078(state, target)
struct object_state *state, *target;
{
    if (!state->flag_57 && (target->flags_52 & 0x40800L)) {
        func_03e4f8(9);
        state->flag_57 = 1;
    }
}
