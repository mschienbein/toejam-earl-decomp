#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "psg_effects.h"
#include "hardware.h"
#include "entities.h"
#include "earthlings.h"
#include "presents.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_presents.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "graphics_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "ram_sprites.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* MATCHING QUALIFICATION: the explicit (int)AllocateEntity() result view
 * retains the observed call-result width with the shared short declaration.
 * Exact historical spelling remains unknown [INFERENCE]. */

/* Allocate a world-item object and configure its frames, size and position.
 * Non-present ground items set drawing flag 0x80; kinds 81 and up use the
 * larger 32-by-64 dimensions. */
void EntityCreateLevelItem(item)
struct level_item *item;
{
    struct object_state *object;

    if (item->object = (int)AllocateEntity()) {
        object = &gPlayerInfo[item->object];
        if (item->kind < ITEM_GROUND_BASE)
            object->frames = &PresentSpriteTable[gPresentInfo[item->kind].appearance];
        else if (item->kind >= ITEM_LARGE_BASE)
            object->frames = &LargeGroundItemFrames[item->kind - ITEM_LARGE_BASE];
        else {
            object->frames = &Spr_GroundItem[item->kind - ITEM_GROUND_BASE];
            object->draw_flags |= ENTITY_DRAW_LATE;
        }
        object->sequence = SingleFrameHoldAnimation;
        object->frame_phase = 0;
        object->frame_duration = 1;
        object->x = item->x;
        object->y = item->y;
        object->level = item->level;
        if (item->kind < ITEM_LARGE_BASE) {
            object->width = 24;
            object->height_pixels = 16;
            object->sprite_count = 1;
            object->tile_count = 12;
        } else {
            object->width = 32;
            object->height_pixels = 64;
            object->sprite_count = 2;
            object->tile_count = 32;
            object->behavior_flags |= ENTITY_COLLIDABLE;
        }
        object->tile = VDP_TILE_PRIORITY;
        object->draw_flags |= ENTITY_SKIP_PHYSICS;
    } else if (item->kind < ITEM_LARGE_BASE) {
        /* ORIGINAL BUG: MapFindItemPlacementPosition reads a fourth short argument, but
         * this call passes only the two coordinate pointers and the level. */
        MapFindItemPlacementPosition(&item->x, &item->y, item->level);
    }
}

/* Keep each item's object allocated only while a matching camera can see it.
 * The second camera is considered only during split-screen play. */
void EntityUpdateLevelItemVisibility(items, count)
struct level_item *items;
short count;
{
    short item_number, first_x, first_y, second_x, second_y;
    struct level_item *item;

    for (item_number = 0; item_number < count; ++item_number) {
        item = &items[item_number];
        first_x = item->x - gCamera1PendingX;
        first_y = item->y - gCamera1PendingY;
        second_x = item->x - gCamera2PendingX;
        second_y = item->y - gCamera2PendingY;
        if (item->kind != ITEM_UNUSED &&
            ((item->level == gPlayer1MapLevel &&
              first_x + 20 > 0 && first_x < 340 && first_y + 20 > 0 &&
              ((item->kind < ITEM_LARGE_BASE && first_y < 244) ||
               (item->kind >= ITEM_LARGE_BASE && first_y < 288))) ||
             (item->level == gPlayer2MapLevel && gSplitScreenEnabled &&
              second_x + 20 > 0 && second_x < 340 && second_y + 20 > 0 &&
              ((item->kind < ITEM_LARGE_BASE && second_y < 132) ||
               (item->kind >= ITEM_LARGE_BASE && second_y < 176))))) {
            if (!item->object)
                EntityCreateLevelItem(item);
        } else if (item->object) {
            FreeEntity(item->object);
            item->object = 0;
        }
    }
}

/* Release every allocated dropped-present object without changing its item. */
void PresentReleaseDroppedEntities()
{
    short item_number;
    struct level_item *item;

    item = gDroppedPresentTable;
    for (item_number = 0; item_number < 32; ++item_number, ++item) {
        if (item->object) {
            FreeEntity(item->object);
            item->object = 0;
        }
    }
}

/* Add eight health units and display the observed food speech. */
void EatYummyFood(player_number)
short player_number;
{
    gPlayerHealth[player_number] += 8;
    GenerateSpeech(player_number, "yummm", 1);
}

/* Subtract eight health units and display the observed food speech. */
void EatYuckyFood(player_number)
short player_number;
{
    gPlayerHealth[player_number] -= 8;
    GenerateSpeech(player_number, "yuck", 1);
}

/* Add twelve health units and request player response 6. */
void EatFudgeSundae(player_number)
short player_number;
{
    gPlayerHealth[player_number] += 12;
    StartDialogue(6, player_number);
}

/* Collect nearby present and food items, removing successful pickups and
 * recording fixed-level pickups in the per-level bit map. */
