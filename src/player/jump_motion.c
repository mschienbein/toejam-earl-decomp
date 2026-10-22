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
extern char ram_ffda28[], ram_ffda26[], TOEJAM_JUMP_COUNTER[], gPlayer1Powerup[];
extern char ram_ffda24[], TOEJAM_FALL_STATE[], ELEVATOR_FLAG[], ram_ffda36[];
extern char ram_ffe238[], ram_ffe1dc[], ram_ffda38[], Sfx_Splash[];
extern unsigned char P1_PROCESSED_INPUT[];
extern int abs(), GetActivePresentSprite();
extern int func_01cc62();
extern void PlaySound(), PlaySoundEffect(), SetSprite(), SetToejamFalling();
extern short GenerateSpeech();
extern void func_011dd8();

/* Advance the alternating jump phase, handle landing and water/space,
 * and delegate the input-dependent horizontal velocity update. */
void func_011ac0(state, terrain)
struct object_state *state;
unsigned short terrain;
{
    short player;
    char direction, phase, jump;

    player = state->value_5e;
    if (gPlayer1Powerup[player])
        return;
    if (!ram_ffda24[player] && (state->flags_52 & 12)) {
        state->height = 0;
        state->draw_flags &= 0xbf;
        return;
    }
    if (TOEJAM_FALL_STATE[player] ||
        !(state->sprite == 0 || state->sprite == 4 ||
          (state->flags_52 & 0x20400000) || state->sprite == 76 ||
          (state->flags_52 & 0x8000)) || ELEVATOR_FLAG[player] ||
        (ram_ffda24[player] && state->height < -4))
        return;
    direction = state->sprite & 3;
    phase = -1;
    jump = P1_PROCESSED_INPUT[player] & 0x40;
    if (!jump) {
        ram_ffda36[player] = 1;
        ram_ffe238[player] = 0;
    }
    if (ram_ffe238[player])
        return;
    if (state->sprite == 0 || state->sprite == 4 ||
        (state->flags_52 & 0x20400000) || state->sprite == 76) {
        if (!jump || !ram_ffda36[player])
            return;
        phase = 1;
    } else if (!ram_ffda26[player] && !jump) {
        if (ram_ffda28[player] > 1)
            phase = 1;
    } else if (ram_ffda26[player] == 1 && jump) {
        phase = 0;
        ram_ffda28[player] = 0;
        PlaySound(23, 20);
    } else if (state->height < 4) {
        state->velocity_x = 0;
        state->acceleration_x = 0;
        state->velocity_y = 0;
        state->acceleration_y = 0;
        phase = -1;
        if (state->velocity_height < 0) {
            state->height = 0;
            state->velocity_height = 0;
            state->acceleration_height = 0;
            state->draw_flags &= 0xbf;
            if (terrain >= 0x80) {
                if (terrain == 0x90)
                    TOEJAM_JUMP_COUNTER[player] = 0;
                else {
                    phase = direction + 64;
                    PlaySoundEffect((long)Sfx_Splash, 50, 0);
                }
            } else {
                phase = GetActivePresentSprite(state);
                state->mode_15 = 3;
                ram_ffda36[player] = 0;
            }
            if (phase != -1) {
                SetSprite(state, phase);
                phase = -1;
            }
        } else if (terrain == 0x90) {
            if (TOEJAM_JUMP_COUNTER[player] > 10)
                SetToejamFalling(state, player);
            else if (ram_ffda28[player] > 1) {
                phase = ram_ffda26[player] ^ 1;
                ram_ffda28[player] = 0;
                ++TOEJAM_JUMP_COUNTER[player];
            }
        }
        if (phase == -1)
            return;
    }
    if (ram_ffda28[player] < 64)
        ++ram_ffda28[player];
    if (!(state->flags_52 & 0x8000)) {
        SetSprite(state, 61);
        state->draw_flags |= 0x40;
        state->mode_15 = 0;
    } else if (terrain == 0x90)
        state->draw_flags &= 0xbf;
    else
        state->draw_flags |= 0x40;
    func_011dd8(state, player, phase, jump);
}

/* Apply directional boosts at phase changes, decelerate against released
 * directions, clamp both horizontal axes, and speak once after rising. */
void func_011dd8(state, player, phase, jump)
struct object_state *state;
short player;
char phase, jump;
{
    unsigned char input;
    short velocity_x, velocity_y;
    char left, right, up, down;
    short maximum_speed;

    if (ram_ffe1dc[player])
        input = func_01cc62(player);
    else
        input = P1_PROCESSED_INPUT[player];
    velocity_x = state->velocity_x;
    velocity_y = state->velocity_y;
    left = input & 4;
    right = input & 8;
    up = input & 1;
    down = input & 2;
    if (phase != -1) {
        ram_ffda26[player] = phase;
        state->frame = phase - 1;
        state->frame_duration = 1;
        if (jump) {
            if (state->height < 50) {
                state->velocity_height = 256;
                state->acceleration_height = -10;
            } else
                state->velocity_height = 0;
            if ((velocity_x > 0 && right) || (velocity_x < 0 && left))
                velocity_x <<= 1;
            else if ((velocity_x < 0 && right) || (velocity_x > 0 && left))
                velocity_x = 0;
            if ((velocity_y > 0 && down) || (velocity_y < 0 && up))
                velocity_y <<= 1;
            else if ((velocity_y < 0 && down) || (velocity_y > 0 && up))
                velocity_y = 0;
        }
    }
    if ((velocity_x > 0 && !right && (velocity_x -= 10) < 0) ||
        (velocity_x < 0 && !left && (velocity_x += 10) > 0))
        velocity_x = 0;
    if ((velocity_y > 0 && !down && (velocity_y -= 10) < 0) ||
        (velocity_y < 0 && !up && (velocity_y += 10) > 0))
        velocity_y = 0;
    if (velocity_y)
        maximum_speed = 384;
    else
        maximum_speed = 512;
    if (abs(velocity_x) > maximum_speed) {
        if (velocity_x > 0)
            velocity_x = maximum_speed;
        else
            velocity_x = -maximum_speed;
    }
    if (velocity_x)
        maximum_speed -= 128;
    else
        maximum_speed -= 64;
    if (abs(velocity_y) > maximum_speed) {
        if (velocity_y > 0)
            velocity_y = maximum_speed;
        else
            velocity_y = -maximum_speed;
    }
    state->velocity_x = velocity_x;
    state->velocity_y = velocity_y;
    if (!ram_ffda38[player] && state->height > 10) {
        GenerateSpeech(player, "Awesome!!", 1);
        ram_ffda38[player] = 1;
    }
}
