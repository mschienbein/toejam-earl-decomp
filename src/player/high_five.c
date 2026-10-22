struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4], frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
extern char Sfx_JamClap[], TOEJAMS_HEALTH[];
extern struct object_state gPlayerInfo[40], gEarlPositionX;
extern int func_00ae90();
extern void SetSprite(), PlaySoundEffect();

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
    if ((toejam->sprite & 0xfe) == 90) {
        if (!toejam->frame_duration)
            SetSprite(toejam, 0);
        else if (toejam->frame == 2) {
            PlaySoundEffect((long)Sfx_JamClap, 95, 1);
            ++toejam->frame;
        }
    }
    if ((earl->sprite & 0xfe) == 90) {
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
    if (toejam->sprite == 11 && earl->sprite == 10) {
        if (separation_x > 0 && separation_x < 25) {
            SetSprite(toejam, 91);
            toejam->x = earl->x - 23;
            toejam->y = earl->y - 3;
            SetSprite(earl, 90);
            toejam->height = 0;
            earl->height = 0;
        }
    } else if (earl->sprite == 11 && toejam->sprite == 10 &&
               separation_x < 0 && separation_x > -25) {
        SetSprite(toejam, 90);
        toejam->x = earl->x + 23;
        toejam->y = earl->y - 3;
        SetSprite(earl, 91);
        toejam->height = 0;
        earl->height = 0;
    }
}
