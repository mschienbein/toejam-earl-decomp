#include "intro_object_calls.h"
#include "ram_intro.h"
#include "sprite_calls.h"

/* Release the collision object and the two three-object lists. */
void func_0287c0()
{
    unsigned short index;

    if (ram_ffe3ba)
        func_00d970(ram_ffe3ba);
    ram_ffe3ba = 0;
    for (index = 0; index < 3; ++index) {
        if (ram_ffe3bc[index])
            func_00d970(ram_ffe3bc[index]);
        ram_ffe3bc[index] = 0;
        if (ram_ffe3c0[index])
            func_00d970(ram_ffe3c0[index]);
        ram_ffe3c0[index] = 0;
    }
}