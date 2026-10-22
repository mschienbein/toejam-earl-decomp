#define TJE_LOCAL_RANKCHARSTABLE
#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "ram.h"
#include "assets.h"
#include "player.h"
#include "present_calls.h"
#include "ui_calls.h"
#include "romcalls.h"

/* Compiler bound qualification: the compatible incomplete-array view
 * preserves the observed index arithmetic. The complete bound changes width;
 * a pointer cast changes address-register allocation. Historical declaration
 * spelling is unknown [INFERENCE]. */
extern char *RankCharsTable[];

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
/* Print each active player's rank and the absent player's vacation label. */
void func_00b64a()
{
    if (PLAYER_INDICATOR == 2) {
        VDP_COMMAND = 0x5c160003;
        func_009f8a(RankCharsTable[gPlayerRank[0]]);
        VDP_COMMAND = 0x5c400003;
        func_009f8a(RankCharsTable[gPlayerRank[1]]);
    } else if (PLAYER_INDICATOR == 0) {
        VDP_COMMAND = 0x5c160003;
        func_009f8a(RankCharsTable[gPlayerRank[0]]);
        VDP_COMMAND = 0x5c3c0003;
        func_009f8a(" on");
        VDP_COMMAND = 0x5cba0003;
        func_009f8a("vacation");
    } else {
        VDP_COMMAND = 0x5c400003;
        func_009f8a(RankCharsTable[gPlayerRank[1]]);
        VDP_COMMAND = 0x5c120003;
        func_009f8a(" on");
        VDP_COMMAND = 0x5c900003;
        func_009f8a("vacation");
    }
}

/* Draw the five-row combined HUD when neither inventory is open, otherwise
 * refresh the alternate HUD and the hardware sprite positions. */
void DrawHUD()
{
    short row, column;
    unsigned short tile;

    if (gLevel912C == 26) {
        VBLANK_FLAG_5 = 0;
        return;
    }
    if (!SPLIT_SCREEN && !P1_MENU_OPEN[0] && !P1_MENU_OPEN[1]) {
        for (row = 0; row < 5; row++) {
            VDP_COMMAND = 0x50000003 + ((row + 23) << 23);
            if (row < 4) {
                for (column = 0; column < 40; column++) {
                    tile = rom_0a836c[row][column];
                    if (tile & 0x40)
                        tile |= 0x800;
                    if (tile & 0x80)
                        tile |= 0x1000;
                    tile &= 0xff3f;
                    VDP_DATA = tile + 0x8580;
                }
            } else
                for (column = 0; column < 40; column++)
                    VDP_DATA = 0xc580;
        }
        VDP_CONTROL_WORD = 0x9297;
        func_00b64a();
    } else {
        if (!P1_MENU_OPEN[0] && !P1_MENU_OPEN[1])
            VDP_CONTROL_WORD = 0x9200;
        func_00abbe(1);
    }
    if (gPlayerLives[0] >= 0)
        func_00abbe(0);
    else
        func_00abbe(1);
    func_00ad8a(0);
    func_00ad8a(1);
    func_00b0cc(0);
    func_00b0cc(1);
    func_00b590(0);
    func_00b590(1);
    VDPWritesRoutine1(ram_ffb6b2, 0xf858, 48);
    VBLANK_FLAG_5 = 0;
}

/* Sum the promotion thresholds below rank eight. */
short GetScoreForPromo(rank)
short rank;
{
    short score = 40;
    short prior_rank;

    if (rank >= 8)
        score = 0;
    else
        for (prior_rank = 0; prior_rank < rank; ++prior_rank)
            score += (prior_rank + 3) * 20;
    return score;
}

int CheckPromotionScore(player_number)
short player_number;
{
    short rank;

    if (gPlayerLives[player_number] < 0)
        return 0;
    rank = gPlayerRank[player_number];
    if (rank < 8 && gPlayerPoints[player_number] >= GetScoreForPromo(rank)) {
        ++gPlayerRank[player_number];
        while (VBLANK_FLAG_2)
            ;
        func_00aeb8(player_number, 0);
        if (!SPLIT_SCREEN)
            VBLANK_FLAG_5 = 1;
        return 1;
    }
    return 0;
}

