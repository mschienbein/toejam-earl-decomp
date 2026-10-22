#include "hardware.h"
#include "core_calls.h"
#include "graphics_calls.h"
#include "hardware_calls.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "ram_system.h"

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

/* Clamp camera coordinates and redraw the selected viewport. */
void func_0020aa(camera, position_x, position_y)
short camera, position_x, position_y;
{
    if (gPlayerLives[0] < 0)
        camera = 0;
    if (position_x < 8)
        position_x = 8;
    if (position_x > 2360)
        position_x = 2360;
    if (position_y < 8)
        position_y = 8;
    if (SPLIT_SCREEN) {
        if (position_y > 1672)
            position_y = 1672;
        ram_ff8198 = 19;
    } else {
        if (position_y > 1560)
            position_y = 1560;
        ram_ff8198 = 32;
    }
    if (camera == 0) {
        TOEJAM_CAMERA_NEWX = position_x;
        TOEJAM_CAMERA_NEWY = position_y;
        ram_ff8040 = (short)((int)position_x >> 3) - 1;
        ram_ff803e = (short)((int)position_y >> 3) - 1;
    } else {
        EARL_CAMERA_NEWX = position_x;
        EARL_CAMERA_NEWY = position_y;
        ram_ff8044 = (short)((int)position_x >> 3) - 1;
        ram_ff8042 = (short)((int)position_y >> 3) - 1;
    }
    {
        /* Compiler-shape note: the named int argument retains the original
         * in-place promotion of the exhausted short camera selector. */
        int selected_camera;
        selected_camera = camera;
        func_00200a(selected_camera);
    }
    if (gPlayerLives[0] < 0) {
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    }
}

/* Build rows and columns requested by the four camera-direction bits. */
void func_00219c()
{
    char directions;

    directions = ram_ff8192 & 5;
    if (directions && directions != 5) {
        ram_ff803e = ram_ff8046;
        func_0016d2(directions & 1, 0);
    }
    directions = ram_ff8192 & 10;
    if (directions && directions != 10) {
        ram_ff8040 = TOEJAM_CAMERA_X;
        func_001ba8(directions & 8, 0);
    }
    ram_ff8192 = 0;
    directions = ram_ff8194 & 5;
    if (directions && directions != 5) {
        ram_ff8042 = ram_ff804a;
        func_0016d2(directions & 1, 1);
    }
    directions = ram_ff8194 & 10;
    if (directions && directions != 10) {
        ram_ff8044 = EARL_CAMERA_X;
        func_001ba8(directions & 8, 1);
    }
    ram_ff8194 = 0;
}

/* Flush one camera's pending row while preserving the other camera's flag. */
void TryUpdateCameraY(camera)
int camera;
{
    char other_pending;

    other_pending = ram_ff8106[camera ^ 1];
    ram_ff8106[camera ^ 1] = 0;
    VBLANK_FLAG_6 = 1;
    VBLANK_FLAG_3 = 1;
    WaitForNextVBlank(1);
    ClearSkipVHBlankFlags();
    UpdateCameraY();
    ram_ff8106[camera ^ 1] = other_pending;
    SetSkipVHBlankFlags();
    VBLANK_FLAG_3 = 0;
    VBLANK_FLAG_6 = 0;
}

/* Flush one camera's pending column without consuming the other's flag. */
void TryUpdateCameraX(camera)
int camera;
{
    char other_pending;

    other_pending = ram_ff818e[camera ^ 1];
    ram_ff818e[camera ^ 1] = 0;
    VBLANK_FLAG_6 = 1;
    VBLANK_FLAG_3 = 1;
    WaitForNextVBlank(1);
    ClearSkipVHBlankFlags();
    UpdateCameraX();
    ram_ff818e[camera ^ 1] = other_pending;
    SetSkipVHBlankFlags();
    VBLANK_FLAG_3 = 0;
    VBLANK_FLAG_6 = 0;
}

