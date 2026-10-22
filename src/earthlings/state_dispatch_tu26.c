#include "earthling_types.h"
#include "terrain.h"
#define TJE_LOCAL_FUNC_00D9DC

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "present_calls.h"
#include "ram_map.h"
#include "ram_system.h"
#include "ram_ui.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared unsigned char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetMapTerrainAtWorldPosition();
/* Probe eight pixels vertically or sixteen horizontally in states 4..7. */
int EarthlingProbeDirectionalTerrain(player_number, world_x, world_y, state)
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
    return (unsigned char)GetMapTerrainAtWorldPosition(player_number, world_x, world_y);
}

/* Predict a fixed-point movement step and reject newly entered obstacles. */
void EarthlingMoveOnAllowedTerrain(step, object)
struct render_frame *step;
struct object_state *object;
{
    struct earthling_state *earthling;
    long fixed_position;
    register short next_y;
    unsigned char fraction_x, fraction_y;
    short next_x, terrain, player_number, distance;

    earthling = object->earthling;
    if (EarthlingIsUpdateBlocked(object->earthling) || earthling->state == 99)
        return;
    if (object->level == gLevel912C)
        player_number = PLAYER_TOEJAM;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (object->animation_motion_axes & ANIMATION_MOTION_X) {
        distance = object->velocity_x * step->motion_x;
        fixed_position = object->fraction_x +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->fraction_x;
    }
    if (object->animation_motion_axes & ANIMATION_MOTION_Y) {
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
    if (earthling->kind == EARTHLING_HULA_DANCER || earthling->kind == EARTHLING_WIZARD ||
        earthling->kind == EARTHLING_WISEMAN || earthling->kind == EARTHLING_OPERA_SINGER) {
        if (!EntityFindNarrowCollisionAtPosition(object, object->x, object->y, 0) &&
            EntityFindNarrowCollisionAtPosition(object, next_x, next_y, 0)) {
            object->sprite_state = 0;
            return;
        }
    } else if (earthling->kind != EARTHLING_MOLE) {
        if (!EntityFindMenuAwareCollisionAtPosition(object, object->x, object->y) &&
            (terrain = EntityFindMenuAwareCollisionAtPosition(object, next_x, next_y))) {
            object->sprite_state = 0;
            PresentRegisterDecoyHit(terrain - 1);
            return;
        }
    }
    if (earthling->kind == EARTHLING_SHARK || earthling->kind == EARTHLING_MOLE)
        terrain = EarthlingProbeDirectionalTerrain(player_number, next_x, next_y, earthling->state);
    else
        terrain = GetMapTerrainAtWorldPosition(player_number, next_x, next_y);
    if ((earthling->kind == EARTHLING_LAWNMOWER && terrain == TERRAIN_GROUND) ||
        (earthling->kind == EARTHLING_SHARK && terrain == TERRAIN_WATER) ||
        (earthling->kind != EARTHLING_LAWNMOWER && earthling->kind != EARTHLING_SHARK &&
         (unsigned short)terrain < 80)) {
        object->x = next_x;
        object->fraction_x = fraction_x;
        object->y = next_y;
        object->fraction_y = fraction_y;
    } else
        object->sprite_state = 0;
}

/* Freeze a non-null record unless a selected player's gate byte is clear. */
int EarthlingIsUpdateBlocked(earthling)
struct earthling_state *earthling;
{
    if (earthling) {
        if (((earthling->health_visibility[1] & 2) && !P1_MENU_OPEN[PLAYER_TOEJAM]) ||
            ((earthling->health_visibility[1] & 4) && !HBLANK_FLAG_1))
            return 0;
        else
            return 1;
    }
    return 0;
}

/* Keep the sprite's terrain-dependent drawing bit synchronized. */
void EntityUpdateTerrainDrawFlag(object)
struct object_state *object;
{
    short player_number;

    if (object->level == gLevel912C)
        player_number = PLAYER_TOEJAM;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (GetMapTerrainAtWorldPosition(player_number, object->x, object->y) == 144)
        object->draw_flags &= ~ENTITY_SHADOW;
    else
        object->draw_flags |= ENTITY_SHADOW;
}

/* Move eligible earthlings within map bounds and update their terrain flags. */
void EarthlingMoveWithinMapBounds(step, object)
struct render_frame *step;
struct object_state *object;
{
    struct earthling_state *earthling;
    long fixed_position;
    register short next_y;
    unsigned char fraction_x, fraction_y;
    short next_x, terrain, player_number, distance, collision;

    earthling = object->earthling;
    if (EarthlingIsUpdateBlocked(earthling) || earthling->state == 99 ||
        (object->behavior_flags & ENTITY_HIDDEN) ||
        (earthling->kind == EARTHLING_CUPID && earthling->state == 3))
        return;
    if (object->level == gLevel912C)
        player_number = PLAYER_TOEJAM;
    else if (object->level == gLevel912E)
        player_number = 1;
    else
        return;
    if (object->animation_motion_axes & ANIMATION_MOTION_X) {
        distance = object->velocity_x * step->motion_x;
        fixed_position = object->fraction_x +
            (object->x << 8) + distance;
        next_x = (short)(fixed_position >> 8);
        fraction_x = fixed_position & 0xff;
    } else {
        next_x = object->x;
        fraction_x = object->fraction_x;
    }
    if (object->animation_motion_axes & ANIMATION_MOTION_Y) {
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
    if (!EntityFindMenuAwareCollisionAtPosition(object, object->x, object->y) &&
        (collision = EntityFindMenuAwareCollisionAtPosition(object, next_x, next_y))) {
        object->sprite_state = 0;
        PresentRegisterDecoyHit(collision - 1);
        return;
    }
    terrain = GetMapTerrainAtWorldPosition(player_number, next_x, next_y);
    if (earthling->kind == EARTHLING_ICE_CREAM_TRUCK) {
        if ((gTerrainQueryMetatileWord & 127) == 34)
            return;
        earthling->saved_x_or_status = terrain;
    } else if (terrain == TERRAIN_VOID) {
        if (earthling->kind == EARTHLING_BEES || earthling->kind == EARTHLING_BEES_VARIANT)
            object->behavior_flags |= ENTITY_MASK_PIXELS;
        else
            object->draw_flags &= ~ENTITY_SHADOW;
    } else {
        if (earthling->kind == EARTHLING_BEES || earthling->kind == EARTHLING_BEES_VARIANT)
            object->behavior_flags &= ~ENTITY_MASK_PIXELS;
        else
            object->draw_flags |= ENTITY_SHADOW;
    }
    object->x = next_x;
    object->fraction_x = fraction_x;
    object->y = next_y;
    object->fraction_y = fraction_y;
}
