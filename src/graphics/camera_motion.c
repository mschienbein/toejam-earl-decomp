#include "core_calls.h"
#include "graphics_calls.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "ram_system.h"

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
