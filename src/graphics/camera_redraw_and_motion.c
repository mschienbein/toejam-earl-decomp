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
        if (gCameraColumnUploadPending[camera]) {
            start_row = gCameraColumnUploadStartRow[camera];
            count = gCameraColumnUploadWordCount[camera];
            if (camera == 0)
                VDP_COMMAND = 0x60000002L + (start_row << 23) +
                    (gCameraColumnUploadVramColumn[camera] << 17);
            else
                VDP_COMMAND = 0x40000003L + (start_row << 23) +
                    (gCameraColumnUploadVramColumn[camera] << 17);
            if (start_row + count <= 32)
                MoveDataToVRAM(gCameraColumnUploadWords[camera], count);
            else {
                first_count = 32 - start_row;
                MoveDataToVRAM(gCameraColumnUploadWords[camera], first_count);
                if (camera == 0)
                    VDP_COMMAND = 0x60000002L + (gCameraColumnUploadVramColumn[camera] << 17);
                else
                    VDP_COMMAND = 0x40000003L + (gCameraColumnUploadVramColumn[camera] << 17);
                MoveDataToVRAM(
                    &gCameraColumnUploadWords[camera][first_count], count - first_count);
            }
            gCameraColumnUploadPending[camera] = 0;
        }
    }
    VDP_CONTROL_WORD = 0x8f02;
}

/* Draw every buffered row for one camera, then restore its row cursor. */
void RedrawCameraTileRows(camera)
short camera;
{
    short end_row;

    if (camera == 0) {
        end_row = gCamera1TileRowCursor + gCameraColumnUploadHeight;
        while (end_row > gCamera1TileRowCursor) {
            BuildCameraTileRow(1, 0);
            UpdateCameraY();
            gCamera1TileRowCursor++;
        }
        gCamera1TileRowCursor = end_row - gCameraColumnUploadHeight;
        gCamera1PendingTileDirections = 0;
    } else {
        end_row = gCamera2TileRowCursor + gCameraColumnUploadHeight;
        while (end_row > gCamera2TileRowCursor) {
            BuildCameraTileRow(1, 1);
            UpdateCameraY();
            gCamera2TileRowCursor++;
        }
        gCamera2TileRowCursor = end_row - gCameraColumnUploadHeight;
        gCamera2PendingTileDirections = 0;
    }
    gCameraRowUploadPending[camera] = 0;
    gCameraColumnUploadPending[camera] = 0;
}

/* Clamp camera coordinates and redraw the selected viewport. */
void SetCameraViewOrigin(camera, position_x, position_y)
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
        gCameraColumnUploadHeight = 19;
    } else {
        if (position_y > 1560)
            position_y = 1560;
        gCameraColumnUploadHeight = 32;
    }
    if (camera == 0) {
        TOEJAM_CAMERA_NEWX = position_x;
        TOEJAM_CAMERA_NEWY = position_y;
        gCamera1TileColumnCursor = (short)((int)position_x >> 3) - 1;
        gCamera1TileRowCursor = (short)((int)position_y >> 3) - 1;
    } else {
        EARL_CAMERA_NEWX = position_x;
        EARL_CAMERA_NEWY = position_y;
        gCamera2TileColumnCursor = (short)((int)position_x >> 3) - 1;
        gCamera2TileRowCursor = (short)((int)position_y >> 3) - 1;
    }
    {
        /* Compiler-shape note: the named int argument retains the original
         * in-place promotion of the exhausted short camera selector. */
        int selected_camera;
        selected_camera = camera;
        RedrawCameraTileRows(selected_camera);
    }
    if (gPlayerLives[0] < 0) {
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    }
}

/* Build rows and columns requested by the four camera-direction bits. */
void BuildPendingCameraTiles()
{
    char directions;

    directions = gCamera1PendingTileDirections & 5;
    if (directions && directions != 5) {
        gCamera1TileRowCursor = gCamera1PendingTileRow;
        BuildCameraTileRow(directions & 1, 0);
    }
    directions = gCamera1PendingTileDirections & 10;
    if (directions && directions != 10) {
        gCamera1TileColumnCursor = TOEJAM_CAMERA_X;
        BuildCameraTileColumn(directions & 8, 0);
    }
    gCamera1PendingTileDirections = 0;
    directions = gCamera2PendingTileDirections & 5;
    if (directions && directions != 5) {
        gCamera2TileRowCursor = gCamera2PendingTileRow;
        BuildCameraTileRow(directions & 1, 1);
    }
    directions = gCamera2PendingTileDirections & 10;
    if (directions && directions != 10) {
        gCamera2TileColumnCursor = EARL_CAMERA_X;
        BuildCameraTileColumn(directions & 8, 1);
    }
    gCamera2PendingTileDirections = 0;
}

