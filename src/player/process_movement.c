struct moving_entity {
    short x, y, elevation;
    short velocity_x, velocity_y, velocity_z;
    short acceleration_x, acceleration_y, acceleration_z;
    unsigned char fraction_x, fraction_y, fraction_z, movement_lock;
    char reserved16[19];
    unsigned char half_width, height;
    char reserved2b[4];
    unsigned char flags;
    char reserved30[27];
    char sprite_state;
    char reserved4c[2];
    long identity;
    long sprite_mask;
    char reserved56[4];
    struct moving_entity *next;
    char character;
    char reserved5f[33];
};
extern struct moving_entity *ram_ffd938;
extern int func_0193fa(), func_025d80(), func_012028(), func_012d0c();

/* Integrate the linked active entities in 24.8 coordinates. Player records
 * additionally clamp map bounds and test horizontal and descending collisions. */
void ProcessMovement()
{
    struct moving_entity *entity;
    long fixed_position;
    register unsigned char fraction_z;
    unsigned char fraction_y, fraction_x;
    short next_y, next_x, next_z;

    for (entity = ram_ffd938; entity; entity = entity->next) {
        if ((entity->flags & 1) && !(entity->flags & 8) &&
            !func_0193fa(entity->identity)) {
            if (!(entity->movement_lock & 1) &&
                (entity->velocity_x || entity->acceleration_x)) {
                entity->velocity_x += entity->acceleration_x;
                fixed_position = (entity->x << 8) +
                    entity->fraction_x + entity->velocity_x;
                next_x = (short)(fixed_position >> 8);
                fraction_x = fixed_position & 0xff;
                entity->flags &= 0xef;
            } else {
                next_x = entity->x;
                fraction_x = entity->fraction_x;
            }
            if (!(entity->movement_lock & 2) &&
                (entity->velocity_y || entity->acceleration_y)) {
                entity->velocity_y += entity->acceleration_y;
                fixed_position = (entity->y << 8) +
                    entity->fraction_y + entity->velocity_y;
                next_y = (short)(fixed_position >> 8);
                fraction_y = fixed_position & 0xff;
                entity->flags &= 0xdf;
            } else {
                next_y = entity->y;
                fraction_y = entity->fraction_y;
            }
            if (entity->velocity_z || entity->acceleration_z) {
                entity->velocity_z += entity->acceleration_z;
                fixed_position = (entity->elevation << 8) +
                    entity->fraction_z + entity->velocity_z;
                next_z = (short)(fixed_position >> 8);
                fraction_z = fixed_position & 0xff;
                entity->flags &= 0xdf;
            } else {
                next_z = entity->elevation;
                fraction_z = entity->fraction_z;
            }
            if (entity->character <= 1) {
                if (next_x - entity->half_width <= 8 ||
                    entity->half_width + next_x >= 2652)
                    next_x = entity->x;
                if (next_y - entity->height - next_z <= 8 &&
                    entity->sprite_state != 5 && entity->sprite_state != 6 &&
                    !(entity->sprite_mask & 32)) {
                    next_y = entity->y;
                    next_z = entity->elevation;
                } else if (next_y >= 1736)
                    next_y = 1735;
            }
            if (entity->character <= 1 && !entity->movement_lock) {
                if (!func_025d80(entity, entity->x, entity->y, entity->elevation, 0)) {
                    if ((entity->sprite_mask & 0x08000000) &&
                        func_012028(entity, next_x, next_y))
                        next_z = 10;
                    if (next_x != entity->x &&
                        func_025d80(entity, next_x, entity->y, entity->elevation, 0)) {
                        next_x = entity->x;
                        fraction_x = entity->fraction_x;
                        entity->flags |= 16;
                    }
                    if (next_y != entity->y &&
                        func_025d80(entity, next_x, next_y, entity->elevation, 0)) {
                        next_y = entity->y;
                        fraction_y = entity->fraction_y;
                        entity->flags |= 32;
                    }
                    if (next_z < entity->elevation &&
                        func_025d80(entity, next_x, next_y, next_z, 2)) {
                        next_z = entity->elevation;
                        fraction_z = entity->fraction_z;
                        entity->flags |= 32;
                        if (entity->sprite_mask & 32) {
                            entity->velocity_z = 0x300;
                            entity->acceleration_z = -64;
                            next_y += 4;
                        }
                    }
                } else if (func_012d0c(entity, next_x, next_y)) {
                    next_y = entity->y;
                    fraction_y = entity->fraction_y;
                    entity->flags |= 32;
                }
            }
            entity->x = next_x;
            entity->fraction_x = fraction_x;
            entity->y = next_y;
            entity->fraction_y = fraction_y;
            entity->elevation = next_z;
            entity->fraction_z = fraction_z;
        }
    }
}
