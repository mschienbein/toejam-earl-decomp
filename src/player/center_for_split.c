struct centered_entity {
    short x, y, elevation;
    char reserved06[36];
    unsigned char height;
    char reserved2b[85];
};
extern struct centered_entity gPlayerInfo[26];
extern short TOEJAM_CAMERA_NEWX, TOEJAM_CAMERA_NEWY;
extern void *ram_ffd938;
extern long abs();
extern void func_00c242(), WaitForNextVBlank(), SpriteLoadingStuff();

/* Animate the players toward the centers of the two half-height viewports,
 * keeping 26.6 intermediates; restore the world positions after the effect. */
void func_00300a()
{
    long distance, distance_divisor;
    short finished = 0;
    long primary_x, primary_y, secondary_x, secondary_y;
    long primary_dx, primary_dy, secondary_dx, secondary_dy;
    long primary_target_x, primary_target_y, secondary_target_x, secondary_target_y;
    short saved_primary_x, saved_primary_y, saved_secondary_x, saved_secondary_y;

    saved_primary_x = gPlayerInfo[0].x;
    saved_primary_y = gPlayerInfo[0].y;
    saved_secondary_x = gPlayerInfo[1].x;
    saved_secondary_y = gPlayerInfo[1].y;
    primary_x = (long)gPlayerInfo[0].x << 6;
    primary_y = (long)gPlayerInfo[0].y << 6;
    secondary_x = (long)gPlayerInfo[1].x << 6;
    secondary_y = (long)gPlayerInfo[1].y << 6;
    primary_target_x = (long)(TOEJAM_CAMERA_NEWX + 160);
    primary_target_x *= 64;
    primary_target_y = (long)((TOEJAM_CAMERA_NEWY + 56) +
        gPlayerInfo[0].height / 2 + gPlayerInfo[0].elevation / 2);
    primary_target_y *= 64;
    secondary_target_x = (long)(TOEJAM_CAMERA_NEWX + 160);
    secondary_target_x *= 64;
    secondary_target_y = (long)((TOEJAM_CAMERA_NEWY + 168) +
        gPlayerInfo[1].height / 2 + gPlayerInfo[1].elevation / 2);
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
        func_00c242(ram_ffd938);
        WaitForNextVBlank(1);
        SpriteLoadingStuff(1, 0);
        distance = primary_target_x - primary_x;
        if (primary_dx && distance && (distance ^ primary_dx) >= 0) {
            finished = 0;
            primary_x += primary_dx;
            gPlayerInfo[0].x = primary_x >> 6;
        }
        distance = primary_target_y - primary_y;
        if (primary_dy && distance && (distance ^ primary_dy) >= 0) {
            finished = 0;
            primary_y += primary_dy;
            gPlayerInfo[0].y = primary_y >> 6;
        }
        distance = secondary_target_x - secondary_x;
        if (secondary_dx && distance && (distance ^ secondary_dx) >= 0) {
            finished = 0;
            secondary_x += secondary_dx;
            gPlayerInfo[1].x = secondary_x >> 6;
        }
        distance = secondary_target_y - secondary_y;
        if (secondary_dy && distance && (distance ^ secondary_dy) >= 0) {
            finished = 0;
            secondary_y += secondary_dy;
            gPlayerInfo[1].y = secondary_y >> 6;
        }
    }
    gPlayerInfo[0].x = saved_primary_x;
    gPlayerInfo[0].y = saved_primary_y;
    gPlayerInfo[1].x = saved_secondary_x;
    gPlayerInfo[1].y = saved_secondary_y;
}
