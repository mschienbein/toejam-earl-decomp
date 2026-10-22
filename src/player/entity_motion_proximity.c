#include "entities.h"
#include "player.h"
#include "romcalls.h"
/* Require the same level and absolute x/y differences below 16 and 5. */
int func_00dcc2(entity, x, y, level)
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
int func_00dd1e(entity, other, distance_x, distance_y)
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
