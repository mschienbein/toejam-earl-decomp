#include "ram.h"
#include "engine_calls.h"
/* Test whether an ordinary object is held by one of the four effect slots. */
int func_01dee8(object_number)
short object_number;
{
    short slot;

    if (object_number >= 6) {
        for (slot = 0; slot < 4; ++slot)
            if (ram_ffe1ee[slot] == object_number)
                return 1;
    }
    return 0;
}
