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
