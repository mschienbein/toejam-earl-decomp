#include "core_calls.h"
#include "ram_camera.h"

/* Store the high bit of the flag, or clear the word. */
void SetCameraTilePriorityEnabled(enabled)
short enabled;
{
    if (enabled)
        gCameraTileAttributes = 0x8000;
    else
        gCameraTileAttributes = 0;
}
