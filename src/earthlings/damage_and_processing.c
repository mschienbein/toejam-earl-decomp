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

extern struct object_state gPlayerInfo[40];
extern struct earthling_state EARTHLINGS_IN_LEVEL[][20];
extern struct earthling_state EARTHLINGS_IN_LEVEL_COPY[20];
extern char EARTHLING_LOAD_FLAG[];
extern char EARTHLING_LOAD_FLAG_COPY;
extern char EARTHLING_LEVEL[];
extern char P1_MENU_OPEN[];
extern char HBLANK_FLAG_1;
extern short ram_ffe1ae;
extern int func_020e70();
extern int NiceEarthlingCheck();
extern int func_01bdd0();
/* ORIGINAL BUG: these callers read an int/word from func_00d9dc, whose accepted
 * definition returns unsigned char. Preserve the missing byte conversion. */
extern int func_00d9dc();
extern void func_01bde8();
extern void func_020d3e();
extern void func_0223f6();
extern void SetEarthlingState();
extern void LoadInEarthlings();
extern void func_018834();
extern void func_0188ee();
extern void func_018a6c();
extern void func_018da4();

/* Decrement the hit-point byte; nice-earthling handling can preserve one hit.
 * Removal also creates the effect at the record's saved position. */
void DamageEarthling(earthling)
struct earthling_state *earthling;
{
    if (earthling && earthling->reserved > 0) {
        if (!func_020e70(earthling) && earthling->reserved != 20 &&
            earthling->reserved < 23 && --earthling->reserved_10[0] <= 0) {
            if (earthling->reserved == 17 || earthling->reserved == 10 ||
                earthling->reserved == 11) {
                if (NiceEarthlingCheck(earthling)) {
                    earthling->reserved_10[0] = 1;
                    return;
                }
            }
            func_018da4(earthling);
            func_01bde8(earthling->x, earthling->y, earthling->reserved_2);
        } else if (earthling->second_object) {
            SetEarthlingState(earthling, 0);
            earthling->timer = 0;
        }
    }
}

/* Remove eligible earthlings whose saved coordinates now lie on terrain 0x90. */
void func_018f1e(player_number, level)
short player_number, level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->reserved > 0 && earthling->object &&
            earthling->reserved != 6 && earthling->reserved != 22 &&
            earthling->reserved != 7 && earthling->reserved != 20 &&
            earthling->reserved != 8 && earthling->reserved != 19 &&
            func_00d9dc(player_number, earthling->x, earthling->y) == 0x90) {
            earthling->reserved_10[0] = 0;
            DamageEarthling(earthling);
        }
    }
}

/* Damage eligible allocated earthlings whose object matches the draw mask. */
void DoVikingAbility(draw_mask)
char draw_mask;
{
    short buffer, slot;
    struct earthling_state *earthling;

    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = 0; slot < 20; ++slot) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->reserved != -1 && earthling->reserved != 17 &&
                    earthling->object &&
                    (gPlayerInfo[earthling->object].draw_flags & draw_mask) &&
                    (earthling->reserved_10[0] > 1 || func_01bdd0()))
                    DamageEarthling(earthling);
            }
        }
    }
}

/* Clear the player's per-mole contact flag for every allocated mole object. */
void func_019086(player)
struct object_state *player;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == player->level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->reserved == 18 && earthling->object) {
            if (!player->value_5e)
                gPlayerInfo[earthling->object].flag_57 = 0;
            else
                gPlayerInfo[earthling->object].flag_58 = 0;
        }
    }
}

/* Advance loaded earthlings unless both the menu and HBlank flags are set. */
void ProcessEarthlings()
{
    if (P1_MENU_OPEN[0] && HBLANK_FLAG_1)
        return;
    if (EARTHLING_LOAD_FLAG[0])
        LoadInEarthlings(EARTHLINGS_IN_LEVEL[0]);
    if (EARTHLING_LOAD_FLAG_COPY)
        LoadInEarthlings(EARTHLINGS_IN_LEVEL_COPY);
    func_018834();
    func_0188ee();
    func_018a6c();
    func_020d3e();
    func_0223f6();
    ram_ffe1ae ^= 1;
}
