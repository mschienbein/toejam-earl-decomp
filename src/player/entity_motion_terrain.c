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
struct movement_destination {
    short x, y;
    char reserved04[2];
    short tile_x, tile_y;
    unsigned char fraction_x, fraction_y;
};
extern unsigned char LAND_NEAR_TOEJAM[], LAND_NEAR_EARL[];
extern char gLevel912C, gPlayer1Powerup[], TOEJAM_JUMP_COUNTER[];
extern int func_000fb6();
extern void SetSprite(), func_00dd82();

/* Apply the nearby terrain grid to a proposed 24.8 position. Properties at
 * least 0x80 also adjust edge cells and select collision-facing sprites. */
void func_00e17a(player, entity, destination)
short player;
struct moving_entity *entity;
struct movement_destination *destination;
{
    short alternate = 0;
    short sprite = -1;
    unsigned char *terrain;
    unsigned char property;
    short saved_x, saved_y;

    if (!player)
        terrain = LAND_NEAR_TOEJAM;
    else
        terrain = LAND_NEAR_EARL;
    if (gLevel912C == 26 &&
        (entity->x != destination->x || entity->y != destination->y)) {
        saved_x = entity->x;
        saved_y = entity->y;
        entity->x = destination->x;
        entity->y = destination->y;
        if (func_000fb6()) {
            entity->x = saved_x;
            entity->y = saved_y;
            entity->flags |= 48;
            return;
        }
        entity->x = saved_x;
        entity->y = saved_y;
    }
    property = terrain[destination->tile_y * 5 + destination->tile_x] & 0xf0;
    if (entity->sprite_mask & 0x40800L) {
        if (entity->level == 1 || entity->level == 26) {
            if (destination->x <= 28 || destination->x >= 2652) {
                destination->x = entity->x;
                entity->flags |= 16;
            }
            if (destination->y <= 48 || destination->y >= 1736) {
                destination->y = entity->y;
                entity->flags |= 32;
            }
        }
        if (property != 0x80) {
            sprite = (entity->sprite_state & 3) + 48;
            SetSprite(entity, sprite);
            entity->velocity_x = 0;
            entity->velocity_y = 0;
            entity->movement_lock = 0;
        }
        property = 0x50;
    }
    if (property <= 0x50 || entity->elevation > 0) {
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
            entity->flags |= 48;
            TOEJAM_JUMP_COUNTER[player] = 0;
        }
    }
}

/* StopPlayer (name from RyanFae's disassembly) leaves the first six bytes alone and
 * clears two interleaved groups of three 16-bit components, in component
 * order. [INFERENCE] They are the motion components of a player record. */
struct resettable_vectors {
    unsigned char header[6];
    short first[3];
    short second[3];
};

void StopPlayer(state)
struct resettable_vectors *state;
{
    state->first[0] = 0;
    state->second[0] = 0;
    state->first[1] = 0;
    state->second[1] = 0;
    state->first[2] = 0;
    state->second[2] = 0;
}

/* Process callbacks installed by SetupPlayer for the two player records. */
void func_00e400(direction)
char *direction;
{
    func_00dd82(0, direction);
}

void func_00e410(direction)
char *direction;
{
    func_00dd82(1, direction);
}