void PlayerCollectNearbyItems(player, items, count, record_pickup)
short player;
struct level_item *items;
short count;
char record_pickup;
{
    char collected;
    short index, food;
    short slot;
    struct level_item *item;
    struct object_state *state;

    state = &gPlayerInfo[player];
    if (gPlayerLives[player] < 0 || state->height >= 4 ||
        state->sprite_state == PLAYER_ANIM_SWIM_IDLE || state->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
        /* MATCHING QUALIFICATION: the signed-long animation_group_mask view
         * retains the observed mask/register exchange; storage stays unsigned. */
        state->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE || ((long)state->animation_group_mask & (long)(PLAYER_GROUP_SWIM | PLAYER_GROUP_CLIMB | PLAYER_GROUP_INNERTUBE_MOVE)))
        return;
    for (index = 0; index < count; index++) {
        item = &items[index];
        if (item->object) {
            collected = 0;
            if (abs(item->x - state->x) < 16 &&
                abs(item->y - state->y) < 8) {
                if (item->kind < ITEM_GROUND_BASE) {
                    for (slot = 0; slot < 16; slot++) {
                        if (gPlayerInventory[player][slot] == PRESENT_NONE) {
                            gPlayerInventory[player][slot] = item->kind;
                            collected = 1;
                            PlaySound(PSG_DOUBLE_TONE_PULSE, AUDIO_PRIORITY_NORMAL);
                            EarthlingClearMolePlayerContactFlags(state);
                            break;
                        }
                    }
                    if (slot == 16 && !HasSpeakerSpeech(player))
                        StartDialogue(25, player);
                } else {
                    collected = 1;
                    food = item->kind;
                    food -= 64;
                    if (item->kind == ITEM_BUCK) {
                        gPlayerBucks[player]++;
                        PlaySoundEffect((long)Sfx_Money, AUDIO_PRIORITY_NORMAL, 0);
                    } else {
                        slot = PresentAuxiliarySignedLookup[food];
                        if (slot > 0 && gPlayerHealth[player] >= PlayerGetMaximumHealth(player)) {
                            GenerateSpeech(player, "I'm stuffed", 1);
                            PlaySoundEffect((long)Sfx_ChowDown, AUDIO_PRIORITY_NORMAL, 0);
                            /* ORIGINAL BUG: The full-health path still removes
                             * the item because collected remains set. */
                            goto pickup_done;
                        } else {
                            gPlayerHealth[player] += slot;
                            if (slot > 0)
                                PlaySoundEffect((long)Sfx_ChowDown, AUDIO_PRIORITY_NORMAL, 0);
                            else
                                PlaySound(PSG_TONE_NOISE_BURSTS, 40);
                        }
                    }
                    StartDialogue(food + 5, player);
                }
            }
pickup_done:
            if (collected) {
                item->kind = ITEM_UNUSED;
                if (record_pickup)
                    gCollectedLevelItemBits[item->level] |= (unsigned long)0x80000000L >> index;
            }
        }
    }
}

/* Nearby active items of kind 83 call the same hurt handler as earthlings. */
void PlayerCheckLevelItemHazards(player, items)
short player;
struct level_item *items;
{
    struct earthling_state source;
    short index;
    struct level_item *item;
    struct object_state *state;

    state = &gPlayerInfo[player];
    if (state->height > 0)
        return;
    for (index = 0; index < 4; index++) {
        item = &items[index];
        if (item->kind == ITEM_HURT_HAZARD && item->object &&
            abs(item->x - state->x) < 24 &&
            abs(item->y - state->y) < 12) {
            /* The accepted reserved byte supplies the hurt-source kind. */
            source.kind = 1;
            source.x = item->x;
            source.y = item->y;
            source.object = item->object;
            Player_Youch(player, &source);
        }
    }
}

/* End an active present, restoring a terrain-appropriate sprite when the
 * player's current height and animation flags permit it. */
