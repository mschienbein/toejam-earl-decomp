#include "player_animation_groups.h"
#include "entities.h"
#include "sprites.h"
#include "graphics_calls.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
/* Compute camera velocity from animation movement and viewport edge thresholds. */
void GetPlayerCameraVelocity(player, delta_x, delta_y, screen_x, screen_y)
short player, *delta_x, *delta_y, screen_x, screen_y;
{
    short camera_x, camera_y;
    short velocity_x = 0, velocity_y = 0;
    short frame_count;
    unsigned char sequence_byte;
    struct render_frame *frame;
    struct object_state *entity;
    short left, right, top, bottom;

    if (gPlayerLives[player] < 0 || gPlayerInventoryOpen[player]) {
        *delta_x = 0;
        *delta_y = 0;
        return;
    }
    if (player == 0) {
        entity = gPlayerInfo;
        camera_x = gCamera1PendingX;
        camera_y = gCamera1PendingY;
    } else {
        entity = &gPlayer2Entity;
        camera_x = gCamera2PendingX;
        camera_y = gCamera2PendingY;
    }
    if (entity->frame_phase && (entity->animation_motion_axes & ANIMATION_MOTION_XY)) {
        for (frame_count = 0; ; frame_count++) {
            sequence_byte = entity->sequence[frame_count];
            if ((sequence_byte & 0xf0) == 0xf0)
                break;
            frame = entity->frames[sequence_byte & 0x7f];
            velocity_x += frame->motion_x;
            velocity_y += frame->motion_y;
        }
        velocity_x *= entity->velocity_x;
        velocity_y *= entity->velocity_y;
        velocity_x /= entity->frame_phase * frame_count;
        velocity_y /= entity->frame_phase * frame_count;
    }
    if ((entity->draw_flags & (ENTITY_SKIP_PHYSICS | ENTITY_BLOCKED_X)) || ((long)entity->animation_group_mask & PLAYER_GROUP_JUMP_TAKEOFF))
        velocity_x = 0;
    else if (!(entity->animation_motion_axes & ANIMATION_MOTION_X) && entity->x > 168 && entity->x < 2512)
        velocity_x = entity->velocity_x;
    if ((entity->draw_flags & (ENTITY_SKIP_PHYSICS | ENTITY_BLOCKED_Y_OR_HEIGHT)) || ((long)entity->animation_group_mask & PLAYER_GROUP_JUMP_TAKEOFF) ||
            entity->sprite_state == 5 || entity->sprite_state == 6)
        velocity_y = 0;
    else if (!(entity->animation_motion_axes & ANIMATION_MOTION_Y)) {
        velocity_y = entity->velocity_y;
        if (!gPlayerFallStates[player])
            velocity_y -= entity->velocity_height / 2;
    }
    if (gPlayerLives[player ^ 1] < 0) {
        left = right = 160;
        top = bottom = 112;
    } else if (!gSplitScreenEnabled) {
        left = 50;
        right = 270;
        top = 40;
        bottom = 158;
    } else {
        left = right = 160;
        top = bottom = 56;
    }
    if (!velocity_x) {
        if (screen_x < left && camera_x > 8)
            velocity_x = -256;
        if (screen_x > right && camera_x < 2360)
            velocity_x = 256;
    }
    if (!velocity_y) {
        if (screen_y < top && camera_y > 8)
            velocity_y = -256;
        if (screen_y > bottom && camera_y < 1560)
            velocity_y = 256;
    }
    *delta_x = velocity_x;
    *delta_y = velocity_y;
}

