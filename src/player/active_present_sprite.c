/* Fields reused from accepted src/ui/inventory.c; the record stride is 128 bytes. */
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[26], value_4a, sprite_state, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
extern struct object_state gPlayerInfo[40];

extern char gPlayer1Powerup[], ELEVATOR_FLAG[];
extern unsigned char LAND_UNDER_TOEJAM, LAND_UNDER_EARL;
extern void OpenRocketSkates();

/* Select the player's idle sprite from the active present and terrain,
 * resetting the animation and horizontal velocity for a new selection. */
int GetActivePresentSprite(player)
struct object_state *player;
{
    register unsigned short land;
    short player_number;
    char sprite;

    player_number = player->value_5e;
    land = player_number == 0 ? LAND_UNDER_TOEJAM : LAND_UNDER_EARL;
    land &= 0xf0;
    if (gPlayer1Powerup[player_number] == 2) {
        if (land == 0x80)
            sprite = 77;
        else if (!ELEVATOR_FLAG[player_number])
            sprite = 76;
        else {
            sprite = 0;
            player->frame = -1;
        }
    } else if (gPlayer1Powerup[player_number] == 5) {
        OpenRocketSkates(player, player_number, land);
        sprite = -1;
        return sprite;
    } else if (land == 0x80 && !player->height)
        sprite = 3;
    else if (gPlayer1Powerup[player_number] == 1 && !ELEVATOR_FLAG[player_number])
        sprite = 57;
    else if (!gPlayer1Powerup[player_number] && !ELEVATOR_FLAG[player_number]) {
        if ((player->flags_52 & 0x8000) && player->height > 0) {
            sprite = -1;
            return sprite;
        } else {
            player->draw_flags |= 0x40;
            sprite = 4;
        }
    } else {
        sprite = 0;
        player->frame = -1;
    }
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite != 4)
        player->draw_flags &= ~0x40;
    return sprite;
}
