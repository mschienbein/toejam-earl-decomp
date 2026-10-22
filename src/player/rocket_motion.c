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
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
extern unsigned char P1_PROCESSED_INPUT[], P1_PREV_PROCESSED_INPUT[];
extern char ram_ffda30[], ram_ffda3a[], TOEJAM_FALL_STATE[], ELEVATOR_FLAG[], ram_ffe1dc[];
extern unsigned short ram_ff91e0, PREGEN_RANDOMS[59];
extern int abs();
extern int func_01cc62();
extern void StopPlayer(), SetSprite();

/* Choose a direction from velocity, then select standing, jumping, or
 * braking sprites according to the terrain and the active movement flags. */
void func_011462(state, player, terrain)
struct object_state *state;
short player;
unsigned short terrain;
{
    char sprite = -1;
    short velocity_x = state->velocity_x;
    short velocity_y = state->velocity_y;
    char direction = state->sprite & 3;
    char jump;

    if (velocity_x > 0 && direction != 3)
        direction = 3;
    else if (velocity_x < 0 && direction != 2)
        direction = 2;
    else if (velocity_y > 0 && direction != 1)
        direction = 1;
    else if (velocity_y < 0 && direction != 0)
        direction = 0;
    jump = P1_PROCESSED_INPUT[player] & 0x40;
    if (jump && terrain <= 0x50 && (state->flags_52 & 0x800000))
        sprite = direction + 96;
    else if (!jump && (state->flags_52 & 0x1000000)) {
        state->velocity_height = 0x400;
        state->acceleration_height = -64;
        sprite = direction + 104;
    } else if (((terrain == 0x80 || terrain == 0x90) &&
                !(state->flags_52 & 0x6000000)) ||
               (terrain != 0x80 && (state->flags_52 & 0x2000000))) {
        state->velocity_height = 0x200;
        state->acceleration_height = -64;
        sprite = direction + 104;
    } else if ((state->flags_52 & 0x4000000) && state->height <= 0) {
        if (terrain <= 0x50)
            sprite = direction + 92;
        else if (terrain == 0x80)
            sprite = direction + 100;
        else {
            StopPlayer(state);
            state->acceleration_height = -32;
            sprite = (state->sprite & 3) + 20;
            ram_ffda30[player] = 2;
        }
    } else if (direction != (state->sprite & (unsigned long)3))
        /* The unsigned long mask reproduces the ROM's register order; a plain 3 keeps the size
         * but swaps scratch registers ([INFERENCE] the original spelling is unknown). */
        sprite = (state->sprite & 0xfc) + direction;
    if (sprite != -1)
        SetSprite(state, sprite);
}

/* Resolve directional input against the current facing, accelerate on one
 * axis, and retain the original asymmetric x-then-y speed clamping. */
void func_0115da(state, player, changed_direction, input)
struct object_state *state;
short player;
char changed_direction;
unsigned char input;
{
    char up, down, left, right;
    short velocity_x, velocity_y;
    char direction;

    velocity_x = state->velocity_x;
    velocity_y = state->velocity_y;
    if (!(state->flags_52 & 0x4000000) || (state->draw_flags & 0x30)) {
        up = input & 1;
        down = input & 2;
        left = input & 4;
        right = input & 8;
        direction = state->sprite & 3;
        if ((up && right) || (up && left) ||
            (down && right) || (down && left)) {
            if (!changed_direction)
                goto clamp_speed;
            if (up && direction == 0)
                up = 0;
            else if (down && direction == 1)
                down = 0;
            else if (left && direction == 2)
                left = 0;
            else if (right && direction == 3)
                right = 0;
        }
        if (!(input & 15)) {
            if (direction == 0)
                up = 1;
            else if (direction == 1)
                down = 1;
            else if (direction == 2)
                left = 1;
            else if (direction == 3)
                right = 1;
        } else if (up && direction == 1) {
            up = 0;
            ram_ffda3a[player] = 3;
            if (!(left | right))
                right = 1;
        } else if (down && direction == 0) {
            down = 0;
            ram_ffda3a[player] = 3;
            if (!(left | right))
                left = 1;
        } else if (left && direction == 3) {
            left = 0;
            ram_ffda3a[player] = 3;
            if (!(up | down))
                up = 1;
        } else if (right && direction == 2) {
            right = 0;
            ram_ffda3a[player] = 3;
            if (!(up | down))
                down = 1;
        }
        if (left | right) {
            velocity_y = 0;
            state->acceleration_y = 0;
            if (left) {
                state->acceleration_x = -128;
                if (velocity_x > -0x300)
                    velocity_x = -0x300;
            } else {
                state->acceleration_x = 128;
                if (velocity_x < 0x300)
                    velocity_x = 0x300;
            }
        } else {
            velocity_x = 0;
            state->acceleration_x = 0;
            if (up) {
                state->acceleration_y = -128;
                if (velocity_y > -0x240)
                    velocity_y = -0x240;
            } else {
                state->acceleration_y = 128;
                if (velocity_y < 0x240)
                    velocity_y = 0x240;
            }
        }
    }
clamp_speed:
    /* ORIGINAL BUG: An excessive x speed skips the y-speed clamp. */
    if (abs(velocity_x) > 0x600) {
        if (velocity_x > 0)
            velocity_x = 0x600;
        else
            velocity_x = -0x600;
    } else if (abs(velocity_y) > 0x480) {
        if (velocity_y > 0)
            velocity_y = 0x480;
        else
            velocity_y = -0x480;
    }
    state->velocity_x = velocity_x;
    state->velocity_y = velocity_y;
}

/* Handle the active rocket-movement flags, direction-change delay, and
 * collision-flag-driven random turns before updating acceleration and sprite. */
void func_01189c(state, terrain)
struct object_state *state;
unsigned short terrain;
{
    char changed_direction = 0;
    short player;
    unsigned char input;

    player = state->value_5e;
    if (TOEJAM_FALL_STATE[player] || !(state->flags_52 & 0x7800000) ||
        ELEVATOR_FLAG[player])
        return;
    if ((state->velocity_height < 0 && state->height <= 0) ||
        (state->velocity_height > 0 && !state->acceleration_height && state->height >= 0)) {
        state->height = 0;
        state->velocity_height = 0;
        state->acceleration_height = 0;
        state->draw_flags |= 0x40;
    }
    if ((P1_PROCESSED_INPUT[player] & 15) != (P1_PREV_PROCESSED_INPUT[player] & 15)) {
        changed_direction = 1;
        ram_ffda3a[player] = 0;
    } else if (ram_ffda3a[player] && ram_ffda3a[player]--)
        return;
    if (ram_ffe1dc[player])
        input = func_01cc62(player);
    else
        input = P1_PROCESSED_INPUT[player];
    if (state->velocity_y && (state->draw_flags & 32)) {
        input &= 0xfc;
        changed_direction = 1;
        if (!(input & 12)) {
            if (PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
                input |= 1;
            else
                input |= 2;
        }
    }
    if (state->velocity_x && (state->draw_flags & 16)) {
        input &= 0xf3;
        changed_direction = 1;
        if (!(input & 3)) {
            if (PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
                input |= 4;
            else
                input |= 8;
        }
    }
    func_0115da(state, player, changed_direction, input);
    func_011462(state, player, terrain);
}
