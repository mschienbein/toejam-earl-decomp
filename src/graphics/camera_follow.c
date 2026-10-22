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
void func_00267a(player, delta_x, delta_y, screen_x, screen_y)
short player, *delta_x, *delta_y, screen_x, screen_y;
{
    short camera_x, camera_y;
    short velocity_x = 0, velocity_y = 0;
    short frame_count;
    unsigned char sequence_byte;
    struct render_frame *frame;
    struct object_state *entity;
    short left, right, top, bottom;

    if (gPlayerLives[player] < 0 || P1_MENU_OPEN[player]) {
        *delta_x = 0;
        *delta_y = 0;
        return;
    }
    if (player == 0) {
        entity = gPlayerInfo;
        camera_x = TOEJAM_CAMERA_NEWX;
        camera_y = TOEJAM_CAMERA_NEWY;
    } else {
        entity = &gEarlPositionX;
        camera_x = EARL_CAMERA_NEWX;
        camera_y = EARL_CAMERA_NEWY;
    }
    if (entity->frame_phase && (entity->mode_15 & 3)) {
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
    if ((entity->draw_flags & 24) || ((long)entity->flags_52 & 0x100))
        velocity_x = 0;
    else if (!(entity->mode_15 & 1) && entity->x > 168 && entity->x < 2512)
        velocity_x = entity->velocity_x;
    if ((entity->draw_flags & 40) || ((long)entity->flags_52 & 0x100) ||
            entity->sprite_state == 5 || entity->sprite_state == 6)
        velocity_y = 0;
    else if (!(entity->mode_15 & 2)) {
        velocity_y = entity->velocity_y;
        if (!TOEJAM_FALL_STATE[player])
            velocity_y -= entity->velocity_height / 2;
    }
    if (gPlayerLives[player ^ 1] < 0) {
        left = right = 160;
        top = bottom = 112;
    } else if (!SPLIT_SCREEN) {
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
void func_0028f6()
{
    short blocked = 0;
    short primary_x, primary_y, secondary_x, secondary_y;
    short velocity_x[2], velocity_y[2];
    short combined_velocity, target;
    short *camera_x, *camera_y;

    camera_x = &TOEJAM_CAMERA_NEWX;
    camera_y = &TOEJAM_CAMERA_NEWY;
    primary_x = gPlayerInfo[0].x - *camera_x;
    primary_y = gPlayerInfo[0].y - *camera_y - gPlayerInfo[0].height_pixels / 2;
    secondary_x = gPlayerInfo[1].x - EARL_CAMERA_NEWX;
    secondary_y = gPlayerInfo[1].y - EARL_CAMERA_NEWY - gPlayerInfo[1].height_pixels / 2;
    if (gPlayerInfo[0].height > 0 && ((long)gPlayerInfo[0].flags_52 & 0x04008000)) {
        if (SPLIT_SCREEN)
            primary_y -= gPlayerInfo[0].height / 2;
        else if (gPlayerInfo[0].y < gPlayerInfo[1].y)
            primary_y -= gPlayerInfo[0].height;
    }
    if (gPlayerInfo[1].height > 0 && ((long)gPlayerInfo[1].flags_52 & 0x04008000)) {
        if (SPLIT_SCREEN)
            secondary_y -= gPlayerInfo[1].height / 2;
        else if (gPlayerInfo[1].y < gPlayerInfo[0].y)
            secondary_y -= gPlayerInfo[1].height;
    }
    func_00267a(0, &velocity_x[0], &velocity_y[0], primary_x, primary_y);
    func_00267a(1, &velocity_x[1], &velocity_y[1], secondary_x, secondary_y);
    if (!SPLIT_SCREEN && !P1_MENU_OPEN[0] && !HBLANK_FLAG_1 &&
            gPlayerLives[0] >= 0 && gPlayerLives[1] >= 0) {
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
            ram_ff819a += combined_velocity;
            target = ram_ff819a >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                ram_ff819a = *camera_x << 8;
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
            ram_ff819e += combined_velocity;
            target = ram_ff819e >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked)
                ram_ff819e = *camera_y << 8;
        }
        EARL_CAMERA_NEWX = *camera_x;
        EARL_CAMERA_NEWY = *camera_y;
    } else if (gPlayerLives[0] >= 0) {
        if (velocity_x[0]) {
            ram_ff819a += velocity_x[0];
            target = ram_ff819a >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                ram_ff819a = *camera_x << 8;
                blocked = 0;
            }
        }
        if (velocity_y[0]) {
            ram_ff819e += velocity_y[0];
            target = ram_ff819e >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked) {
                ram_ff819e = *camera_y << 8;
                blocked = 0;
            }
        }
        if (velocity_x[1]) {
            ram_ff81a2 += velocity_x[1];
            target = ram_ff81a2 >> 8;
            while (target > EARL_CAMERA_NEWX)
                if (blocked = MoveCameraRight(1))
                    break;
            while (target < EARL_CAMERA_NEWX)
                if (blocked = MoveCameraLeft(1))
                    break;
            if (blocked) {
                ram_ff81a2 = EARL_CAMERA_NEWX << 8;
                blocked = 0;
            }
        }
        if (velocity_y[1]) {
            ram_ff81a6 += velocity_y[1];
            target = ram_ff81a6 >> 8;
            while (target > EARL_CAMERA_NEWY)
                if (blocked = MoveCameraDown(1))
                    break;
            while (target < EARL_CAMERA_NEWY)
                if (blocked = MoveCameraUp(1))
                    break;
            if (blocked)
                ram_ff81a6 = EARL_CAMERA_NEWY << 8;
        }
    } else {
        if (velocity_x[1]) {
            ram_ff819a += velocity_x[1];
            target = ram_ff819a >> 8;
            while (target > *camera_x)
                if (blocked = MoveCameraRight(0))
                    break;
            while (target < *camera_x)
                if (blocked = MoveCameraLeft(0))
                    break;
            if (blocked) {
                ram_ff819a = *camera_x << 8;
                blocked = 0;
            }
        }
        if (velocity_y[1]) {
            ram_ff819e += velocity_y[1];
            target = ram_ff819e >> 8;
            while (target > *camera_y)
                if (blocked = MoveCameraDown(0))
                    break;
            while (target < *camera_y)
                if (blocked = MoveCameraUp(0))
                    break;
            if (blocked)
                ram_ff819e = *camera_y << 8;
        }
    }
}
