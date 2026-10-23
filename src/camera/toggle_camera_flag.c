#include "hardware.h"
#include "core_calls.h"
#include "ram_camera.h"

/* Store the high bit of the flag, or clear the word. */
void SetCameraTilePriorityEnabled(enabled)
short enabled;
{
    if (enabled)
        gCameraTileAttributes = VDP_TILE_PRIORITY;
    else
        gCameraTileAttributes = 0;
}
