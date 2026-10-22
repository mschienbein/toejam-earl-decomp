/* Coordinate, motion, and record-link layout also used by ProcessMovement.
 * This unit additionally reads the level byte at offset 0x4c. */
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
    char sprite_state, level, reserved4d;
    long identity;
    long sprite_mask;
    char reserved56[4];
    struct moving_entity *next;
    char character;
    char reserved5f[33];
};
extern long abs();

/* Require the same level and absolute x/y differences below 16 and 5. */
int func_00dcc2(entity, x, y, level)
struct moving_entity *entity;
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
int func_00dd1e(entity, other, distance_x, distance_y)
struct moving_entity *entity, *other;
short distance_x, distance_y;
{
    if (entity->level == other->level &&
        abs(entity->x - other->x) < distance_x &&
        abs(entity->y - other->y) < distance_y)
        return 1;
    else
        return 0;
}
