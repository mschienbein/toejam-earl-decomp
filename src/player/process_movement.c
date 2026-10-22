#include "entities.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "player.h"
#include "ram_sprites.h"

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
/* Integrate the linked active entities in 24.8 coordinates. Player records
 * additionally clamp map bounds and test horizontal and descending collisions. */
void ProcessMovement()
{
    struct object_state *entity;
    long fixed_position;
    unsigned char fraction_z;
    unsigned char fraction_y, fraction_x;
    short next_y, next_x, next_z;

    for (entity = gEntityDrawListHead; entity; entity = entity->next) {
        if ((entity->draw_flags & 1) && !(entity->draw_flags & 8) &&
            !EarthlingIsUpdateBlocked(entity->earthling)) {
            if (!(entity->mode_15 & 1) &&
                (entity->velocity_x || entity->acceleration_x)) {
                entity->velocity_x += entity->acceleration_x;
                fixed_position = (entity->x << 8) +
                    entity->fraction_x + entity->velocity_x;
                next_x = (short)(fixed_position >> 8);
                fraction_x = fixed_position & 0xff;
                entity->draw_flags &= 0xef;
            } else {
                next_x = entity->x;
                fraction_x = entity->fraction_x;
            }
            if (!(entity->mode_15 & 2) &&
                (entity->velocity_y || entity->acceleration_y)) {
                entity->velocity_y += entity->acceleration_y;
                fixed_position = (entity->y << 8) +
                    entity->fraction_y + entity->velocity_y;
                next_y = (short)(fixed_position >> 8);
                fraction_y = fixed_position & 0xff;
                entity->draw_flags &= 0xdf;
            } else {
                next_y = entity->y;
                fraction_y = entity->fraction_y;
            }
            if (entity->velocity_height || entity->acceleration_height) {
                entity->velocity_height += entity->acceleration_height;
                fixed_position = (entity->height << 8) +
                    entity->fraction_height + entity->velocity_height;
                next_z = (short)(fixed_position >> 8);
                fraction_z = fixed_position & 0xff;
                entity->draw_flags &= 0xdf;
            } else {
                next_z = entity->height;
                fraction_z = entity->fraction_height;
            }
            if (entity->entity_number <= 1) {
                if (next_x - entity->width <= 8 ||
                    entity->width + next_x >= 2652)
                    next_x = entity->x;
                if (next_y - entity->height_pixels - next_z <= 8 &&
                    entity->sprite_state != 5 && entity->sprite_state != 6 &&
                    !((long)entity->flags_52 & 32)) {
                    next_y = entity->y;
                    next_z = entity->height;
                } else if (next_y >= 1736)
                    next_y = 1735;
            }
            if (entity->entity_number <= 1 && !entity->mode_15) {
                if (!EntityFindVolumeCollisionAtPosition(entity, entity->x, entity->y, entity->height, 0)) {
                    if (((long)entity->flags_52 & 0x08000000) &&
                        PlayerCheckTerrainEdgeHop(entity, next_x, next_y))
                        next_z = 10;
                    if (next_x != entity->x &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, entity->y, entity->height, 0)) {
                        next_x = entity->x;
                        fraction_x = entity->fraction_x;
                        entity->draw_flags |= 16;
                    }
                    if (next_y != entity->y &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, next_y, entity->height, 0)) {
                        next_y = entity->y;
                        fraction_y = entity->fraction_y;
                        entity->draw_flags |= 32;
                    }
                    if (next_z < entity->height &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, next_y, next_z, 2)) {
                        next_z = entity->height;
                        fraction_z = entity->fraction_height;
                        entity->draw_flags |= 32;
                        if ((long)entity->flags_52 & 32) {
                            entity->velocity_height = 0x300;
                            entity->acceleration_height = -64;
                            next_y += 4;
                        }
                    }
                } else if (ElevatorBlocksEntityMovement(entity, next_x, next_y)) {
                    next_y = entity->y;
                    fraction_y = entity->fraction_y;
                    entity->draw_flags |= 32;
                }
            }
            entity->x = next_x;
            entity->fraction_x = fraction_x;
            entity->y = next_y;
            entity->fraction_y = fraction_y;
            entity->height = next_z;
            entity->fraction_height = fraction_z;
        }
    }
}
