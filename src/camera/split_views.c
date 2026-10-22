#include "entities.h"
#include "core_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "random.h"
#include "sprite_calls.h"

/* Animate the players toward the centers of the two half-height viewports,
 * keeping 26.6 intermediates; restore the world positions after the effect. */
void AnimatePlayersIntoSplitViews()
{
    long distance, distance_divisor;
    short finished = 0;
    long primary_x, primary_y, secondary_x, secondary_y;
    long primary_dx, primary_dy, secondary_dx, secondary_dy;
    long primary_target_x, primary_target_y, secondary_target_x, secondary_target_y;
    short saved_primary_x, saved_primary_y, saved_secondary_x, saved_secondary_y;

    saved_primary_x = gPlayerInfo[PLAYER_TOEJAM].x;
    saved_primary_y = gPlayerInfo[PLAYER_TOEJAM].y;
    saved_secondary_x = gPlayerInfo[PLAYER_EARL].x;
    saved_secondary_y = gPlayerInfo[PLAYER_EARL].y;
    primary_x = (long)gPlayerInfo[PLAYER_TOEJAM].x << 6;
    primary_y = (long)gPlayerInfo[PLAYER_TOEJAM].y << 6;
    secondary_x = (long)gPlayerInfo[PLAYER_EARL].x << 6;
    secondary_y = (long)gPlayerInfo[PLAYER_EARL].y << 6;
    primary_target_x = (long)(gCamera1PendingX + 160);
    primary_target_x *= 64;
    primary_target_y = (long)((gCamera1PendingY + 56) +
        gPlayerInfo[PLAYER_TOEJAM].height_pixels / 2 + gPlayerInfo[PLAYER_TOEJAM].height / 2);
    primary_target_y *= 64;
    secondary_target_x = (long)(gCamera1PendingX + 160);
    secondary_target_x *= 64;
    secondary_target_y = (long)((gCamera1PendingY + 168) +
        gPlayerInfo[PLAYER_EARL].height_pixels / 2 + gPlayerInfo[PLAYER_EARL].height / 2);
    secondary_target_y *= 64;
    primary_dx = primary_target_x - primary_x;
    primary_dy = primary_target_y - primary_y;
    secondary_dx = secondary_target_x - secondary_x;
    secondary_dy = secondary_target_y - secondary_y;
    distance = abs(primary_dx);
    if (distance < abs(primary_dy))
        distance = abs(primary_dy);
    if (distance < abs(secondary_dx))
        distance = abs(secondary_dx);
    if (distance < abs(secondary_dy))
        distance = abs(secondary_dy);
    distance_divisor = (distance >> 7) + 192;
    distance /= distance_divisor;
    primary_dx /= distance;
    primary_dy /= distance;
    secondary_dx /= distance;
    secondary_dy /= distance;
    while (!finished) {
        finished = 1;
        PositionEntitySprites(gEntityDrawListHead);
        WaitForNextVBlank(1);
        UploadQueuedSpriteGraphics(1, 0);
        distance = primary_target_x - primary_x;
        if (primary_dx && distance && (distance ^ primary_dx) >= 0) {
            finished = 0;
            primary_x += primary_dx;
            gPlayerInfo[PLAYER_TOEJAM].x = primary_x >> 6;
        }
        distance = primary_target_y - primary_y;
        if (primary_dy && distance && (distance ^ primary_dy) >= 0) {
            finished = 0;
            primary_y += primary_dy;
            gPlayerInfo[PLAYER_TOEJAM].y = primary_y >> 6;
        }
        distance = secondary_target_x - secondary_x;
        if (secondary_dx && distance && (distance ^ secondary_dx) >= 0) {
            finished = 0;
            secondary_x += secondary_dx;
            gPlayerInfo[PLAYER_EARL].x = secondary_x >> 6;
        }
        distance = secondary_target_y - secondary_y;
        if (secondary_dy && distance && (distance ^ secondary_dy) >= 0) {
            finished = 0;
            secondary_y += secondary_dy;
            gPlayerInfo[PLAYER_EARL].y = secondary_y >> 6;
        }
    }
    gPlayerInfo[PLAYER_TOEJAM].x = saved_primary_x;
    gPlayerInfo[PLAYER_TOEJAM].y = saved_primary_y;
    gPlayerInfo[PLAYER_EARL].x = saved_secondary_x;
    gPlayerInfo[PLAYER_EARL].y = saved_secondary_y;
}
