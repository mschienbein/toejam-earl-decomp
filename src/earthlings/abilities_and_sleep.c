struct earthling_state;
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15;
    void (*process)();
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    struct earthling_state *earthling;
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer, x, y, reserved_0a, reserved_0c;
    char reserved_0e, second_object, reserved_10[2];
};

struct timed_animation {
    char *sequence;
    char **frames;
    char period, spare;
};
struct earthling_definition {
    struct timed_animation *states;
    void (*update)(), (*ability)(), (*touch)();
    char mode_15;
    unsigned char width, height_pixels;
    char draw_offset, draw_attribute, reserved_15[3];
    unsigned char flags_18, radius, distance_x, distance_y;
    char reserved_1c[8], health, reserved_25;
};

struct animation_descriptor {
    char *sequence;
    char **frames;
};

extern struct object_state gPlayerInfo[40];
extern struct earthling_state EARTHLINGS_IN_LEVEL[][20];
extern struct earthling_definition EARTHLING_DATA[24];
extern struct animation_descriptor SleepAnimationDescriptors[23];
extern char EARTHLING_LOAD_FLAG[];
extern char EARTHLING_LEVEL[];
extern char P1_MENU_OPEN[];
extern char ELEVATOR_FLAG[];
extern char ram_ffde6c[];
extern short ram_ffe1ae;
extern unsigned short PREGEN_RANDOMS[59];
extern unsigned short ram_ff91e0;
extern int abs();
extern int func_020e70();
extern int func_0193fa();
extern int func_00dd1e();
extern void func_00d970();
extern void func_01bb62();
extern void SetEarthlingState();

/* Invoke eligible loaded earthlings' per-type abilities on alternating slots. */
void func_018834()
{
    short bank, index;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    for (bank = 0; bank < 2; ++bank) {
        if (EARTHLING_LOAD_FLAG[bank]) {
            for (index = ram_ffe1ae; index < 20; index += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[bank][index];
                if (earthling->reserved != -1 && earthling->object &&
                    earthling->state != 99 && !func_020e70(earthling)) {
                    data = &EARTHLING_DATA[earthling->reserved];
                    if (data->ability && !func_0193fa(earthling))
                        (*data->ability)(earthling);
                }
            }
        }
    }
}

/* Visit alternating occupied earthling slots and test both eligible players
 * against the type's distance limits before calling its touch handler. */
void func_0188ee()
{
    short buffer, slot, player_number, trial;
    struct earthling_state *earthling;
    struct earthling_definition *data;
    struct object_state *object, *player;

    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = ram_ffe1ae; slot < 20; slot += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->reserved == -1 || !earthling->object ||
                    earthling->state == 99)
                    continue;
                data = &EARTHLING_DATA[earthling->reserved];
                object = &gPlayerInfo[earthling->object];
                player_number = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                                (ram_ff91e0 = 0) : ++ram_ff91e0] & 1;
                for (trial = 0; trial < 2; ++trial) {
                    player = &gPlayerInfo[player_number];
                    if (!P1_MENU_OPEN[player_number] &&
                        !ELEVATOR_FLAG[player_number] &&
                        !(player->flags_4d & 16) &&
                        func_00dd1e(object, player, data->distance_x,
                                    data->distance_y)) {
                        if ((data->flags_18 & 1) && earthling->state == 2) {
                            SetEarthlingState(earthling, 0);
                            earthling->timer = 0;
                        }
                        (*data->touch)(player_number, earthling);
                    }
                    player_number ^= 1;
                }
                if (earthling->reserved == 7)
                    func_01bb62(earthling, data);
            }
        }
    }
}

/* Nearby active objects in ram_ffde6c put eligible earthlings into state 99,
 * install their sleep animation, and release any secondary object. */
void func_018a6c()
{
    short source, buffer, slot;
    struct earthling_state *earthling;
    struct object_state *source_object, *object;

    for (source = 0; source <= 1; ++source) {
        if (ram_ffde6c[source]) {
            source_object = &gPlayerInfo[ram_ffde6c[source]];
            if (EARTHLING_LOAD_FLAG[0] &&
                EARTHLING_LEVEL[0] == source_object->level)
                buffer = 0;
            else
                buffer = 1;
            for (slot = ram_ffe1ae; slot < 20; slot += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->reserved == -1 || earthling->state == 99 ||
                    !earthling->object || earthling->reserved == 0 ||
                    earthling->reserved == 8 || earthling->reserved == 14 ||
                    earthling->reserved == 20 || earthling->reserved == 23 ||
                    earthling->reserved == 24 || func_020e70(earthling))
                    continue;
                if (earthling->reserved == 7) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->reserved == 18) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->reserved == 11) {
                    if (earthling->state < 2)
                        continue;
                } else if (earthling->reserved == 17 || earthling->reserved == 10) {
                    if ((earthling->state & 0xfc) != 4)
                        continue;
                }
                if (abs(earthling->x - source_object->x) >= 320 ||
                    abs(earthling->y - source_object->y) >= 224)
                    continue;
                object = &gPlayerInfo[earthling->object];
                object->velocity_x = 0;
                object->velocity_y = 0;
                object->sequence = SleepAnimationDescriptors[earthling->reserved].sequence;
                object->frames = SleepAnimationDescriptors[earthling->reserved].frames;
                object->frame_phase = 6;
                object->frame_duration = 1;
                object->frame = -1;
                object->sprite = 99;
                earthling->state = 99;
                if (earthling->second_object) {
                    func_00d970(earthling->second_object);
                    earthling->second_object = 0;
                }
            }
        }
    }
}

/* Wake each occupied state-99 record in the selected loaded level. */
void func_018c74(level)
short level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->reserved != -1 && earthling->state == 99)
            SetEarthlingState(earthling, 0);
    }
}

/* Reset state-2 earthlings whose type flag and object draw mask permit it. */
int func_018ce2(draw_mask)
char draw_mask;
{
    short buffer, slot;
    int changed;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    changed = 0;
    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = 0; slot < 20; ++slot) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->reserved != -1 && earthling->object &&
                    earthling->state == 2) {
                    data = &EARTHLING_DATA[earthling->reserved];
                    if ((data->flags_18 & 1) &&
                        (gPlayerInfo[earthling->object].draw_flags & draw_mask)) {
                        SetEarthlingState(earthling, 0);
                        earthling->timer = 0;
                        changed = 1;
                    }
                }
            }
        }
    }
    return changed;
}