/* Flush one camera's pending row while preserving the other camera's flag. */
void TryUpdateCameraY(camera)
int camera;
{
    char other_pending;

    other_pending = gCameraRowUploadPending[camera ^ 1];
    gCameraRowUploadPending[camera ^ 1] = 0;
    VBLANK_FLAG_6 = 1;
    VBLANK_FLAG_3 = 1;
    WaitForNextVBlank(1);
    ClearSkipVHBlankFlags();
    UpdateCameraY();
    gCameraRowUploadPending[camera ^ 1] = other_pending;
    SetSkipVHBlankFlags();
    VBLANK_FLAG_3 = 0;
    VBLANK_FLAG_6 = 0;
}

/* Flush one camera's pending column without consuming the other's flag. */
void TryUpdateCameraX(camera)
int camera;
{
    char other_pending;

    other_pending = gCameraColumnUploadPending[camera ^ 1];
    gCameraColumnUploadPending[camera ^ 1] = 0;
    VBLANK_FLAG_6 = 1;
    VBLANK_FLAG_3 = 1;
    WaitForNextVBlank(1);
    ClearSkipVHBlankFlags();
    UpdateCameraX();
    gCameraColumnUploadPending[camera ^ 1] = other_pending;
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
            if (new_row != gCamera1TileRowCursor) {
                if (gCameraRowUploadPending[0])
                    TryUpdateCameraY(0);
                gCamera1PendingTileDirections |= 1;
                gCamera1PendingTileRow = new_row;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWY > 8) {
            EARL_CAMERA_NEWY--;
            new_row = (EARL_CAMERA_NEWY >> 3) - 1;
            if (new_row != gCamera2TileRowCursor) {
                if (gCameraRowUploadPending[1])
                    TryUpdateCameraY(1);
                gCamera2PendingTileDirections |= 1;
                gCamera2PendingTileRow = new_row;
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
            if (new_row != gCamera1TileRowCursor) {
                if (gCameraRowUploadPending[0])
                    TryUpdateCameraY(0);
                gCamera1PendingTileDirections |= 4;
                gCamera1PendingTileRow = new_row;
            }
        } else
            return 1;
    } else {
        if ((SPLIT_SCREEN && EARL_CAMERA_NEWY < 1672) || EARL_CAMERA_NEWY < 1560) {
            EARL_CAMERA_NEWY++;
            new_row = (EARL_CAMERA_NEWY >> 3) - 1;
            if (new_row != gCamera2TileRowCursor) {
                if (gCameraRowUploadPending[1])
                    TryUpdateCameraY(1);
                gCamera2PendingTileDirections |= 4;
                gCamera2PendingTileRow = new_row;
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
            if (new_column != gCamera1TileColumnCursor) {
                if (gCameraColumnUploadPending[0])
                    TryUpdateCameraX(0);
                gCamera1PendingTileDirections |= 8;
                TOEJAM_CAMERA_X = new_column;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWX > 8) {
            EARL_CAMERA_NEWX--;
            new_column = (EARL_CAMERA_NEWX >> 3) - 1;
            if (new_column != gCamera2TileColumnCursor) {
                if (gCameraColumnUploadPending[1])
                    TryUpdateCameraX(1);
                gCamera2PendingTileDirections |= 8;
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
            if (new_column != gCamera1TileColumnCursor) {
                if (gCameraColumnUploadPending[0])
                    TryUpdateCameraX(0);
                gCamera1PendingTileDirections |= 2;
                TOEJAM_CAMERA_X = new_column;
            }
        } else
            return 1;
    } else {
        if (EARL_CAMERA_NEWX < 2360) {
            EARL_CAMERA_NEWX++;
            new_column = (EARL_CAMERA_NEWX >> 3) - 1;
            if (new_column != gCamera2TileColumnCursor) {
                if (gCameraColumnUploadPending[1])
                    TryUpdateCameraX(1);
                gCamera2PendingTileDirections |= 2;
                EARL_CAMERA_X = new_column;
            }
        } else
            return 1;
    }
    if (gPlayerLives[0] < 0)
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    return 0;
}
