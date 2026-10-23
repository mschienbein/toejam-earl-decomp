#include "entities.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Release the two ending-character objects. */
void EndingReleaseActivePlayerEntities()
{
    if (gEndingPlayer1Entity)
        FreeEntity(gEndingPlayer1Entity);
    gEndingPlayer1Entity = 0;
    if (gEndingPlayer2Entity)
        FreeEntity(gEndingPlayer2Entity);
    gEndingPlayer2Entity = 0;
}

/* Either live handle enables the query, but both branches index the first.
 * MATCHING QUALIFICATION: duplicate the first-handle pointer selection
 * before the shared frame read. Combined guards, early returns and duplicate
 * return expressions change output; this does not repair the handle choice. */
short EndingGetFirstPlayerAnimationFrame()
{
    struct object_state *object;
    if (gEndingPlayer1Entity)
        object = &gPlayerInfo[gEndingPlayer1Entity];
    else if (gEndingPlayer2Entity)
        object = &gPlayerInfo[gEndingPlayer1Entity];
    else
        return 0;
    return object->frame;
}
