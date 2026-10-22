#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#define TJE_LOCAL_RANKCHARSTABLE

#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_ui.h"
#include "hardware_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "ui_calls.h"

/* Compatible incomplete-array compiler view. The actual complete shared
 * declaration changes the observed instructions; historical source spelling
 * is unknown [INFERENCE]. */
extern char *RankCharsTable[];

/* Position the life icons for the menu, split-screen, or combined HUD. */
void HudPositionLivesSprite(player)
short player;
{
    short sprite_number, y, lives, x;

    if (player == 0) {
        sprite_number = 19;
        y = 226;
    } else {
        sprite_number = 20;
        y = 329;
    }
    lives = gPlayerLives[player];
    if (P1_MENU_OPEN[player])
        x = 298;
    else if (SPLIT_SCREEN || P1_MENU_OPEN[player ^ 1]) {
        /* MATCHING QUALIFICATION: the redundant default-only switch keeps
         * the observed TST/BNE.S/BRA.W join. This is compiler coaxing, not
         * evidence that the historical source used this control structure. */
        if (player == 0) {
            switch (player) {
            default:
                x = 138;
                break;
            }
        } else
            x = 138;
        y = 5;
    } else {
        if (player == 0)
            x = 228;
        else
            x = 398;
        y = 330;
    }
    if (lives < 5)
        y -= 3;
    if (gPlayerLives[player] < 0)
        y = 1;
    gHardwareSpriteTable[sprite_number].x = x;
    gHardwareSpriteTable[sprite_number].y = y;
}

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
/* Print each active player's rank and the absent player's vacation label. */
void HudDrawRankLabels()
{
    if (PLAYER_INDICATOR == 2) {
        VDP_COMMAND = 0x5c160003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_TOEJAM]]);
        VDP_COMMAND = 0x5c400003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_EARL]]);
    } else if (PLAYER_INDICATOR == 0) {
        VDP_COMMAND = 0x5c160003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_TOEJAM]]);
        VDP_COMMAND = 0x5c3c0003;
        TextWriteStringTiles(" on");
        VDP_COMMAND = 0x5cba0003;
        TextWriteStringTiles("vacation");
    } else {
        VDP_COMMAND = 0x5c400003;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_EARL]]);
        VDP_COMMAND = 0x5c120003;
        TextWriteStringTiles(" on");
        VDP_COMMAND = 0x5c900003;
        TextWriteStringTiles("vacation");
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
    if (!SPLIT_SCREEN && !P1_MENU_OPEN[PLAYER_TOEJAM] && !P1_MENU_OPEN[PLAYER_EARL]) {
        for (row = 0; row < 5; row++) {
            VDP_COMMAND = VDP_WRITE_WINDOW + ((row + 23) << 23);
            if (row < 4) {
                for (column = 0; column < 40; column++) {
                    tile = CombinedHudBackgroundByteMap[row][column];
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
        HudDrawRankLabels();
    } else {
        if (!P1_MENU_OPEN[PLAYER_TOEJAM] && !P1_MENU_OPEN[PLAYER_EARL])
            VDP_CONTROL_WORD = 0x9200;
        HudPositionLevelNumberSprites(1);
    }
    if (gPlayerLives[PLAYER_TOEJAM] >= 0)
        HudPositionLevelNumberSprites(0);
    else
        HudPositionLevelNumberSprites(1);
    HudPositionActionIconSprite(0);
    HudPositionActionIconSprite(1);
    HudPositionHealthBarSprites(0);
    HudPositionHealthBarSprites(1);
    HudPositionLivesSprite(0);
    HudPositionLivesSprite(1);
    VDPWritesRoutine1(gHudHardwareSpriteBytes, 0xf858, 48);
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
        HudInitializeHealthBar(player_number, 0);
        if (!SPLIT_SCREEN)
            VBLANK_FLAG_5 = 1;
        return 1;
    }
    return 0;
}

/* Hide the selected player_number's HUD sprites in VRAM and the sprite shadow. */
void HudHidePlayerSprites(player_number)
short player_number;
{
    if (player_number == PLAYER_TOEJAM || player_number == 2) {
        VDP_COMMAND = 0x78580003;
        VDP_DATA = 1;
        gHardwareSpriteTable[11].y = 1;
        VDP_COMMAND = 0x78600003;
        VDP_DATA = 1;
        gHardwareSpriteTable[12].y = 1;
        VDP_COMMAND = 0x78a80003;
        VDP_DATA = 1;
        gHardwareSpriteTable[21].y = 1;
        VDP_COMMAND = 0x78780003;
        VDP_DATA = 1;
        gHardwareSpriteTable[15].y = 1;
        VDP_COMMAND = 0x78800003;
        VDP_DATA = 1;
        gHardwareSpriteTable[16].y = 1;
        VDP_COMMAND = 0x78980003;
        VDP_DATA = 1;
        gHardwareSpriteTable[19].y = 1;
    }
    if (player_number == PLAYER_EARL || player_number == 2) {
        VDP_COMMAND = 0x78680003;
        VDP_DATA = 1;
        gHardwareSpriteTable[13].y = 1;
        VDP_COMMAND = 0x78700003;
        VDP_DATA = 1;
        gHardwareSpriteTable[14].y = 1;
        VDP_COMMAND = 0x78b00003;
        VDP_DATA = 1;
        gHardwareSpriteTable[22].y = 1;
        VDP_COMMAND = 0x78880003;
        VDP_DATA = 1;
        gHardwareSpriteTable[17].y = 1;
        VDP_COMMAND = 0x78900003;
        VDP_DATA = 1;
        gHardwareSpriteTable[18].y = 1;
        VDP_COMMAND = 0x78a00003;
        VDP_DATA = 1;
        gHardwareSpriteTable[20].y = 1;
    }
}

/* Recompute rank from points, restore starting resources, and refresh HUD data. */
void PlayerResetRankAndResources(player_number)
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
    TOEJAMS_HEALTH[player_number] = PlayerGetMaximumHealth(player_number);
    TOEJAM_RESTORE_HEALTH[player_number] = -1;
    gPlayer1Powerup[player_number] = PRESENT_NONE;
    HudInitializeHealthBar(player_number, 1);
    HudLoadActionIconSprite(player_number);
    HudLoadLivesSpriteTiles(player_number);
}

