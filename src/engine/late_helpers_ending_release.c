#include "entities.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Release the two ending-character objects. */
void func_02899c()
{
    if (ram_ffe3aa)
        func_00d970(ram_ffe3aa);
    ram_ffe3aa = 0;
    if (ram_ffe3ac)
        func_00d970(ram_ffe3ac);
    ram_ffe3ac = 0;
}

/* Either live handle enables the query, but both branches index the first.
 * Preserve the duplicate pointer selections: MCC68K shares their later frame
 * read, unlike a combined guard or duplicate return expressions. */
short func_0289da()
{
    struct object_state *object;
    if (ram_ffe3aa)
        object = &gPlayerInfo[ram_ffe3aa];
    else if (ram_ffe3ac)
        object = &gPlayerInfo[ram_ffe3aa];
    else
        return 0;
    return object->frame;
}