/* Move upward one pixel and request a new row when crossing a tile edge. */
short MoveCameraUp(camera)
short camera;
{
    short new_row;

    if (gPlayerLives[0] < 0)
        camera = 0;
    if (camera == 0) {
        if (TOEJAM_CAMERA_NEWY > 8) {
            TOEJAM_CAMERA_NEWY--;
            new_row = (TOEJAM_CAMERA_NEWY >> 3) - 1;
            if (new_row != ram_ff803e) {
                if (ram_ff8106[0])
                    TryUpdateCameraY(0);
                ram_ff8192 |= 1;
                ram_ff8046 = new_row;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWY > 8) {
            EARL_CAMERA_NEWY--;
            new_row = (EARL_CAMERA_NEWY >> 3) - 1;
            if (new_row != ram_ff8042) {
                if (ram_ff8106[1])
                    TryUpdateCameraY(1);
                ram_ff8194 |= 1;
                ram_ff804a = new_row;
            }
        } else
            return 1;
    }
    if (gPlayerLives[0] < 0)
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    return 0;
}

/* Move downward one pixel within the full- or split-screen Y limit. */
short MoveCameraDown(camera)
short camera;
{
    short new_row;

    if (gPlayerLives[0] < 0)
        camera = 0;
    if (camera == 0) {
        if ((SPLIT_SCREEN && TOEJAM_CAMERA_NEWY < 1672) || TOEJAM_CAMERA_NEWY < 1560) {
            TOEJAM_CAMERA_NEWY++;
            new_row = (TOEJAM_CAMERA_NEWY >> 3) - 1;
            if (new_row != ram_ff803e) {
                if (ram_ff8106[0])
                    TryUpdateCameraY(0);
                ram_ff8192 |= 4;
                ram_ff8046 = new_row;
            }
        } else
            return 1;
    } else {
        if ((SPLIT_SCREEN && EARL_CAMERA_NEWY < 1672) || EARL_CAMERA_NEWY < 1560) {
            EARL_CAMERA_NEWY++;
            new_row = (EARL_CAMERA_NEWY >> 3) - 1;
            if (new_row != ram_ff8042) {
                if (ram_ff8106[1])
                    TryUpdateCameraY(1);
                ram_ff8194 |= 4;
                ram_ff804a = new_row;
            }
        } else
            return 1;
    }
    if (gPlayerLives[0] < 0)
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    return 0;
}

/* Move left one pixel and request a column at a tile boundary. */
short MoveCameraLeft(camera)
short camera;
{
    short new_column;

    if (gPlayerLives[0] < 0)
        camera = 0;
    if (camera == 0) {
        if (TOEJAM_CAMERA_NEWX > 8) {
            TOEJAM_CAMERA_NEWX--;
            new_column = (TOEJAM_CAMERA_NEWX >> 3) - 1;
            if (new_column != ram_ff8040) {
                if (ram_ff818e[0])
                    TryUpdateCameraX(0);
                ram_ff8192 |= 8;
                TOEJAM_CAMERA_X = new_column;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWX > 8) {
            EARL_CAMERA_NEWX--;
            new_column = (EARL_CAMERA_NEWX >> 3) - 1;
            if (new_column != ram_ff8044) {
                if (ram_ff818e[1])
                    TryUpdateCameraX(1);
                ram_ff8194 |= 8;
                EARL_CAMERA_X = new_column;
            }
        } else
            return 1;
    }
    if (gPlayerLives[0] < 0)
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    return 0;
}

/* Move right one pixel, stopping at world X coordinate 2360. */
short MoveCameraRight(camera)
short camera;
{
    short new_column;

    if (gPlayerLives[0] < 0)
        camera = 0;
    if (camera == 0) {
        if (TOEJAM_CAMERA_NEWX < 2360) {
            TOEJAM_CAMERA_NEWX++;
            new_column = (TOEJAM_CAMERA_NEWX >> 3) - 1;
            if (new_column != ram_ff8040) {
                if (ram_ff818e[0])
                    TryUpdateCameraX(0);
                ram_ff8192 |= 2;
                TOEJAM_CAMERA_X = new_column;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWX < 2360) {
            EARL_CAMERA_NEWX++;
            new_column = (EARL_CAMERA_NEWX >> 3) - 1;
            if (new_column != ram_ff8044) {
                if (ram_ff818e[1])
                    TryUpdateCameraX(1);
                ram_ff8194 |= 2;
                EARL_CAMERA_X = new_column;
            }
        } else
            return 1;
    }
    if (gPlayerLives[0] < 0)
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    return 0;
}
