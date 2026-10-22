#include "core_calls.h"
#include "intro_object_calls.h"
#include "ram_intro.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Release ten intro objects, then refresh the sprite state. */
void func_028b20()
{
    short index, object_number;

    for (index = 0; index < 10; ++index) {
        if (object_number = ram_ffe3ce[index])
            func_00d970(object_number);
        ram_ffe3ce[index] = 0;
    }
    VBLANK_FLAG_2 = 0;
    func_00cc82();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
}