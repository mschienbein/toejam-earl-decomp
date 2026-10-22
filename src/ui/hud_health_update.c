#define VDP_CONTROL (*(unsigned short *)0xc00004L)
#define VDP_COMMAND (*(long *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)
#define VRAM_WRITE(address) (0x40000000L + (((address) & 0x3fff) << 16) + (((address) & 0xc000) >> 14))

struct hud_sprite {
    short y;
    unsigned char size, link;
    unsigned short tile;
    short x;
};
struct hud_player {
    char reserved00[75];
    char sprite_state;
    char reserved4c[52];
};
extern struct hud_sprite ram_ffb65a[80];
extern struct hud_player gPlayerInfo[40];
extern char gPlayerLives[], ram_ffa256[], gPlayerRank[];
extern char TOEJAMS_HEALTH[], TOEJAMS_MAX_HEALTH[], TOEJAM_RESTORE_HEALTH[];
extern char P1_MENU_OPEN[], SPLIT_SCREEN, gLevel912C;
extern void func_00b3e4(), func_00b590();

/* Position the two health-bar sprites for the active HUD arrangement. */
void func_00b0cc(player_number)
short player_number;
{
    short x, sprite_number, y;

    if (player_number == 0) {
        sprite_number = 15;
        y = 229;
    } else {
        sprite_number = 17;
        y = 332;
    }
    if (P1_MENU_OPEN[player_number])
        x = 230;
    else if (SPLIT_SCREEN || P1_MENU_OPEN[player_number ^ 1]) {
        if (player_number == 0)
            y = 142;
        else
            y = 252;
        x = 144;
    } else {
        if (player_number == 0)
            x = 160;
        else
            x = 330;
        y = 333;
    }
    if (gPlayerLives[player_number] < 0)
        y = 1;
    ram_ffb65a[sprite_number].x = x;
    ram_ffb65a[sprite_number].y = y;
    ram_ffb65a[sprite_number + 1].x = x + 32;
    ram_ffb65a[sprite_number + 1].y = y;
}

/* Move the displayed health one step toward the clamped health value and
 * rewrite the affected three rows of the health bar. */
void func_00b18c(player_number)
short player_number;
{
    short maximum_health, tile_offset, displayed_health, vram_base, vram_address, pixel_number;
    unsigned short pixel_pattern;

    if (ram_ffa256[player_number] != gPlayerLives[player_number]) {
        func_00b3e4(player_number);
        func_00b590(player_number);
    }
    if (gPlayerLives[player_number] < 0 || (&gPlayerInfo[player_number])->sprite_state == 5 ||
        (&gPlayerInfo[player_number])->sprite_state == 6 || gLevel912C == 26)
        return;
    if (player_number == 0)
        maximum_health = 23;
    else
        maximum_health = 31;
    maximum_health += gPlayerRank[player_number] * 4;
    if (TOEJAMS_HEALTH[player_number] < 0)
        TOEJAMS_HEALTH[player_number] = 0;
    if (TOEJAMS_HEALTH[player_number] > maximum_health)
        TOEJAMS_HEALTH[player_number] = maximum_health;
    if (TOEJAM_RESTORE_HEALTH[player_number] > maximum_health)
        TOEJAM_RESTORE_HEALTH[player_number] = maximum_health;
    if (TOEJAMS_HEALTH[player_number] > TOEJAMS_MAX_HEALTH[player_number])
        TOEJAMS_MAX_HEALTH[player_number]++;
    else if (TOEJAMS_HEALTH[player_number] < TOEJAMS_MAX_HEALTH[player_number])
        TOEJAMS_MAX_HEALTH[player_number]--;
    else
        return;
    displayed_health = TOEJAMS_MAX_HEALTH[player_number];
    if (player_number == 0)
        vram_base = 0xf680;
    else
        vram_base = 0xfa80;
    VDP_CONTROL = 0x8f04;
    tile_offset = (short)(displayed_health / 8) * 32 + ((displayed_health >> 1) & 2);
    pixel_pattern = 0xdddd;
    if (displayed_health <= maximum_health - 4) {
        if (displayed_health & 4)
            vram_address = vram_base + tile_offset + 30;
        else
            vram_address = vram_base + tile_offset + 2;
        VDP_COMMAND = VRAM_WRITE(vram_address);
        VDP_DATA = pixel_pattern;
        VDP_DATA = pixel_pattern;
        VDP_DATA = pixel_pattern;
    }
    if (displayed_health)
        for (pixel_number = displayed_health & 3; pixel_number >= 0; pixel_number--) {
            pixel_pattern >>= 4;
            pixel_pattern |= 0xb000;
        }
    VDP_COMMAND = VRAM_WRITE(vram_base + tile_offset);
    VDP_DATA = pixel_pattern;
    VDP_DATA = pixel_pattern;
    VDP_DATA = pixel_pattern;
    VDP_CONTROL = 0x8f02;
}
