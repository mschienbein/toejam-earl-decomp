#include "graphics_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_player.h"

/* Copy the selected camera coordinates into their 24.8 accumulators. */
void SetCameraPosition(player, position_x, position_y)
int player;
long position_x, position_y;
{
    if (gPlayerLives[0] < 0)
        player = 0;
    SetCameraViewOrigin(player, position_x, position_y);
    if (player == 0) {
        gCamera1FollowXFixed = (long)TOEJAM_CAMERA_NEWX << 8;
        gCamera1FollowYFixed = (long)TOEJAM_CAMERA_NEWY << 8;
    } else {
        gCamera2FollowXFixed = (long)EARL_CAMERA_NEWX << 8;
        gCamera2FollowYFixed = (long)EARL_CAMERA_NEWY << 8;
    }
}
