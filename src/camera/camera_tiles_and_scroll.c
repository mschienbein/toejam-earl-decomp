#include "assets_map.h"
#include "graphics_calls.h"
#include "ram_camera.h"

/* Fill one 45-word transfer row and mark it ready. */
void ClearCameraTileRow(camera)
short camera;
{
    short column;

    for (column = 0; column < 45; column++)
        gCameraRowUploadWords[camera][column] = gCameraTileAttributes | 0;
    gCameraRowUploadWordCount[camera] = 45;
    gCameraRowUploadPending[camera] = 1;
}

/* Read a background metatile word; bit 7 requests a horizontal mirror. */
unsigned short GetLevelMetatileTile(metatile, row, column)
unsigned short metatile, row, column;
{
    short tile;

    if (metatile & 0x80) {
        metatile &= 0x7f;
        tile = LevelMetatileMaps[metatile][row][15 - column] ^ 0x800;
    } else
        tile = LevelMetatileMaps[metatile][row][column];
    return tile;
}

/* Read an overlay word, retaining zero when mirroring an empty tile. */
unsigned long GetOverlayMetatileTile(metatile, row, column)
unsigned short metatile, row, column;
{
    unsigned short tile;

    if (metatile & 0x10) {
        metatile &= 15;
        metatile--;
        tile = OverlayMetatileMaps[metatile][row][15 - column];
        if (tile)
            tile ^= 0x800;
    } else {
        metatile--;
        tile = OverlayMetatileMaps[metatile][row][column];
    }
    return tile;
}


#include "ram_camera.h"
#include "ram_map.h"
#include "graphics_calls.h"
/* Prepare the entering horizontal tile row from the selected level map. */
/* MATCHING QUALIFICATION: the five discarded sizeof-address observations
 * select directly allocated stack words; the fixed map index remains the sole
 * post-allocation spill. Reverse declaration order gives the original six slots.
 * Removing any observation fails. The explicit int promotion of the unsigned-
 * short metatile result prevents an otherwise redundant MOVE.W self-normalizer.
 * Masks are split into word assignments/updates to retain word operations on
 * addressed locals; final OR operands retain the measured scratch ownership.
 * These are compiler-coaxing spellings, not claimed historical source. */
