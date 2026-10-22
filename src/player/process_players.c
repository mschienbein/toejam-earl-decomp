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

extern unsigned char LAND_UNDER_TOEJAM, LAND_UNDER_EARL;
extern char gPlayerLives[], P1_MENU_OPEN[], ELEVATOR_FLAG[], ram_ffde60, ram_ffde61, PLAYER_INDICATOR;
extern void func_021d32(), func_00f96c(), func_01044e(), func_01062c(), func_0107b2(), func_011ac0(), func_01189c(), func_010086(), func_0101b4(), func_010294(), func_010ffc(), func_01c188(), ProcessSleep(), func_01b8ba(), HealthAndLivesChecks(), BummerLifePromo(), func_01ccd2(), func_01d084(), CheckPlayerFalling(), func_012116();

/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = LAND_UNDER_TOEJAM & 0xf0;
    player = gPlayerInfo;
    for (player_number = 0; player_number <= 1;
         terrain = LAND_UNDER_EARL & 0xf0, ++player, ++player_number) {
        if (gPlayerLives[player_number] < 0)
            func_021d32(player_number);
        else {
            if (P1_MENU_OPEN[player_number])
                continue;
            func_00f96c(player);
            func_01044e(player, terrain);
            func_01062c(player, terrain);
            func_0107b2(player, terrain);
            func_011ac0(player, terrain);
            func_01189c(player, terrain);
            func_010086(player, terrain);
            func_0101b4(player, terrain);
            func_010294(player, terrain);
            func_010ffc(player, terrain);
            func_01c188(player, terrain);
            ProcessSleep(player);
            func_01b8ba(player, terrain);
            HealthAndLivesChecks(player, terrain);
            if (ELEVATOR_FLAG[player_number] || (ram_ffde60 & 3) == player_number + 1 ||
                (ram_ffde61 & 3) == player_number + 1)
                player->flags_4d |= 32;
            else if (player->sprite_state != 70 && player->sprite_state != 120)
                player->flags_4d &= ~32;
        }
        BummerLifePromo(player, terrain);
        func_01ccd2(player);
        func_01d084(player, terrain);
    }
    CheckPlayerFalling();
    if (PLAYER_INDICATOR == 2)
        func_012116();
}
