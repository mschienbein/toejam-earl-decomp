#include "intro_object_calls.h"
#include "ram_intro.h"
#include "sprite_calls.h"

/* Release exhaust before the rocketship object. */
void func_028c4c()
{
    if (ram_ffe3e0)
        func_00d970(ram_ffe3e0);
    ram_ffe3e0 = 0;
    if (ram_ffe3de)
        func_00d970(ram_ffe3de);
    ram_ffe3de = 0;
}