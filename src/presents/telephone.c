/* The 128-byte stride and accessed offsets are observed in these routines. */
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
    char reserved_30[27];
    char sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
extern struct object_state gPlayerInfo[40];

struct telephone_state {
    char reserved, level, object, reserved_3;
    short x, y;
};
struct animation_descriptor {
    char *sequence;
    char **frames;
};
extern struct telephone_state TELEPHONE_1[];
extern struct animation_descriptor AnimationDescriptors_0AF0A8[];
extern char dat_0ae4a0[];
extern char Sfx_Telephone[];
extern char ram_ffda6c;
extern char gTelephoneTimeToRing[], gTelephoneRingsLeft[], ram_ffe25e[];
extern char gLevel912C, gLevel912E, SPLIT_SCREEN, gPlayerLives[];
extern short TOEJAM_CAMERA_NEWX, TOEJAM_CAMERA_NEWY;
extern short EARL_CAMERA_NEWX, EARL_CAMERA_NEWY, TOEJAM_SPEECH_COUNTER;
extern short ram_ff935a;
extern int func_01e736(), func_015b66();
extern int abs();
extern void PlaySoundEffect(), func_00d970(), DrawGameMap();

/* Allocate the telephone's displayed object and copy its position and level. */
void func_022bf6(telephone)
struct telephone_state *telephone;
{
    struct object_state *object;
    struct animation_descriptor *animation;

    if (telephone->object = func_01e736(0)) {
        object = &gPlayerInfo[telephone->object];
        animation = AnimationDescriptors_0AF0A8;
        object->frames = animation->frames;
        object->sequence = dat_0ae4a0;
        object->frame_phase = 0;
        object->frame_duration = 1;
        object->frame = -1;
        object->x = telephone->x;
        object->y = telephone->y;
        object->level = telephone->level;
        object->width = 24;
        object->height_pixels = 16;
        object->draw_attribute = 1;
        object->draw_offset = 6;
        object->tile = 0x8000;
        object->draw_flags |= 8;
    }
}

/* Restart the ring timer and advance the visible telephone animation. */
void func_022c78(telephone_number, telephone)
short telephone_number;
struct telephone_state *telephone;
{
    struct object_state *object;

    if (!ram_ffda6c)
        PlaySoundEffect((long)Sfx_Telephone, 50, 0);
    gTelephoneTimeToRing[telephone_number] = 60;
    if (telephone->object) {
        object = &gPlayerInfo[telephone->object];
        object->sequence = AnimationDescriptors_0AF0A8[0].sequence;
        object->frame = -1;
        object->frame_duration = 1;
        object->frame_phase = 3;
    }
}

/* Visibility creates/removes the object. Nearby living players start its
 * ringing; a player reaching it opens the map. */
void CheckForTelephone(telephone_number)
short telephone_number;
{
    struct telephone_state *telephone;
    struct object_state *player;
    short player_number, screen_x, screen_y;
    short earl_screen_x, earl_screen_y;

    telephone = &TELEPHONE_1[telephone_number];
    if (!telephone->level)
        return;
    screen_x = telephone->x - TOEJAM_CAMERA_NEWX;
    screen_y = telephone->y - TOEJAM_CAMERA_NEWY;
    earl_screen_x = telephone->x - EARL_CAMERA_NEWX;
    earl_screen_y = telephone->y - EARL_CAMERA_NEWY;
    if ((telephone->level == gLevel912C && screen_x + 20 > 0 &&
         screen_x < 340 && screen_y + 20 > 0 && screen_y < 244) ||
        (telephone->level == gLevel912E && SPLIT_SCREEN &&
         earl_screen_x + 20 > 0 && earl_screen_x < 340 &&
         earl_screen_y + 20 > 0 && earl_screen_y < 132)) {
        if (!telephone->object)
            func_022bf6(telephone);
    } else if (telephone->object) {
        func_00d970(telephone->object);
        telephone->object = 0;
    }
    if (!ram_ffe25e[telephone_number]) {
        player = &gPlayerInfo[0];
        player_number = 0;
        do {
            if (gPlayerLives[player_number] >= 0 &&
                player->level == telephone->level && !TOEJAM_SPEECH_COUNTER &&
                abs(telephone->x - player->x) < 320 &&
                abs(telephone->y - player->y) < 224) {
                ram_ffe25e[telephone_number] = 1;
                gTelephoneRingsLeft[telephone_number] = 6;
                func_022c78(telephone_number, telephone);
                break;
            }
            ++player;
            ++player_number;
        } while (player_number <= 1);
        if (!ram_ffe25e[telephone_number])
            return;
    }
    if (--gTelephoneTimeToRing[telephone_number] <= 0) {
        if (--gTelephoneRingsLeft[telephone_number] > 0 &&
            (gPlayerInfo[0].level == telephone->level ||
             gPlayerInfo[1].level == telephone->level)) {
            if (!TOEJAM_SPEECH_COUNTER)
                func_022c78(telephone_number, telephone);
            else {
                ++gTelephoneRingsLeft[telephone_number];
                gTelephoneTimeToRing[telephone_number] = 1;
            }
        } else {
            if (telephone->object) {
                func_00d970(telephone->object);
                telephone->object = 0;
                func_015b66(telephone_number, telephone->x, telephone->y + 1,
                            0, telephone->level);
            }
            telephone->level = 0;
        }
    }
    if (telephone->object) {
        player = &gPlayerInfo[0];
        player_number = 0;
        do {
            if (gPlayerLives[player_number] >= 0 &&
                player->level == telephone->level &&
                abs(telephone->x - player->x) < 16 &&
                abs(telephone->y - player->y) < 8) {
                func_00d970(telephone->object);
                telephone->object = 0;
                ram_ff935a = 1;
                DrawGameMap(player_number);
                ram_ff935a = 0;
                telephone->level = 0;
                break;
            }
            ++player;
            ++player_number;
        } while (player_number <= 1);
    }
}
