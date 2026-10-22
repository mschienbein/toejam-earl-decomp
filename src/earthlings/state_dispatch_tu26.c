#define TJE_LOCAL_FUNC_00D9DC

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "ram.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "present_calls.h"
#include "romcalls.h"

/* ORIGINAL BUG: this consumer keeps a int result view of func_00d9dc,
 * whose canonical return is unsigned char. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
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
struct render_frame *step;
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
        distance = object->velocity_x * step->motion_x;
        fixed_position = object->fraction_x +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->fraction_x;
    }
    if (object->mode_15 & 2) {
        distance = object->velocity_y * step->motion_y;
        fixed_position = object->fraction_y +
            (object->y << 8) + distance;
        fraction_y = fixed_position & 0xff;
        next_y = (short)(fixed_position >> 8);
    } else {
        next_y = object->y;
        fraction_y = object->fraction_y;
    }
    if (next_x == object->x && next_y == object->y)
        return;
    if (earthling->kind == 5 || earthling->kind == 10 ||
        earthling->kind == 11 || earthling->kind == 17) {
        if (!func_025bce(object, object->x, object->y, 0) &&
            func_025bce(object, next_x, next_y, 0)) {
            object->sprite_state = 0;
            return;
        }
    } else if (earthling->kind != 18) {
        if (!func_025ca4(object, object->x, object->y) &&
            (terrain = func_025ca4(object, next_x, next_y))) {
            object->sprite_state = 0;
            func_015da6(terrain - 1);
            return;
        }
    }
    if (earthling->kind == 14 || earthling->kind == 18)
        terrain = func_01916c(player_number, next_x, next_y, earthling->state);
    else
        terrain = func_00d9dc(player_number, next_x, next_y);
    if ((earthling->kind == 13 && terrain == 16) ||
        (earthling->kind == 14 && terrain == 128) ||
        (earthling->kind != 13 && earthling->kind != 14 &&
         (unsigned short)terrain < 80)) {
        object->x = next_x;
        object->fraction_x = fraction_x;
        object->y = next_y;
        object->fraction_y = fraction_y;
    } else
        object->sprite_state = 0;
}

/* Freeze a non-null record unless a selected player's gate byte is clear. */
int func_0193fa(earthling)
struct earthling_state *earthling;
{
    if (earthling) {
        if (((earthling->reserved_10[1] & 2) && !P1_MENU_OPEN[0]) ||
            ((earthling->reserved_10[1] & 4) && !HBLANK_FLAG_1))
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
struct render_frame *step;
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
        (earthling->kind == 7 && earthling->state == 3))
        return;
    if (object->level == gLevel912C)
        player_number = 0;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (object->mode_15 & 1) {
        distance = object->velocity_x * step->motion_x;
        fixed_position = object->fraction_x +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->fraction_x;
    }
    if (object->mode_15 & 2) {
        distance = object->velocity_y * step->motion_y;
        fixed_position = object->fraction_y +
            (object->y << 8) + distance;
        fraction_y = fixed_position & 0xff;
        next_y = (short)(fixed_position >> 8);
    } else {
        next_y = object->y;
        fraction_y = object->fraction_y;
    }
    if (next_x - object->width <= 8 || object->width + next_x >= 2672)
        next_x = object->x;
    if (next_y - object->height_pixels <= 8 || next_y >= 1776)
        next_y = object->y;
    if (!func_025ca4(object, object->x, object->y) &&
        (collision = func_025ca4(object, next_x, next_y))) {
        object->sprite_state = 0;
        func_015da6(collision - 1);
        return;
    }
    terrain = func_00d9dc(player_number, next_x, next_y);
    if (earthling->kind == 19) {
        if ((ram_ffda1c & 127) == 34)
            return;
        earthling->value_0a = terrain;
    } else if (terrain == 144) {
        if (earthling->kind == 6 || earthling->kind == 22)
            object->flags_4d |= 32;
        else
            object->draw_flags &= ~64;
    } else {
        if (earthling->kind == 6 || earthling->kind == 22)
            object->flags_4d &= ~32;
        else
            object->draw_flags |= 64;
    }
    object->x = next_x;
    object->fraction_x = fraction_x;
    object->y = next_y;
    object->fraction_y = fraction_y;
}
