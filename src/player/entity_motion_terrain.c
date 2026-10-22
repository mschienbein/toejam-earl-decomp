#include "entities.h"
#include "player.h"
#include "random.h"
#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "core_calls.h"
#include "ram_effects.h"
#include "ram_map.h"
#include "ram_player.h"
#include "sprite_calls.h"
#include "ram_ui.h"
#include "ram_elevator.h"
#include "elevator_calls.h"
#include "collision_calls.h"
#include "revision.h"

/* Require the same level and absolute x/y differences below 16 and 5. */
int EntityIsNearPosition(entity, x, y, level)
struct object_state *entity;
short x, y;
char level;
{
    if (entity->level == level && abs(entity->x - x) < 16 &&
        abs(entity->y - y) < 5)
        return 1;
    else
        return 0;
}

/* Require the same level and distances below the caller's exclusive limits. */
int EntityIsWithinRange(entity, other, distance_x, distance_y)
struct object_state *entity, *other;
short distance_x, distance_y;
{
    if (entity->level == other->level &&
        abs(entity->x - other->x) < distance_x &&
        abs(entity->y - other->y) < distance_y)
        return 1;
    else
        return 0;
}

/* Local 12-byte proposed-position result, not an entity storage view. */
struct movement_destination {
    short x, y;                            /* 00/02: proposed world pixels; terrain-motion producer writes. */
    char unused[2];                        /* 04: no producer/consumer access in packet 00dd82. */
    short tile_x, tile_y;                  /* 06/08: indices in the 5x5 terrain cache, usually 0..4; producer writes. */
    unsigned char fraction_x, fraction_y;  /* 0a/0b: fractional world pixels, 0..255; producer writes. */
};

/* Animation groups during which terrain motion is not processed; REV 02 also
 * skips it during the lean animations. */
#if REVISION >= 2
#define TERRAIN_MOTION_PAUSED_GROUPS (PLAYER_GROUP_REACTION | PLAYER_GROUP_FALL_SPLASH | PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_HURT | PLAYER_GROUP_JUMP_SPLASH | PLAYER_GROUP_LEAN)
#else
#define TERRAIN_MOTION_PAUSED_GROUPS (PLAYER_GROUP_REACTION | PLAYER_GROUP_FALL_SPLASH | PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_HURT | PLAYER_GROUP_JUMP_SPLASH)
#endif

/* Form a proposed 24.8 position, reject crossings, then apply terrain.
 *
 * MATCHING QUALIFICATION: the cell index goes through a signed 32-bit
 * stage (shifted_cell) and the Y fraction is also captured through an int at
 * its store (all such values are 0..255); a direct byte store or a short
 * stage changes MCC68K's register allocation. The unused long[1] keeps the
 * observed untouched four-byte frame slot; its historical purpose is
 * unknown [INFERENCE]. */
