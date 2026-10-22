#include "player_animation.h"
#include "player_animation_groups.h"
#include "entities.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "player.h"
#include "ram_sprites.h"

/* MATCHING QUALIFICATION: ProcessMovement casts the shared unsigned-long
 * animation_group_mask to signed long in its fall and high-top tests.
 * Direct unsigned reads change MCC68K's mask code and scratch allocation;
 * these scalar views retain the matching instructions without changing
 * the shared record. Historical spelling is unknown [INFERENCE]. */
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
        if ((entity->draw_flags & ENTITY_ACTIVE) && !(entity->draw_flags & ENTITY_SKIP_PHYSICS) &&
            !EarthlingIsUpdateBlocked(entity->earthling)) {
            if (!(entity->animation_motion_axes & ANIMATION_MOTION_X) &&
                (entity->velocity_x || entity->acceleration_x)) {
                entity->velocity_x += entity->acceleration_x;
                fixed_position = (entity->x << 8) +
                    entity->fraction_x + entity->velocity_x;
                next_x = (short)(fixed_position >> 8);
                fraction_x = fixed_position & 0xff;
                entity->draw_flags &= ~ENTITY_BLOCKED_X;
            } else {
                next_x = entity->x;
                fraction_x = entity->fraction_x;
            }
            if (!(entity->animation_motion_axes & ANIMATION_MOTION_Y) &&
                (entity->velocity_y || entity->acceleration_y)) {
                entity->velocity_y += entity->acceleration_y;
                fixed_position = (entity->y << 8) +
                    entity->fraction_y + entity->velocity_y;
                next_y = (short)(fixed_position >> 8);
                fraction_y = fixed_position & 0xff;
                entity->draw_flags &= ~ENTITY_BLOCKED_Y_OR_HEIGHT;
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
                entity->draw_flags &= ~ENTITY_BLOCKED_Y_OR_HEIGHT;
            } else {
                next_z = entity->height;
                fraction_z = entity->fraction_height;
            }
            if (entity->entity_index <= 1) {
                if (next_x - entity->width <= 8 ||
                    entity->width + next_x >= 2652)
                    next_x = entity->x;
                if (next_y - entity->height_pixels - next_z <= 8 &&
                    entity->sprite_state != PLAYER_ANIM_GHOST && entity->sprite_state != PLAYER_ANIM_LAST_LIFE_GHOST &&
                    !((long)entity->animation_group_mask & PLAYER_GROUP_FALL)) {
                    next_y = entity->y;
                    next_z = entity->height;
                } else if (next_y >= 1736)
                    next_y = 1735;
            }
            if (entity->entity_index <= 1 && !entity->animation_motion_axes) {
                if (!EntityFindVolumeCollisionAtPosition(entity, entity->x, entity->y, entity->height, 0)) {
                    if (((long)entity->animation_group_mask & PLAYER_GROUP_HITOPS) &&
                        PlayerCheckTerrainEdgeHop(entity, next_x, next_y))
                        next_z = 10;
                    if (next_x != entity->x &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, entity->y, entity->height, 0)) {
                        next_x = entity->x;
                        fraction_x = entity->fraction_x;
                        entity->draw_flags |= ENTITY_BLOCKED_X;
                    }
                    if (next_y != entity->y &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, next_y, entity->height, 0)) {
                        next_y = entity->y;
                        fraction_y = entity->fraction_y;
                        entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
                    }
                    if (next_z < entity->height &&
                        EntityFindVolumeCollisionAtPosition(entity, next_x, next_y, next_z, 2)) {
                        next_z = entity->height;
                        fraction_z = entity->fraction_height;
                        entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
                        if ((long)entity->animation_group_mask & PLAYER_GROUP_FALL) {
                            entity->velocity_height = 0x300;
                            entity->acceleration_height = -64;
                            next_y += 4;
                        }
                    }
                } else if (ElevatorBlocksEntityMovement(entity, next_x, next_y)) {
                    next_y = entity->y;
                    fraction_y = entity->fraction_y;
                    entity->draw_flags |= ENTITY_BLOCKED_Y_OR_HEIGHT;
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