void BuildCameraTileRow(top_edge, camera)
short top_edge;
short camera;
{
  short world_column;
  short map_x;
  short screen_row;
  char blank = 0;
  char wraps = 0;
  unsigned short map_cell;
  unsigned char metatile;
  unsigned short *map;
  short transfer_count;
  short tile_column;
  short tile_row;
  unsigned short tile;
  short map_row;
  sizeof(&tile);
  sizeof(&tile_row);
  sizeof(&tile_column);
  sizeof(&transfer_count);
  sizeof(&world_column);
  if (camera == 0)
  {
    if (top_edge)
    {
      screen_row = gCamera1TileRowCursor & 31;
      tile_row = gCamera1TileRowCursor;
      tile_row &= 7;
      map_row = gCamera1TileRowCursor;
      map_row = map_row >> 3;
    }
    else
    {
      screen_row = (gCamera1TileRowCursor + gCameraColumnUploadHeight) - 3;
      if (screen_row >= 224)
        return;
      tile_row = screen_row;
      tile_row &= 7;
      map_row = screen_row;
      map_row = map_row >> 3;
      screen_row &= 31;
    }
    map = &gMapDataA[0][0];
    if ((gLevel912C == 1) || (gLevel912C == 26))
      wraps = 1;
    gCamera1TileColumnCursor--;
    gCameraRowUploadStartColumn[0] = gCamera1TileColumnCursor & 63;
    world_column = gCamera1TileColumnCursor;
    gCamera1TileColumnCursor++;
  }
  else
  {
    if (top_edge)
    {
      screen_row = gCamera2TileRowCursor & 31;
      tile_row = gCamera2TileRowCursor;
      tile_row &= 7;
      map_row = gCamera2TileRowCursor;
      map_row = map_row >> 3;
    }
    else
    {
      screen_row = (gCamera2TileRowCursor + gCameraColumnUploadHeight) - 3;
      if (screen_row >= 224)
        return;
      tile_row = screen_row;
      tile_row &= 7;
      map_row = screen_row;
      map_row = map_row >> 3;
      screen_row &= 31;
    }
    if ((gLevel912E == 1) || (gLevel912E == 26))
      wraps = 1;
    gCamera2TileColumnCursor--;
    gCameraRowUploadStartColumn[1] = gCamera2TileColumnCursor & 63;
    world_column = gCamera2TileColumnCursor;
    gCamera2TileColumnCursor++;
    map = &gMapDataB[0][0];
  }
  gCameraRowUploadVramRow[camera] = screen_row;
  if ((map_row == 0) || (map_row > 26))
  {
    if (!wraps)
    {
      ClearCameraTileRow(camera);
      return;
    }
    blank = 1;
  }
  map_row--;
  for (transfer_count = 0; (transfer_count < 45) && (world_column < 336); transfer_count++, world_column++)
  {
    tile_column = world_column;
    tile_column &= 15;
    map_x = world_column >> 4;
    tile = 0;
    if (blank)
      metatile = 82;
    else
      if ((map_x == 0) || (map_x > 19))
    {
      if (!wraps)
      {
        gCameraRowUploadWords[camera][transfer_count] = gCameraTileAttributes | ((unsigned short) 0);
        continue;
      }
      metatile = 82;
    }
    else
    {
      map_x--;
      map_cell = map[(map_row * 19) + map_x];
      metatile = (unsigned char) ((map_cell & 0x1f00) >> 8);
      if (metatile)
        tile = GetOverlayMetatileTile(metatile, tile_row, tile_column);
      if (!tile)
        metatile = map_cell;
    }
    if (!tile)
      tile = (int) GetLevelMetatileTile(metatile, tile_row, tile_column);
    gCameraRowUploadWords[camera][transfer_count] = tile | gCameraTileAttributes;
  }

  gCameraRowUploadWordCount[camera] = 45;
  gCameraRowUploadPending[camera] = 1;
}




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
        if (gCameraRowUploadPending[camera]) {
            start_column = gCameraRowUploadStartColumn[camera];
            count = gCameraRowUploadWordCount[camera];
            if (camera == 0)
                VDP_COMMAND = VDP_WRITE_PLANE_A + (gCameraRowUploadVramRow[camera] << 23) +
                    (start_column << 17);
            else
                VDP_COMMAND = 0x40000003L + (gCameraRowUploadVramRow[camera] << 23) +
                    (start_column << 17);
            if (start_column + count <= 64)
                MoveDataToVRAM(gCameraRowUploadWords[camera], count);
            else {
                first_count = 64 - start_column;
                MoveDataToVRAM(gCameraRowUploadWords[camera], first_count);
                if (camera == 0)
                    VDP_COMMAND = VDP_WRITE_PLANE_A + (gCameraRowUploadVramRow[camera] << 23);
                else
                    VDP_COMMAND = 0x40000003L + (gCameraRowUploadVramRow[camera] << 23);
                MoveDataToVRAM(
                    &gCameraRowUploadWords[camera][first_count], count - first_count);
            }
            gCameraRowUploadPending[camera] = 0;
        }
    }
}

/* Fill the selected vertical transfer buffer and mark it ready. */
void ClearCameraTileColumn(camera)
short camera;
{
    short row;

    for (row = 0; row < gCameraColumnUploadHeight; row++)
        gCameraColumnUploadWords[camera][row] = gCameraTileAttributes | 0;
    gCameraColumnUploadPending[camera] = 1;
    gCameraColumnUploadWordCount[camera] = gCameraColumnUploadHeightLowByte;
}


#include "ram_camera.h"
#include "ram_map.h"
#include "graphics_calls.h"

/* Prepare the entering vertical tile column from the selected level map. */
/* MATCHING QUALIFICATION: the five discarded sizeof-address observations
 * select directly allocated stack words; the fixed map index remains the sole
 * post-allocation spill. Reverse declaration order gives the original six slots.
 * Removing any observation fails. The explicit int promotion of the unsigned-
 * short metatile result prevents an otherwise redundant MOVE.W self-normalizer.
 * Masks are split into word assignments/updates to retain word operations on
 * addressed locals; final OR operands retain the measured scratch ownership.
 * These are compiler-coaxing spellings, not claimed historical source. */
