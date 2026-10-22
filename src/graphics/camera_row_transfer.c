#include "hardware.h"
#include "graphics_calls.h"
#include "hardware_calls.h"
#include "ram_camera.h"

/* Transfer pending horizontal rows, splitting at the 64-column wrap. */
void UpdateCameraY()
{
    short first_count;
    short camera, start_column, count;

    for (camera = 0; camera < 2; camera++) {
        if (ram_ff8106[camera]) {
            start_column = ram_ff8104[camera];
            count = ram_ff8108[camera];
            if (camera == 0)
                VDP_COMMAND = 0x60000002L + (ram_ff8102[camera] << 23) +
                    (start_column << 17);
            else
                VDP_COMMAND = 0x40000003L + (ram_ff8102[camera] << 23) +
                    (start_column << 17);
            if (start_column + count <= 64)
                MoveDataToVRAM(ram_ff804e[camera], count);
            else {
                first_count = 64 - start_column;
                MoveDataToVRAM(ram_ff804e[camera], first_count);
                if (camera == 0)
                    VDP_COMMAND = 0x60000002L + (ram_ff8102[camera] << 23);
                else
                    VDP_COMMAND = 0x40000003L + (ram_ff8102[camera] << 23);
                MoveDataToVRAM(&ram_ff804e[camera][first_count], count - first_count);
            }
            ram_ff8106[camera] = 0;
        }
    }
}

/* Fill the selected vertical transfer buffer and mark it ready. */
void func_001b48(camera)
short camera;
{
    short row;

    for (row = 0; row < ram_ff8198; row++)
        ram_ff810a[camera][row] = ram_ff8196 | 0;
    ram_ff818e[camera] = 1;
    ram_ff8190[camera] = ram_ff8199;
}
