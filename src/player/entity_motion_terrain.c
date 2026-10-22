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
    short x, y;
    char reserved04[2];
    short tile_x, tile_y;
    unsigned char fraction_x, fraction_y;
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
            entity->draw_flags |= 48;
            return;
        }
        entity->x = saved_x;
        entity->y = saved_y;
    }
    property = terrain[destination->tile_y * 5 + destination->tile_x] & 0xf0;
    if ((long)entity->flags_52 & 0x40800L) {
        if (entity->level == 1 || entity->level == 26) {
            if (destination->x <= 28 || destination->x >= 2652) {
                destination->x = entity->x;
                entity->draw_flags |= 16;
            }
            if (destination->y <= 48 || destination->y >= 1736) {
                destination->y = entity->y;
                entity->draw_flags |= 32;
            }
        }
        if (property != 0x80) {
            sprite = (entity->sprite_state & 3) + 48;
            SetSprite(entity, sprite);
            entity->velocity_x = 0;
            entity->velocity_y = 0;
            entity->mode_15 = 0;
        }
        property = 0x50;
    }
    if (property <= 0x50 || entity->height > 0) {
        entity->x = destination->x;
        entity->fraction_x = destination->fraction_x;
        entity->y = destination->y;
        entity->fraction_y = destination->fraction_y;
    } else if (property >= 0x80) {
        if (property == 0x80)
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
        property = terrain[destination->tile_y * 5 + destination->tile_x] & 0xf0;
        if (property <= 0x50) {
            entity->x = destination->x;
            entity->fraction_x = 0;
            entity->y = destination->y;
            entity->fraction_y = 0;
        }
        if (entity->velocity_x > 0)
            sprite = 19;
        if (entity->velocity_x < 0)
            sprite = 18;
        if (entity->velocity_y > 0 && (terrain[17] >= 0x80 || terrain[22] >= 0x80))
            sprite = 17;
        if (entity->velocity_y < 0 && (terrain[2] >= 0x80 || terrain[7] >= 0x80))
            sprite = 16;
        if (sprite != -1) {
            if (alternate || gPlayer1Powerup[player] == 1) {
                sprite += 12;
                if (gPlayer1Powerup[player] != 1) {
                    if (sprite & 2)
                        entity->velocity_y = 0;
                    else
                        entity->velocity_x = 0;
                }
            }
            SetSprite(entity, sprite);
            entity->draw_flags |= 48;
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