/* Advance 24.8 camera accumulators; jointly follow players in an unsplit viewport. */
void UpdateCameraFollowing()
{
    short blocked = 0;
    short primary_x, primary_y, secondary_x, secondary_y;
    short velocity_x[2], velocity_y[2];
    short combined_velocity, target;
    short *camera_x, *camera_y;

    camera_x = &gCamera1PendingX;
    camera_y = &gCamera1PendingY;
    primary_x = gPlayerInfo[PLAYER_TOEJAM].x - *camera_x;
    primary_y = gPlayerInfo[PLAYER_TOEJAM].y - *camera_y - gPlayerInfo[PLAYER_TOEJAM].height_pixels / 2;
    secondary_x = gPlayerInfo[PLAYER_EARL].x - gCamera2PendingX;
    secondary_y = gPlayerInfo[PLAYER_EARL].y - gCamera2PendingY - gPlayerInfo[PLAYER_EARL].height_pixels / 2;
    if (gPlayerInfo[PLAYER_TOEJAM].height > 0 && ((long)gPlayerInfo[PLAYER_TOEJAM].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR))) {
        if (gSplitScreenEnabled)
            primary_y -= gPlayerInfo[PLAYER_TOEJAM].height / 2;
        else if (gPlayerInfo[PLAYER_TOEJAM].y < gPlayerInfo[PLAYER_EARL].y)
            primary_y -= gPlayerInfo[PLAYER_TOEJAM].height;
    }
    if (gPlayerInfo[PLAYER_EARL].height > 0 && ((long)gPlayerInfo[PLAYER_EARL].animation_group_mask & (PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR))) {
        if (gSplitScreenEnabled)
            secondary_y -= gPlayerInfo[PLAYER_EARL].height / 2;
        else if (gPlayerInfo[PLAYER_EARL].y < gPlayerInfo[PLAYER_TOEJAM].y)
            secondary_y -= gPlayerInfo[PLAYER_EARL].height;
    }
    GetPlayerCameraVelocity(0, &velocity_x[0], &velocity_y[0], primary_x, primary_y);
    GetPlayerCameraVelocity(1, &velocity_x[1], &velocity_y[1], secondary_x, secondary_y);
    if (!gSplitScreenEnabled && !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen &&
            gPlayerLives[PLAYER_TOEJAM] >= 0 && gPlayerLives[PLAYER_EARL] >= 0) {
        if (velocity_x[0] || velocity_x[1]) {
            combined_velocity = 0;
            if (velocity_x[0] < 0 && velocity_x[1] < 0) {
                if (velocity_x[0] < velocity_x[1] && secondary_x < 270)
                    combined_velocity = velocity_x[0];
                else if (primary_x < 270)
                    combined_velocity = velocity_x[1];
            } else if (velocity_x[0] > 0 && velocity_x[1] > 0) {
                if (velocity_x[0] > velocity_x[1] && secondary_x > 50)
                    combined_velocity = velocity_x[0];
                else if (primary_x > 50)
                    combined_velocity = velocity_x[1];
            } else {
                if (velocity_x[0] < 0 && secondary_x < 270)
                    combined_velocity = velocity_x[0];
                else if (velocity_x[1] < 0 && primary_x < 270)
                    combined_velocity = velocity_x[1];
                if (velocity_x[0] > 0 && secondary_x > 50)
                    combined_velocity += velocity_x[0];
                else if (velocity_x[1] > 0 && primary_x > 50)
                    combined_velocity += velocity_x[1];
            }
            gCamera1FollowXFixed += combined_velocity;
            target = gCamera1FollowXFixed >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                gCamera1FollowXFixed = *camera_x << 8;
                blocked = 0;
            }
        }
        if (velocity_y[0] || velocity_y[1]) {
            combined_velocity = 0;
            if (velocity_y[0] < 0 && velocity_y[1] < 0) {
                if (velocity_y[0] < velocity_y[1] && secondary_y < 158)
                    combined_velocity = velocity_y[0];
                else if (primary_y < 158)
                    combined_velocity = velocity_y[1];
            } else if (velocity_y[0] > 0 && velocity_y[1] > 0) {
                if (velocity_y[0] > velocity_y[1] && secondary_y > 40)
                    combined_velocity = velocity_y[0];
                else if (primary_y > 40)
                    combined_velocity = velocity_y[1];
            } else {
                if (velocity_y[0] < 0 && secondary_y < 158)
                    combined_velocity = velocity_y[0];
                else if (velocity_y[1] < 0 && primary_y < 158)
                    combined_velocity = velocity_y[1];
                if (velocity_y[0] > 0 && secondary_y > 40)
                    combined_velocity += velocity_y[0];
                else if (velocity_y[1] > 0 && primary_y > 40)
                    combined_velocity += velocity_y[1];
            }
            gCamera1FollowYFixed += combined_velocity;
            target = gCamera1FollowYFixed >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked)
                gCamera1FollowYFixed = *camera_y << 8;
        }
        gCamera2PendingX = *camera_x;
        gCamera2PendingY = *camera_y;
    } else if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        if (velocity_x[0]) {
            gCamera1FollowXFixed += velocity_x[0];
            target = gCamera1FollowXFixed >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                gCamera1FollowXFixed = *camera_x << 8;
                blocked = 0;
            }
        }
        if (velocity_y[0]) {
            gCamera1FollowYFixed += velocity_y[0];
            target = gCamera1FollowYFixed >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked) {
                gCamera1FollowYFixed = *camera_y << 8;
                blocked = 0;
            }
        }
        if (velocity_x[1]) {
            gCamera2FollowXFixed += velocity_x[1];
            target = gCamera2FollowXFixed >> 8;
            while (target > gCamera2PendingX)
                if (blocked = MoveCameraRight(1))
                    break;
            while (target < gCamera2PendingX)
                if (blocked = MoveCameraLeft(1))
                    break;
            if (blocked) {
                gCamera2FollowXFixed = gCamera2PendingX << 8;
                blocked = 0;
            }
        }
        if (velocity_y[1]) {
            gCamera2FollowYFixed += velocity_y[1];
            target = gCamera2FollowYFixed >> 8;
            while (target > gCamera2PendingY)
                if (blocked = MoveCameraDown(1))
                    break;
            while (target < gCamera2PendingY)
                if (blocked = MoveCameraUp(1))
                    break;
            if (blocked)
                gCamera2FollowYFixed = gCamera2PendingY << 8;
        }
    } else {
        if (velocity_x[1]) {
            gCamera1FollowXFixed += velocity_x[1];
            target = gCamera1FollowXFixed >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                gCamera1FollowXFixed = *camera_x << 8;
                blocked = 0;
            }
        }
        if (velocity_y[1]) {
            gCamera1FollowYFixed += velocity_y[1];
            target = gCamera1FollowYFixed >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked)
                gCamera1FollowYFixed = *camera_y << 8;
        }
    }
}
