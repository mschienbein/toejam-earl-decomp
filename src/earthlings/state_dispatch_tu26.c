struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer;
    char reserved_6[4];
    short terrain;
    char reserved_c[5];
    unsigned char flags_11;
};
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    unsigned char reserved_12[3];
    char mode_15, reserved_16[4];
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
    struct earthling_state *earthling;
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
/* [INFERENCE] The callback record supplies signed movement multipliers. */
struct movement_step {
    char reserved, x, y;
};
extern char gLevel912C, gLevel912E, P1_MENU_OPEN, HBLANK_FLAG_1;
extern unsigned short ram_ffda1c;
extern int func_0193fa(), func_025bce(), func_025ca4();
extern void func_015da6();
/* ORIGINAL BUG: these callers use an int result for func_00d9dc, whose
 * accepted definition returns unsigned char. The explicit narrowing below
 * and at the terrain assignments preserves the observed caller behaviour. */
extern int func_00d9dc();

/* Probe eight pixels vertically or sixteen horizontally in states 4..7. */
int func_01916c(player_number, world_x, world_y, state)
short player_number, world_x, world_y, state;
{
    if (state == 4)
        world_y -= 8;
    else if (state == 5)
        world_y += 8;
    else if (state == 6)
        world_x -= 16;
    else if (state == 7)
        world_x += 16;
    return (unsigned char)func_00d9dc(player_number, world_x, world_y);
}

/* Predict a fixed-point movement step and reject newly entered obstacles. */
void func_0191d2(step, object)
struct movement_step *step;
struct object_state *object;
{
    struct earthling_state *earthling;
    long fixed_position;
    register short next_y;
    unsigned char fraction_x, fraction_y;
    short next_x, terrain, player_number, distance;

    earthling = object->earthling;
    if (func_0193fa(object->earthling) || earthling->state == 99)
        return;
    if (object->level == gLevel912C)
        player_number = 0;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (object->mode_15 & 1) {
        distance = object->velocity_x * step->x;
        fixed_position = object->reserved_12[0] +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->reserved_12[0];
    }
    if (object->mode_15 & 2) {
        distance = object->velocity_y * step->y;
        fixed_position = object->reserved_12[1] +
            (object->y << 8) + distance;
        fraction_y = fixed_position & 0xff;
        next_y = (short)(fixed_position >> 8);
    } else {
        next_y = object->y;
        fraction_y = object->reserved_12[1];
    }
    if (next_x == object->x && next_y == object->y)
        return;
    if (earthling->reserved == 5 || earthling->reserved == 10 ||
        earthling->reserved == 11 || earthling->reserved == 17) {
        if (!func_025bce(object, object->x, object->y, 0) &&
            func_025bce(object, next_x, next_y, 0)) {
            object->sprite = 0;
            return;
        }
    } else if (earthling->reserved != 18) {
        if (!func_025ca4(object, object->x, object->y) &&
            (terrain = func_025ca4(object, next_x, next_y))) {
            object->sprite = 0;
            func_015da6(terrain - 1);
            return;
        }
    }
    if (earthling->reserved == 14 || earthling->reserved == 18)
        terrain = func_01916c(player_number, next_x, next_y, earthling->state);
    else
        terrain = func_00d9dc(player_number, next_x, next_y);
    if ((earthling->reserved == 13 && terrain == 16) ||
        (earthling->reserved == 14 && terrain == 128) ||
        (earthling->reserved != 13 && earthling->reserved != 14 &&
         (unsigned short)terrain < 80)) {
        object->x = next_x;
        object->reserved_12[0] = fraction_x;
        object->y = next_y;
        object->reserved_12[1] = fraction_y;
    } else
        object->sprite = 0;
}

/* Freeze a non-null record unless a selected player's gate byte is clear. */
int func_0193fa(earthling)
struct earthling_state *earthling;
{
    if (earthling) {
        if (((earthling->flags_11 & 2) && !P1_MENU_OPEN) ||
            ((earthling->flags_11 & 4) && !HBLANK_FLAG_1))
            return 0;
        else
            return 1;
    }
    return 0;
}

/* Keep the sprite's terrain-dependent drawing bit synchronized. */
void func_01942c(object)
struct object_state *object;
{
    short player_number;

    if (object->level == gLevel912C)
        player_number = 0;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (func_00d9dc(player_number, object->x, object->y) == 144)
        object->draw_flags &= ~64;
    else
        object->draw_flags |= 64;
}

/* Move eligible earthlings within map bounds and update their terrain flags. */
void func_01948a(step, object)
struct movement_step *step;
struct object_state *object;
{
    struct earthling_state *earthling;
    long fixed_position;
    register short next_y;
    unsigned char fraction_x, fraction_y;
    short next_x, terrain, player_number, distance, collision;

    earthling = object->earthling;
    if (func_0193fa(earthling) || earthling->state == 99 ||
        (object->flags_4d & 64) ||
        (earthling->reserved == 7 && earthling->state == 3))
        return;
    if (object->level == gLevel912C)
        player_number = 0;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (object->mode_15 & 1) {
        distance = object->velocity_x * step->x;
        fixed_position = object->reserved_12[0] +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->reserved_12[0];
    }
    if (object->mode_15 & 2) {
        distance = object->velocity_y * step->y;
        fixed_position = object->reserved_12[1] +
            (object->y << 8) + distance;
        fraction_y = fixed_position & 0xff;
        next_y = (short)(fixed_position >> 8);
    } else {
        next_y = object->y;
        fraction_y = object->reserved_12[1];
    }
    if (next_x - object->width <= 8 || object->width + next_x >= 2672)
        next_x = object->x;
    if (next_y - object->height_pixels <= 8 || next_y >= 1776)
        next_y = object->y;
    if (!func_025ca4(object, object->x, object->y) &&
        (collision = func_025ca4(object, next_x, next_y))) {
        object->sprite = 0;
        func_015da6(collision - 1);
        return;
    }
    terrain = func_00d9dc(player_number, next_x, next_y);
    if (earthling->reserved == 19) {
        if ((ram_ffda1c & 127) == 34)
            return;
        earthling->terrain = terrain;
    } else if (terrain == 144) {
        if (earthling->reserved == 6 || earthling->reserved == 22)
            object->flags_4d |= 32;
        else
            object->draw_flags &= ~64;
    } else {
        if (earthling->reserved == 6 || earthling->reserved == 22)
            object->flags_4d &= ~32;
        else
            object->draw_flags |= 64;
    }
    object->x = next_x;
    object->reserved_12[0] = fraction_x;
    object->y = next_y;
    object->reserved_12[1] = fraction_y;
}
