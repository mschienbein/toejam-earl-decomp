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
