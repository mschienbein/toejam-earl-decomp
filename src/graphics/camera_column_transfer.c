#include "hardware.h"
#include "ram.h"
#include "graphics_calls.h"
#include "romcalls.h"
/* Transfer pending vertical columns using a 128-byte VRAM increment. */
void UpdateCameraX()
{
    short first_count;
    short camera, start_row;
    char count;

    VDP_CONTROL_WORD = 0x8f80;
    for (camera = 0; camera < 2; camera++) {
        if (ram_ff818e[camera]) {
            start_row = ram_ff818a[camera];
            count = ram_ff8190[camera];
            if (camera == 0)
                VDP_COMMAND = 0x60000002L + (start_row << 23) +
                    (ram_ff818c[camera] << 17);
            else
                VDP_COMMAND = 0x40000003L + (start_row << 23) +
                    (ram_ff818c[camera] << 17);
            if (start_row + count <= 32)
                MoveDataToVRAM(ram_ff810a[camera], count);
            else {
                first_count = 32 - start_row;
                MoveDataToVRAM(ram_ff810a[camera], first_count);
                if (camera == 0)
                    VDP_COMMAND = 0x60000002L + (ram_ff818c[camera] << 17);
                else
                    VDP_COMMAND = 0x40000003L + (ram_ff818c[camera] << 17);
                MoveDataToVRAM(&ram_ff810a[camera][first_count], count - first_count);
            }
            ram_ff818e[camera] = 0;
        }
    }
    VDP_CONTROL_WORD = 0x8f02;
}

/* Draw every buffered row for one camera, then restore its row cursor. */
void func_00200a(camera)
short camera;
{
    short end_row;

    if (camera == 0) {
        end_row = ram_ff803e + ram_ff8198;
        while (end_row > ram_ff803e) {
            func_0016d2(1, 0);
            UpdateCameraY();
            ram_ff803e++;
        }
        ram_ff803e = end_row - ram_ff8198;
        ram_ff8192 = 0;
    } else {
        end_row = ram_ff8042 + ram_ff8198;
        while (end_row > ram_ff8042) {
            func_0016d2(1, 1);
            UpdateCameraY();
            ram_ff8042++;
        }
        ram_ff8042 = end_row - ram_ff8198;
        ram_ff8194 = 0;
    }
    ram_ff8106[camera] = 0;
    ram_ff818e[camera] = 0;
}
