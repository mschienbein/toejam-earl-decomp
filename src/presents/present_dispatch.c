#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "psg_effects.h"
#include "revision.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Update each player's active present effects and nearby telephone. */
void ProcessPresents()
{
    short player_number;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        PresentUpdateDecoy(player_number);
        PresentUpdateBoomBox(player_number);
        PresentUpdateDoorway(player_number);
        PresentUpdateBurps(player_number);
        PresentUpdateRainCloud(player_number);
        PresentUpdateTransientEffect(player_number);
        EntityReleaseFinishedTimedEffect(player_number);
        CheckForTelephone(player_number);
        CheckTomatoRainTimer(player_number);
    }
}

/* Apply the selected present, rejecting it in states where it cannot open. */
int OpenSelectedPresent(player_number, present_number)
short player_number, present_number;
{
    struct object_state *player;
    unsigned short terrain;

    player = &gPlayerInfo[player_number];
    if (gPlayerElevatorFlags[player_number] || player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
        return 1;
    terrain = player_number == PLAYER_TOEJAM ? gPlayer1TerrainUnderfoot : gPlayer2TerrainUnderfoot;
    if (present_number == PRESENT_BONUS_HITOPS)
        present_number = PRESENT_SUPER_HITOPS;
    else if (present_number == PRESENT_DECOY) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenDecoy(player_number);
    } else if (present_number == PRESENT_DOORWAY)
        PresentIsDoorway(player_number);
    else if (present_number == PRESENT_EXTRA_LIFE || present_number == PRESENT_TOTAL_BUMMER)
        PerformPromotion(player_number, present_number, terrain);
    else if (present_number == PRESENT_PROMOTION)
        gPlayerPoints[player_number] = GetScoreForPromo(gPlayerRank[player_number]) - 2;
    else if (present_number == PRESENT_FOOD) {
        if (GetRandomNumber() % 100 < 70)
            EatYummyFood(player_number);
        else
            EatYuckyFood(player_number);
    } else if (present_number == PRESENT_ROOTBEER) {
        gPlayerBurpTimers[player_number] = -1;
        gPlayerBurpsLeft[player_number] += GetRandomNumber() % 7 + 12;
        gPlayerHealth[player_number] += 4;
    } else if (present_number == PRESENT_TOMATO_RAIN) {
        SetTomatoRainTomatoes(player_number);
        GenerateSpeech(player_number, "Uh oh?!?", 1);
        if (player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
            (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
            (player->animation_group_mask & PLAYER_GROUP_SIMPLE_REACTION_MOTION)) {
            SetSprite(player, PLAYER_ANIM_TOMATO_RAIN);
            StopPlayer(player);
            player->animation_motion_axes = ANIMATION_MOTION_NONE;
        }
    } else if (present_number == PRESENT_TOGETHERNESS) {
        if (gPendingUnfallPlayer)
            return 1;
        gPendingUnfallPlayer = player_number + 1;
    } else if (present_number == PRESENT_RAIN_CLOUD)
        PresentIsRainCloud(player_number);
    else if (present_number == PRESENT_FUDGE_SUNDAE)
        EatFudgeSundae(player_number);
    else if (present_number == PRESENT_TELEPHONE) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenTelephone(player_number);
    } else if (present_number == PRESENT_EXTRA_BUCK) {
        ++gPlayerBucks[player_number];
        StartDialogue(29, player_number);
    } else if (present_number == PRESENT_JACKPOT) {
        gPlayerBucks[player_number] += 5;
        StartDialogue(30, player_number);
    } else if (present_number == PRESENT_EARTHLING)
        OpenEarthlingPresent(player, terrain);
    else if (present_number == PRESENT_SCHOOL_BOOK) {
        if (terrain >= TERRAIN_WATER || player->height > 0)
            return 1;
        OpenSchoolBook(player);
    } else if (present_number == PRESENT_BOOMBOX) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenBoomBox(player_number);
    } else if (present_number == PRESENT_SPRING_SHOES && !player->level)
        return 1;
    SetupSelectedPresent(player, player_number, present_number, terrain);
    return 0;
}

/* Add opening points and initialize the present timer and eligible sprite. */
void SetupSelectedPresent(player, player_number, present_number, terrain)
struct object_state *player;
short player_number, present_number;
unsigned short terrain;
{
    char sprite;

    if (present_number < PRESENT_DOORWAY && present_number != PRESENT_ROCKET_SKATES && player->height > 0) {
        if (!gPlayerPowerups[player_number] && (player->animation_group_mask & PLAYER_GROUP_ICARUS_FLY) &&
            present_number != PRESENT_ICARUS_WINGS)
            PresentTryEndActivePowerup(player_number);
        else if (gPlayerPowerups[player_number] == PRESENT_SPRING_SHOES &&
                 !gPlayerFallStates[player_number] &&
                 (player->animation_group_mask & PLAYER_GROUP_FALL) && present_number != PRESENT_SPRING_SHOES) {
            StopPlayer(player);
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
        }
    }
    gPlayerPoints[player_number] += 2;
    if (present_number >= PRESENT_DOORWAY)
        return;
    gPlayerPowerups[player_number] = present_number;
    HudLoadActionIconSprite(player_number);
    gPlayerPowerupTimers[player_number] = 1000;
    sprite = -1;
    switch (present_number) {
    case PRESENT_ROCKET_SKATES:
        gPlayerPowerupTimers[player_number] >>= 2;
#if REVISION >= 2
        if (!gPlayerFallStates[player_number]) {
#endif
            PlaySound(PSG_START_BACKGROUND_NOISE, 94);
            OpenRocketSkates(player, player_number, terrain);
#if REVISION >= 2
        }
#endif
        break;
    case PRESENT_ICARUS_WINGS:
        gPlayerPowerupTimers[player_number] >>= 1;
        PlayerInitializePhasedJumpMotion(player_number);
        break;
    case PRESENT_TOMATOES:
        StartDialogue(26, player_number);
        break;
    case PRESENT_SLING_SHOT:
        StartDialogue(27, player_number);
        break;
    case PRESENT_ROSE_BUSHES:
        GenerateSpeech(player_number, "rosebushes!", 1);
        break;
    case PRESENT_SUPER_HITOPS:
        gPlayerPowerupTimers[player_number] = 750;
        StartDialogue(28, player_number);
        break;
    }
    if (present_number != PRESENT_ROCKET_SKATES)
        sprite = GetActivePresentSprite(player);
    if (sprite != -1 && !(player->animation_group_mask & PLAYER_GROUP_STATIONARY) &&
        (player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
         (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
         player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
#if REVISION >= 2
         (player->animation_group_mask & (PLAYER_GROUP_REFRESH_PRESENT | PLAYER_GROUP_HITOPS)))) {
#else
         (player->animation_group_mask & PLAYER_GROUP_REFRESH_PRESENT))) {
#endif
        SetSprite(player, sprite);
        player->velocity_x = 0;
        player->velocity_y = 0;
    }
}