void PresentTryEndActivePowerup(player)
short player;
{
    char sprite = -1;
    struct object_state *state;
    short terrain;

    state = &gPlayerInfo[player];
    terrain = (player == 0 ? gPlayer1TerrainUnderfoot : gPlayer2TerrainUnderfoot) & 0xf0;
    if (gPlayerPowerups[player] == PRESENT_ROCKET_SKATES)
        PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
    if (state->height > 0 && (state->animation_group_mask & (long)(PLAYER_GROUP_ICARUS_FLY | PLAYER_GROUP_SKATES_AIR))) {
        StopPlayer(state);
        state->acceleration_height = -32;
        sprite = (state->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
        gPlayerLandingCounterSeed[player] = 2;
    } else if (!(state->animation_group_mask & (long)PLAYER_GROUP_FALL) && state->height <= 0) {
        StopPlayer(state);
        gPlayerPowerups[player] = PRESENT_NONE;
        HudLoadActionIconSprite(player);
        HudPositionActionIconSprite(player);
        if ((state->animation_group_mask & (long)PLAYER_GROUP_INNERTUBE_MOVE) || state->sprite_state == PLAYER_ANIM_INNERTUBE_WATER_IDLE ||
            (((state->animation_group_mask & (long)PLAYER_GROUP_ALL_SKATES) || (state->animation_group_mask & (long)PLAYER_GROUP_ICARUS_FLY)) &&
             terrain == TERRAIN_WATER)) {
            sprite = (state->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_SPLASH_UP;
            state->animation_motion_axes = ANIMATION_MOTION_NONE;
            state->draw_flags &= ~ENTITY_SHADOW;
        } else if ((state->sprite_state == PLAYER_ANIM_IDLE || state->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
                    (state->animation_group_mask & (long)(PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || state->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
                    state->sprite_state == PLAYER_ANIM_SWIM_IDLE || state->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
                    (state->animation_group_mask & (long)PLAYER_GROUP_ALL_SKATES) ||
                    (state->animation_group_mask & PLAYER_GROUP_REFRESH_POWERUP_END)) &&
                   !(state->animation_group_mask & (long)PLAYER_GROUP_STATIONARY)) {
            sprite = GetActivePresentSprite(state);
            state->draw_flags &= ~ENTITY_SHADOW;
        }
    }
    if (sprite != -1)
        SetSprite(state, sprite);
}

/* Decrement each eligible player's active-present timer and blink the HUD
 * icon near expiration by testing the timer against masks 12 and 2.
 * MATCHING QUALIFICATION: (&gPlayerInfo[player])->field retains the
 * observed scheduling; direct members and pointer-local alternatives shrink
 * the function in the accepted controls. */
void ReducePresentTimer()
{
    short player, show;

    for (player = 0; player <= PLAYER_EARL; player++) {
        if (gPlayerPowerups[player] >= 0 && gPlayerLives[player] >= 0 &&
            (&gPlayerInfo[player])->sprite_state != PLAYER_ANIM_GHOST &&
            (&gPlayerInfo[player])->sprite_state != PLAYER_ANIM_LAST_LIFE_GHOST && !gPlayerInventoryOpen[player]) {
            if (gPlayerElevatorFlags[player]) {
                if ((player == 0 && gHardwareSpriteTable[21].y == 1) ||
                    (player == 1 && gHardwareSpriteTable[22].y == 1))
                    HudPositionActionIconSprite(player);
            } else {
                show = 0;
                if (!gPlayerPowerupTimers[player]) {
                    PresentTryEndActivePowerup(player);
                    if (gPlayerPowerups[player] < 0)
                        show = 1;
                } else if (--gPlayerPowerupTimers[player] > 300)
                    show = 1;
                else if (gPlayerPowerupTimers[player] > 100) {
                    if (gPlayerPowerupTimers[player] & 12)
                        show = 1;
                } else if (gPlayerPowerupTimers[player] & 2)
                    show = 1;
                if (show)
                    HudPositionActionIconSprite(player);
                else if (player == 0)
                    gHardwareSpriteTable[21].y = 1;
                else
                    gHardwareSpriteTable[22].y = 1;
            }
        }
    }
}

/* Clear the referenced object state byte for active presents in loaded
 * level banks and in the dropped-present table. */
void PresentClearWorldEntityReadiness()
{
    short bank, index;
    struct level_item *item;

    for (bank = 0; bank < 2; bank++) {
        if (gItemBankUsers[bank]) {
            item = gObjectTable[bank];
            for (index = 0; index < 28; index++, item++) {
                if (item->kind != ITEM_UNUSED && item->kind < ITEM_GROUND_BASE && item->object)
                    gPlayerInfo[item->object].sprite_ready = 0;
            }
        }
    }
    item = gDroppedPresentTable;
    for (index = 0; index < 32; index++, item++) {
        if (item->kind != ITEM_UNUSED && item->object)
            gPlayerInfo[item->object].sprite_ready = 0;
    }
}

/* Advance loaded and dropped item banks, process each live unpaused
 * player's nearby pickups and hazards, then update presents and the timer. */
void MapUpdateItemsAndPresents()
{
    short bank, player;

    if (gPlayerInventoryOpen[PLAYER_TOEJAM] && gPlayerInventoryOpen[PLAYER_EARL])
        return;
    if (gItemBankUsers[0])
        EntityUpdateLevelItemVisibility(gObjectTable[0], 32);
    if (gItemBank2Users)
        EntityUpdateLevelItemVisibility(gObjectTable[1], 32);
    EntityUpdateLevelItemVisibility(gDroppedPresentTable, 32);
    for (player = 0; player <= PLAYER_EARL; player++) {
        if (gPlayerLives[player] >= 0 && !gPlayerInventoryOpen[player]) {
            for (bank = 0; bank < 2; bank++) {
                if (gItemBankUsers[bank] &&
                    gPlayerInfo[player].level == gItemBankLevels[bank]) {
                    PlayerCollectNearbyItems(player, gObjectTable[bank], 28, 1);
                    PlayerCheckLevelItemHazards(player, &gObjectTable[bank][28]);
                }
            }
            PlayerCollectNearbyItems(player, gDroppedPresentTable, 32, 0);
        }
    }
    ReducePresentTimer();
    ProcessPresents();
    if (gPlayer1MapLevel == LEVEL_FINAL)
        FunkatronUpdateSceneEntities();
}