void SetupTheMainGame()
{
    if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        gToejamLevel = 1;
        TOEJAMS_HEALTH[PLAYER_TOEJAM] = 23;
        gPlayerLives[PLAYER_TOEJAM] = 3;
        gPlayerBucks[PLAYER_TOEJAM] = 3;
    }
    if (gPlayerLives[PLAYER_EARL] >= 0) {
        gEarlLevel = 1;
        EARLS_HEALTH = 31;
        gPlayerLives[PLAYER_EARL] = 3;
        gPlayer2Bucks = 3;
    }
    EARL_RESTORE_HEALTH = -1;
    TOEJAM_RESTORE_HEALTH[PLAYER_TOEJAM] = -1;
    gPlayer1Powerup[PLAYER_TOEJAM] = PRESENT_NONE;
    gPlayer2Powerup = PRESENT_NONE;
    HudInitializeHealthBar(0, 1);
    HudInitializeHealthBar(1, 1);
    HudLoadLevelNumberSprites(0);
    HudLoadLevelNumberSprites(1);
    HudLoadActionIconSprite(0);
    HudLoadActionIconSprite(1);
    HudLoadLivesSpriteTiles(0);
    HudLoadLivesSpriteTiles(1);
}

/* Apply death, preserve/restore health around the observed state/mask cases,
 * and check promotion. */
void HealthAndLivesChecks(entity, terrain_class)
struct object_state *entity;
unsigned short terrain_class;
{
    short player_number = entity->entity_index;

    if (entity->sprite_state == PLAYER_ANIM_GHOST || entity->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST ||
        ((long)entity->animation_group_mask & PLAYER_GROUP_REACTION) || gLevel912C == 26 ||
        gPlayerPromotionDisplayEntities[player_number])
        return;
    if (!gHudDisplayedHealth[player_number] && TOEJAMS_HEALTH[player_number] <= 0) {
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
    if (((long)entity->animation_group_mask & PLAYER_GROUP_SWIM) || entity->sprite_state == PLAYER_ANIM_SWIM_IDLE ||
        entity->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE) {
        if (TOEJAM_RESTORE_HEALTH[player_number] == -1)
            TOEJAM_RESTORE_HEALTH[player_number] = TOEJAMS_HEALTH[player_number];
        if (!(TIME_COUNTER & 12) && TOEJAMS_HEALTH[player_number] > 0)
            --TOEJAMS_HEALTH[player_number];
    } else {
        if (TOEJAM_RESTORE_HEALTH[player_number] != -1)
            TOEJAMS_HEALTH[player_number] = TOEJAM_RESTORE_HEALTH[player_number];
        TOEJAM_RESTORE_HEALTH[player_number] = -1;
    }
    if ((entity->draw_flags & ENTITY_DRAW_LATE) && entity->sprite_state != PLAYER_ANIM_FLATTENED)
        entity->draw_flags &= ~ENTITY_DRAW_LATE;
    if (CheckPromotionScore(player_number))
        PerformPromotion(player_number, 11, terrain_class);
}
