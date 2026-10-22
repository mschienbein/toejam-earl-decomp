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
extern char ram_ffe236[];

/* Check the per-player exclusion byte and the allowed sprite/state masks. */
int func_01b0aa(player, player_number)
struct object_state *player;
short player_number;
{
    if (!ram_ffe236[player_number] &&
        (!player->sprite || player->sprite == 4 ||
         (player->flags_52 & 0x20400000) || player->sprite == 76 ||
         (player->flags_52 & 0x830409c)))
        return 1;
    return 0;
}
