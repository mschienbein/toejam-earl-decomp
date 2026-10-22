#include "entities.h"
#include "earthlings.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "present_calls.h"
#include "ram_player.h"
#include "ram_system.h"

/* [INFERENCE] Return -1 after consuming a type-18 collision, otherwise choose
 * whether to move from state 6/7 or the low bit of the next random value. */
char func_0196b0(earthling)
struct earthling_state *earthling;
{
    struct object_state *object;
    short collision;

    if (earthling->kind == 18 && earthling->object &&
        (object = &gPlayerInfo[earthling->object],
         collision = func_025f42(object, object->x, object->y, 0))) {
        func_015da6(collision - 1);
        return -1;
    }
    if (earthling->state == 6)
        return 0;
    if (earthling->state == 7)
        return 1;
    if (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
        (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
        return 0;
    return 1;
}