void BuildCameraTileColumn(left_edge, camera)
short left_edge;
short camera;
{
  short world_row;
  short transfer_count;
  short tile_column;
  short tile_row;
  unsigned short tile;
  short map_x;
  short map_row;
  short screen_column;
  char blank = 0;
  char wraps = 0;
  unsigned short map_cell;
  unsigned char metatile;
  unsigned short *map;
  sizeof(&tile);
  sizeof(&tile_row);
  sizeof(&tile_column);
  sizeof(&transfer_count);
  sizeof(&world_row);
  if (camera == 0)
  {
    if (left_edge)
    {
      screen_column = gCamera1TileColumnCursor & 63;
      tile_column = gCamera1TileColumnCursor;
      tile_column &= 15;
      map_x = gCamera1TileColumnCursor;
      map_x >>= 4;
    }
    else
    {
      screen_column = gCamera1TileColumnCursor + 42;
      if (screen_column >= 336)
        return;
      tile_column = screen_column;
      tile_column &= 15;
      map_x = screen_column;
      map_x >>= 4;
      screen_column &= 63;
    }
    gCamera1TileRowCursor--;
    gCameraColumnUploadStartRow[camera] = gCamera1TileRowCursor & 31;
    world_row = gCamera1TileRowCursor;
    gCamera1TileRowCursor++;
    map = &gMapDataA[0][0];
  }
  else
  {
    if (left_edge)
    {
      screen_column = gCamera2TileColumnCursor & 63;
      tile_column = gCamera2TileColumnCursor;
      tile_column &= 15;
      map_x = gCamera2TileColumnCursor;
      map_x >>= 4;
    }
    else
    {
      screen_column = gCamera2TileColumnCursor + 42;
      if (screen_column >= 336)
        return;
      tile_column = screen_column;
      tile_column &= 15;
      map_x = screen_column;
      map_x >>= 4;
      screen_column &= 63;
    }
    gCamera2TileRowCursor--;
    gCameraColumnUploadStartRow[camera] = gCamera2TileRowCursor & 31;
    world_row = gCamera2TileRowCursor;
    gCamera2TileRowCursor++;
    map = &gMapDataB[0][0];
  }
  gCameraColumnUploadVramColumn[camera] = screen_column;
  if (((camera == 0) && ((gLevel912C == 1) || (gLevel912C == 26))) || ((camera == 1) && ((gLevel912E == 1) || (gLevel912E == 26))))
    wraps = 1;
  if ((map_x == 0) || (map_x > 19))
  {
    if (!wraps)
    {
      ClearCameraTileColumn(camera);
      return;
    }
    blank = 1;
  }
  map_x--;
  for (transfer_count = 0; (gCameraColumnUploadHeight > transfer_count) && (world_row < 224); transfer_count++, world_row++)
  {
    tile_row = world_row;
    tile_row &= 7;
    map_row = world_row >> 3;
    tile = 0;
    if (blank)
      metatile = 82;
    else
      if ((map_row == 0) || (map_row > 26))
    {
      if (!wraps)
      {
        gCameraColumnUploadWords[camera][transfer_count] = gCameraTileAttributes | 0;
        continue;
      }
      metatile = 82;
    }
    else
    {
      map_row--;
      map_cell = map[(map_row * 19) + map_x];
      metatile = (unsigned char) ((map_cell & 0x1f00) >> 8);
      if (metatile)
        tile = GetOverlayMetatileTile(metatile, tile_row, tile_column);
      if (!tile)
        metatile = map_cell;
    }
    if (!tile)
      tile = (int) GetLevelMetatileTile(metatile, tile_row, tile_column);
    gCameraColumnUploadWords[camera][transfer_count] = tile | gCameraTileAttributes;
  }

  gCameraColumnUploadWordCount[camera] = gCameraColumnUploadHeightLowByte;
  gCameraColumnUploadPending[camera] = 1;
}




#include "entities.h"
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
                VDP_COMMAND = VDP_WRITE_PLANE_A + (start_row << 23) +
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
                    VDP_COMMAND = VDP_WRITE_PLANE_A + (gCameraColumnUploadVramColumn[camera] << 17);
                else
                    VDP_COMMAND = 0x40000003L + (gCameraColumnUploadVramColumn[camera] << 17);
                MoveDataToVRAM(
                    &gCameraColumnUploadWords[camera][first_count], count - first_count);
            }
            gCameraColumnUploadPending[camera] = 0;
        }
    }
    VDP_CONTROL_WORD = VDP_AUTOINCREMENT_WORD;
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0) {
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

    if (gPlayerLives[PLAYER_TOEJAM] < 0)
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    return 0;
}

/* Move downward one pixel within the full- or split-screen Y limit. */
short MoveCameraDown(camera)
short camera;
{
    short new_row;

    if (gPlayerLives[PLAYER_TOEJAM] < 0)
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
    return 0;
}

/* Move left one pixel and request a column at a tile boundary. */
short MoveCameraLeft(camera)
short camera;
{
    short new_column;

    if (gPlayerLives[PLAYER_TOEJAM] < 0)
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    return 0;
}

/* Move right one pixel, stopping at world X coordinate 2360. */
short MoveCameraRight(camera)
short camera;
{
    short new_column;

    if (gPlayerLives[PLAYER_TOEJAM] < 0)
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
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
    return 0;
}

