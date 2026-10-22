#include "entities.h"
#include "ram.h"
#include "assets.h"
#include "player.h"
#include "sprite_calls.h"
#include "ui_calls.h"
#include "romcalls.h"
/* Align complementary facing sprites into a shared animation. When Earl's
 * animation completes, redistribute the players' health using the average
 * and the two values returned by func_00ae90. */
void func_012116()
{
    struct object_state *toejam, *earl;
    short average_health, separation_y, separation_x;

    toejam = &gPlayerInfo[0];
    earl = &gEarlPositionX;
    if (toejam->level != earl->level)
        return;
    if ((toejam->sprite_state & 0xfe) == 90) {
        if (!toejam->frame_duration)
            SetSprite(toejam, 0);
        else if (toejam->frame == 2) {
            PlaySoundEffect((long)Sfx_JamClap, 95, 1);
            ++toejam->frame;
        }
    }
    if ((earl->sprite_state & 0xfe) == 90) {
        if (!earl->frame_duration) {
            SetSprite(earl, 0);
            average_health = (TOEJAMS_HEALTH[0] + TOEJAMS_HEALTH[1] + 1) >> 1;
            /* ORIGINAL BUG: These branches do not cap the compared player's
             * health to func_00ae90's result ([INFERENCE] its health limit). */
            if (average_health > func_00ae90(0)) {
                TOEJAMS_HEALTH[0] = average_health;
                TOEJAMS_HEALTH[1] = (average_health << 1) - func_00ae90(0);
            } else if (average_health > func_00ae90(1)) {
                TOEJAMS_HEALTH[1] = average_health;
                TOEJAMS_HEALTH[0] = (average_health << 1) - func_00ae90(1);
            } else {
                TOEJAMS_HEALTH[0] = average_health;
                TOEJAMS_HEALTH[1] = average_health;
            }
        } else if (earl->frame == 2)
            ++earl->frame;
        return;
    }
    separation_y = earl->y - toejam->y;
    if (separation_y < 0 || separation_y > 6)
        return;
    separation_x = earl->x - toejam->x;
    if (toejam->sprite_state == 11 && earl->sprite_state == 10) {
        if (separation_x > 0 && separation_x < 25) {
            SetSprite(toejam, 91);
            toejam->x = earl->x - 23;
            toejam->y = earl->y - 3;
            SetSprite(earl, 90);
            toejam->height = 0;
            earl->height = 0;
        }
    } else if (earl->sprite_state == 11 && toejam->sprite_state == 10 &&
               separation_x < 0 && separation_x > -25) {
        SetSprite(toejam, 90);
        toejam->x = earl->x + 23;
        toejam->y = earl->y - 3;
        SetSprite(earl, 91);
        toejam->height = 0;
        earl->height = 0;
    }
}