void PlayerProcessTerrainMotion(player, direction)
short player;
char *direction;
{
    short velocity_delta;
    int fraction_output;
    long unused_motion[1];
    struct object_state *entity;
    unsigned int fraction;
    long fixed_position;
    short coordinate;
    int shifted_cell;
    struct movement_destination destination;

    if (gPlayerLives[player] < 0 || gPlayerInventoryOpen[player])
        return;
    destination.x = -1;
    destination.y = -1;
    entity = &gPlayerInfo[player];
    if ((entity->draw_flags & ENTITY_SKIP_PHYSICS) ||
        ((long)entity->animation_group_mask & TERRAIN_MOTION_PAUSED_GROUPS) ||
        entity->sprite_state == PLAYER_ANIM_GHOST ||
        entity->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
        return;
    if (entity->animation_motion_axes & ANIMATION_MOTION_X) {
        velocity_delta = (short)(entity->velocity_x * direction[1]);
        fixed_position = (entity->x << 8) + entity->fraction_x + velocity_delta;
        coordinate = (short)(fixed_position >> 8);
        destination.x = coordinate;
        fraction = fixed_position & 255;
        destination.fraction_x = fraction;
        shifted_cell = (long)destination.x >> 3;
        coordinate = (short)shifted_cell;
        destination.tile_x = coordinate - (long)(entity->x >> 3) + 2;
    } else {
        destination.x = entity->x;
        destination.fraction_x = entity->fraction_x;
        destination.tile_x = 2;
    }
    if (entity->animation_motion_axes & ANIMATION_MOTION_Y) {
        velocity_delta = (short)(entity->velocity_y * direction[2]);
        fixed_position = (entity->y << 8) + entity->fraction_y + velocity_delta;
        fraction = fixed_position & 255;
        destination.fraction_y = (fraction_output = fraction);
        coordinate = (short)(fixed_position >> 8);
        destination.y = coordinate;
        shifted_cell = (long)destination.y >> 3;
        coordinate = (short)shifted_cell;
        destination.tile_y = coordinate - (long)(entity->y >> 3) + 2;
    } else {
        destination.y = entity->y;
        destination.fraction_y = entity->fraction_y;
        destination.tile_y = 2;
    }
    entity->draw_flags &= ~(ENTITY_BLOCKED_X | ENTITY_BLOCKED_Y_OR_HEIGHT);
    /* Walking players do not step into each other (outside elevators). */
    if (((long)entity->animation_group_mask &
         (PLAYER_GROUP_INNERTUBE_MOVE | PLAYER_GROUP_SPRING_SHOES | PLAYER_GROUP_SNEAK | PLAYER_GROUP_WALK)) &&
        !EntityIsNearPosition(&gPlayerInfo[player ^ 1], entity->x, entity->y, entity->level) &&
        !gPlayerElevatorFlags[player] && !gPlayerElevatorFlags[player ^ 1]) {
        if (EntityIsNearPosition(&gPlayerInfo[player ^ 1], destination.x, entity->y, entity->level)) {
            destination.x = entity->x;
            destination.fraction_x = entity->fraction_x;
            destination.tile_x = 2;
            entity->draw_flags |= ENTITY_BLOCKED_X;
        }
        if (EntityIsNearPosition(&gPlayerInfo[player ^ 1], destination.x, destination.y, entity->level)) {
            destination.y = entity->y;
            destination.fraction_y = entity->fraction_y;
            destination.tile_y = 2;
            entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
        }
    }
    /* Narrow entity collisions block each axis separately. */
    if (!EntityFindNarrowCollisionAtPosition(entity, entity->x, entity->y, 6)) {
        if (EntityFindNarrowCollisionAtPosition(entity, destination.x, entity->y, 6)) {
            destination.x = entity->x;
            destination.fraction_x = entity->fraction_x;
            destination.tile_x = 2;
            entity->draw_flags |= ENTITY_BLOCKED_X;
        }
        if (EntityFindNarrowCollisionAtPosition(entity, destination.x, destination.y, 6)) {
            destination.y = entity->y;
            destination.fraction_y = entity->fraction_y;
            destination.tile_y = 2;
            entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
        }
    }
    if (ElevatorBlocksPlayerMovement(player, destination.x, entity->y, 1)) {
        destination.x = entity->x;
        destination.fraction_x = entity->fraction_x;
        destination.tile_x = 2;
        entity->draw_flags |= ENTITY_BLOCKED_X;
    }
    if (ElevatorBlocksPlayerMovement(player, destination.x, destination.y, 0)) {
        destination.y = entity->y;
        destination.fraction_y = entity->fraction_y;
        destination.tile_y = 2;
        entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
    }
    if (entity->sprite_state == PLAYER_ANIM_WHIRLWIND) {
        /* A whirlwind carries the player anywhere inside the map border. */
        if (destination.x <= 28 || destination.x >= 2652) {
            destination.x = entity->x;
            entity->draw_flags |= ENTITY_BLOCKED_X;
        }
        if (destination.y <= 48 || destination.y >= 1736) {
            destination.y = entity->y;
            entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
        }
        entity->x = destination.x;
        entity->fraction_x = destination.fraction_x;
        entity->y = destination.y;
        entity->fraction_y = destination.fraction_y;
    } else if (entity->sprite_state != PLAYER_ANIM_WHIRLWIND_RECOVERY)
        PlayerApplyTerrainDestination(player, entity, &destination);
}

/* Apply the nearby terrain grid to a proposed 24.8 position. Properties at
 * least 0x80 also adjust edge cells and select collision-facing sprites.
 *
 * MATCHING QUALIFICATION: swimming and innertube movement are tested through
 * a signed-long view of animation_group_mask; reading the shared unsigned-long
 * field directly changes the generated code. The common record stays
 * unsigned; historical spelling is unknown [INFERENCE]. */
void PlayerApplyTerrainDestination(player, entity, destination)
short player;
struct object_state *entity;
struct movement_destination *destination;
{
    short alternate = 0;
    short sprite = -1;
    unsigned char *terrain;
    unsigned char property;
    short saved_x, saved_y;

    if (!player)
        terrain = gPlayer1NearbyTerrain[0];
    else
        terrain = gPlayer2NearbyTerrain[0];
    if (gPlayer1MapLevel == LEVEL_FINAL &&
        (entity->x != destination->x || entity->y != destination->y)) {
        saved_x = entity->x;
        saved_y = entity->y;
        entity->x = destination->x;
        entity->y = destination->y;
        if (PlayersRequireSeparateViews()) {
            entity->x = saved_x;
            entity->y = saved_y;
            entity->draw_flags |= (ENTITY_BLOCKED_X | ENTITY_BLOCKED_Y_OR_HEIGHT);
            return;
        }
        entity->x = saved_x;
        entity->y = saved_y;
    }
    property = terrain[destination->tile_y * 5 + destination->tile_x] & TERRAIN_CLASS_MASK;
    if ((long)entity->animation_group_mask & (PLAYER_GROUP_SWIM | PLAYER_GROUP_INNERTUBE_MOVE)) {
        if (entity->level == 1 || entity->level == LEVEL_FINAL) {
            if (destination->x <= 28 || destination->x >= 2652) {
                destination->x = entity->x;
                entity->draw_flags |= ENTITY_BLOCKED_X;
            }
            if (destination->y <= 48 || destination->y >= 1736) {
                destination->y = entity->y;
                entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
            }
        }
        if (property != TERRAIN_WATER) {
            sprite = (entity->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_CLIMB_UP;
            SetSprite(entity, sprite);
            entity->velocity_x = 0;
            entity->velocity_y = 0;
            entity->animation_motion_axes = ANIMATION_MOTION_NONE;
        }
        property = 0x50;
    }
    if (property <= TERRAIN_WALKABLE_MAX || entity->height > 0) {
        entity->x = destination->x;
        entity->fraction_x = destination->fraction_x;
        entity->y = destination->y;
        entity->fraction_y = destination->fraction_y;
    } else if (property >= TERRAIN_WATER) {
        if (property == TERRAIN_WATER)
            alternate = 1;
        if (destination->tile_x == 0) {
            destination->x += 8;
            ++destination->tile_x;
        } else if (destination->tile_x == 4) {
            destination->x -= 8;
            --destination->tile_x;
        }
        if (destination->tile_y == 0) {
            destination->y += 8;
            ++destination->tile_y;
        } else if (destination->tile_y == 4) {
            destination->y -= 8;
            --destination->tile_y;
        }
        property = terrain[destination->tile_y * 5 + destination->tile_x] & TERRAIN_CLASS_MASK;
        if (property <= TERRAIN_WALKABLE_MAX) {
            entity->x = destination->x;
            entity->fraction_x = 0;
            entity->y = destination->y;
            entity->fraction_y = 0;
        }
        if (entity->velocity_x > 0)
            sprite = PLAYER_ANIM_LEAN_RIGHT;
        if (entity->velocity_x < 0)
            sprite = PLAYER_ANIM_LEAN_LEFT;
        if (entity->velocity_y > 0 && (terrain[17] >= TERRAIN_WATER || terrain[22] >= TERRAIN_WATER))
            sprite = PLAYER_ANIM_LEAN_DOWN;
        if (entity->velocity_y < 0 && (terrain[2] >= TERRAIN_WATER || terrain[7] >= TERRAIN_WATER))
            sprite = PLAYER_ANIM_LEAN_UP;
        if (sprite != -1) {
            if (alternate || gPlayerPowerups[player] == PRESENT_SPRING_SHOES) {
                sprite += 12;
                if (gPlayerPowerups[player] != PRESENT_SPRING_SHOES) {
                    if (sprite & 2)
                        entity->velocity_y = 0;
                    else
                        entity->velocity_x = 0;
                }
            }
            SetSprite(entity, sprite);
            entity->draw_flags |= (ENTITY_BLOCKED_X | ENTITY_BLOCKED_Y_OR_HEIGHT);
            gPlayerJumpCounters[player] = 0;
        }
    }
}

/* Clear all three velocity and acceleration components without changing position. */

void StopPlayer(state)
struct object_state *state;
{
    state->velocity_x = 0;
    state->acceleration_x = 0;
    state->velocity_y = 0;
    state->acceleration_y = 0;
    state->velocity_height = 0;
    state->acceleration_height = 0;
}

/* Apply terrain motion to the first player using the supplied direction byte. */
void Player1ProcessTerrainMotion(direction)
char *direction;
{
    PlayerProcessTerrainMotion(0, direction);
}

/* Apply terrain motion to the second player using the supplied direction byte. */
void Player2ProcessTerrainMotion(direction)
char *direction;
{
    PlayerProcessTerrainMotion(1, direction);
}
