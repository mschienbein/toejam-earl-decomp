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
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer, x, y, terrain;
    char reserved_0c[3], second_object, reserved_10[2];
};
struct earthling_definition {
    char reserved_00[21], target_chance;
    unsigned char target_distance;
    char turn_chance;
    unsigned char movement_flags;
    char reserved_19;
    unsigned char contact_distance_x, contact_distance_y;
    char damage;
    unsigned char target_time_min, target_time_max;
    unsigned char turn_time_min, turn_time_max;
    unsigned char move_time_min, move_time_max;
    char state_delay, reserved_24[2];
};
extern struct object_state gPlayerInfo[40];
extern struct earthling_definition EARTHLING_DATA[24];
extern char ram_ffda24[], TOEJAMS_HEALTH[];
extern char HULA_DANCE_TIMER[], HULA_DANCE[], ram_ffe1b4;
extern unsigned short PREGEN_RANDOMS[59], ram_ff91e0;
extern char Sfx_Squish[], Sfx_Aloha[];
extern int func_01b0aa(), abs(), GetActivePresentSprite();
extern short GenerateSpeech();
extern void SetSprite(), SetEarthlingState(), PlaySoundEffect();

/* Flatten an eligible player, apply direct damage, and play the squish sound. */
void Player_Squish(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *player;

    player = &gPlayerInfo[player_number];
    if ((earthling->reserved != 19 || (earthling->state & 0xfc) == 4) &&
        func_01b0aa(player, player_number)) {
        player->mode_15 = 3;
        SetSprite(player, 2);
        player->draw_flags |= 128;
        if (ram_ffda24[player_number])
            player->height = -3;
        else
            player->height = 0;
        if (earthling->state)
            earthling->timer = 30;
        TOEJAMS_HEALTH[player_number] -= EARTHLING_DATA[earthling->reserved].damage;
        PlaySoundEffect((long)Sfx_Squish, 50, 0);
    }
}

/* Nearby eligible players periodically dance. Close dancing can also trigger
 * the shared speech/sound cooldown; leaving the radius restores their pose. */
void HulaAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char sprite = -1;
    short distance_x, distance_y, distance;
    struct object_state *player, *dancer;

    player = &gPlayerInfo[player_number];
    if (!player->level)
        return;
    if (func_01b0aa(player, player_number) &&
        !(player->flags_52 & 0x20000010) &&
        (dancer = &gPlayerInfo[earthling->object],
         distance_x = abs(dancer->x - player->x),
         distance_y = abs(dancer->y - player->y),
         distance_x <= 100 && distance_y <= 75)) {
        if (--HULA_DANCE_TIMER[player_number] < 0) {
            distance_y += distance_y / 3;
            if (distance_x > distance_y)
                distance = distance_x;
            else
                distance = distance_y;
            if ((PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                (ram_ff91e0 = 0) : ++ram_ff91e0] & 3) == 3) {
                sprite = 89;
                HULA_DANCE[player_number] = 28;
                if (distance_x < 20 && distance_y < 15) {
                    if (!ram_ffe1b4) {
                        GenerateSpeech(player_number, "Hubba hubba", 1);
                        PlaySoundEffect((long)Sfx_Aloha, 50, 1);
                        ram_ffe1b4 = 128;
                    } else
                        --ram_ffe1b4;
                }
            }
            HULA_DANCE_TIMER[player_number] = (distance >> 5) + 4;
        }
    } else if (player->sprite == 89) {
        sprite = GetActivePresentSprite(player);
        HULA_DANCE_TIMER[player_number] = 0;
    }
    if (sprite != -1 && sprite != player->sprite) {
        player->mode_15 = 3;
        SetSprite(player, sprite);
    }
}

/* Begin a horizontal earthling attack when its target is over 65 pixels away. */
int func_01b5ca(earthling, target)
struct earthling_state *earthling;
struct object_state *target;
{
    short delta_x;
    struct object_state *object;

    delta_x = target->x - earthling->x;
    if (abs(delta_x) > 65) {
        SetEarthlingState(earthling, (short)(delta_x > 0 ? 11 : 10));
        object = &gPlayerInfo[earthling->object];
        object->frame = -1;
        return 1;
    }
    return 0;
}
