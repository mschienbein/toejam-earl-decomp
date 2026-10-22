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
extern unsigned char P1_PREV_PROCESSED_INPUT[];
extern void StopPlayer(), SetSprite();

/* Stop horizontal movement, raise a submerged player, and select the
 * directional rocket-skates sprite for the current terrain and height. */
void OpenRocketSkates(state, player, terrain)
struct object_state *state;
short player;
unsigned short terrain;
{
    char sprite = 1;

    StopPlayer(state);
    if (state->height > 0)
        state->acceleration_height = -32;
    else if ((state->flags_52 & 0x800) || state->sprite == 3 || state->sprite == 7) {
        state->height = -16;
        state->velocity_height = 256;
        state->acceleration_height = 0;
    }
    if (state->flags_52 & 0x07b5fffc)
        sprite = state->sprite & 3;
    if (terrain == 0x90 || state->height > 0)
        sprite += 104;
    else if (terrain == 0x80)
        sprite += 100;
    else
        sprite += 92;
    SetSprite(state, sprite);
    state->mode_15 = 0;
    P1_PREV_PROCESSED_INPUT[player] &= 0xf0;
    if (state->height >= 0)
        state->draw_flags |= 0x40;
}
