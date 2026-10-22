#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "entities.h"
#include "core_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_map.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
/* Local 12-byte proposed-position result, not an entity storage view. */
struct movement_destination {
    short x, y;                            /* 00/02: proposed world pixels; terrain-motion producer writes. */
    char unused[2];                        /* 04: no producer/consumer access in packet 00dd82. */
    short tile_x, tile_y;                   /* 06/08: indices in the 5x5 terrain cache, usually 0..4; producer writes. */
    unsigned char fraction_x, fraction_y;  /* 0a/0b: fractional world pixels, 0..255; producer writes. */
};

/* Apply the nearby terrain grid to a proposed 24.8 position. Properties at
 * least 0x80 also adjust edge cells and select collision-facing sprites. */
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
        terrain = LAND_NEAR_TOEJAM[0];
    else
        terrain = LAND_NEAR_EARL[0];
    if (gLevel912C == 26 &&
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
    if ((long)entity->animation_group_mask & (long)(PLAYER_GROUP_SWIM | PLAYER_GROUP_INNERTUBE_MOVE)) {
        if (entity->level == 1 || entity->level == 26) {
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
            if (alternate || gPlayer1Powerup[player] == PRESENT_SPRING_SHOES) {
                sprite += 12;
                if (gPlayer1Powerup[player] != PRESENT_SPRING_SHOES) {
                    if (sprite & 2)
                        entity->velocity_y = 0;
                    else
                        entity->velocity_x = 0;
                }
            }
            SetSprite(entity, sprite);
            entity->draw_flags |= (ENTITY_BLOCKED_X | ENTITY_BLOCKED_Y_OR_HEIGHT);
            TOEJAM_JUMP_COUNTER[player] = 0;
        }
    }
}

/* StopPlayer (name from RyanFae's disassembly) leaves the first six bytes alone and
 * clears two interleaved groups of three 16-bit components, in component
 * order. [INFERENCE] They are the motion components of a player record. */

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

/* Process callbacks installed by SetupPlayer for the two player records. */
void Player1ProcessTerrainMotion(direction)
char *direction;
{
    PlayerProcessTerrainMotion(0, direction);
}

void Player2ProcessTerrainMotion(direction)
char *direction;
{
    PlayerProcessTerrainMotion(1, direction);
}