/* Hide the selected player_number's HUD sprites in VRAM and the sprite shadow. */
void func_00b95c(player_number)
short player_number;
{
    if (player_number == 0 || player_number == 2) {
        VDP_COMMAND = 0x78580003;
        VDP_DATA = 1;
        ram_ffb65a[11].y = 1;
        VDP_COMMAND = 0x78600003;
        VDP_DATA = 1;
        ram_ffb65a[12].y = 1;
        VDP_COMMAND = 0x78a80003;
        VDP_DATA = 1;
        ram_ffb65a[21].y = 1;
        VDP_COMMAND = 0x78780003;
        VDP_DATA = 1;
        ram_ffb65a[15].y = 1;
        VDP_COMMAND = 0x78800003;
        VDP_DATA = 1;
        ram_ffb65a[16].y = 1;
        VDP_COMMAND = 0x78980003;
        VDP_DATA = 1;
        ram_ffb65a[19].y = 1;
    }
    if (player_number == 1 || player_number == 2) {
        VDP_COMMAND = 0x78680003;
        VDP_DATA = 1;
        ram_ffb65a[13].y = 1;
        VDP_COMMAND = 0x78700003;
        VDP_DATA = 1;
        ram_ffb65a[14].y = 1;
        VDP_COMMAND = 0x78b00003;
        VDP_DATA = 1;
        ram_ffb65a[22].y = 1;
        VDP_COMMAND = 0x78880003;
        VDP_DATA = 1;
        ram_ffb65a[17].y = 1;
        VDP_COMMAND = 0x78900003;
        VDP_DATA = 1;
        ram_ffb65a[18].y = 1;
        VDP_COMMAND = 0x78a00003;
        VDP_DATA = 1;
        ram_ffb65a[20].y = 1;
    }
}

/* Recompute rank from points, restore starting resources, and refresh HUD data. */
void func_00baa2(player_number)
short player_number;
{
    gPlayerRank[player_number] = 0;
    /* ORIGINAL BUG: no rank-eight stop. Its zero threshold makes this loop
     * endless when points are at least the rank-seven threshold (880). */
    while (gPlayerPoints[player_number] >= GetScoreForPromo(gPlayerRank[player_number]))
        ++gPlayerRank[player_number];
    gPlayerBucks[player_number] = 0;
    if (PLAYER_INDICATOR == 2)
        gPlayerLives[player_number] = 0;
    else {
        gPlayerLives[player_number] = 3;
        gPlayerBucks[player_number] = 3;
    }
    TOEJAMS_HEALTH[player_number] = func_00ae90(player_number);
    TOEJAM_RESTORE_HEALTH[player_number] = -1;
    gPlayer1Powerup[player_number] = -1;
    func_00aeb8(player_number, 1);
    func_00acca(player_number);
    func_00b3e4(player_number);
}

void SetupTheMainGame()
{
    if (gPlayerLives[0] >= 0) {
        gToejamLevel = 1;
        TOEJAMS_HEALTH[0] = 23;
        gPlayerLives[0] = 3;
        gPlayerBucks[0] = 3;
    }
    if (gPlayerLives[1] >= 0) {
        gEarlLevel = 1;
        EARLS_HEALTH = 31;
        gPlayerLives[1] = 3;
        gPlayer2Bucks = 3;
    }
    EARL_RESTORE_HEALTH = -1;
    TOEJAM_RESTORE_HEALTH[0] = -1;
    gPlayer1Powerup[0] = -1;
    gPlayer2Powerup = -1;
    func_00aeb8(0, 1);
    func_00aeb8(1, 1);
    func_00aa5a(0);
    func_00aa5a(1);
    func_00acca(0);
    func_00acca(1);
    func_00b3e4(0);
    func_00b3e4(1);
}

/* Apply death, preserve/restore health around the observed state/mask cases,
 * and check promotion. */
void HealthAndLivesChecks(entity, terrain_class)
struct object_state *entity;
unsigned short terrain_class;
{
    short player_number = entity->entity_number;

    if (entity->sprite_state == 5 || entity->sprite_state == 6 ||
        ((long)entity->flags_52 & 0x20000) || gLevel912C == 26 ||
        ram_ffde5a[player_number])
        return;
    if (!TOEJAMS_MAX_HEALTH[player_number] && TOEJAMS_HEALTH[player_number] <= 0) {
        KillThePlayer(player_number);
        if (--gPlayerLives[player_number] < 0) {
            ++gPlayerLives[player_number];
            TOEJAM_RESTORE_HEALTH[player_number] = -1;
        } else {
            TOEJAMS_HEALTH[player_number] = 100;
            if (TOEJAM_RESTORE_HEALTH[player_number] != -1)
                TOEJAM_RESTORE_HEALTH[player_number] = 100;
        }
        return;
    }
    if (((long)entity->flags_52 & 0x800) || entity->sprite_state == 3 ||
        entity->sprite_state == 7) {
        if (TOEJAM_RESTORE_HEALTH[player_number] == -1)
            TOEJAM_RESTORE_HEALTH[player_number] = TOEJAMS_HEALTH[player_number];
        if (!(TIME_COUNTER & 12) && TOEJAMS_HEALTH[player_number] > 0)
            --TOEJAMS_HEALTH[player_number];
    } else {
        if (TOEJAM_RESTORE_HEALTH[player_number] != -1)
            TOEJAMS_HEALTH[player_number] = TOEJAM_RESTORE_HEALTH[player_number];
        TOEJAM_RESTORE_HEALTH[player_number] = -1;
    }
    if ((entity->draw_flags & 0x80) && entity->sprite_state != 2)
        entity->draw_flags &= 0x7f;
    if (CheckPromotionScore(player_number))
        PerformPromotion(player_number, 11, terrain_class);
}
