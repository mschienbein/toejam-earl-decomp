#include "graphics_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_player.h"

/* Copy the selected camera coordinates into their 24.8 accumulators. */
void func_002608(player, position_x, position_y)
int player;
long position_x, position_y;
{
    if (gPlayerLives[0] < 0)
        player = 0;
    func_0020aa(player, position_x, position_y);
    if (player == 0) {
        ram_ff819a = (long)TOEJAM_CAMERA_NEWX << 8;
        ram_ff819e = (long)TOEJAM_CAMERA_NEWY << 8;
    } else {
        ram_ff81a2 = (long)EARL_CAMERA_NEWX << 8;
        ram_ff81a6 = (long)EARL_CAMERA_NEWY << 8;
    }
}
