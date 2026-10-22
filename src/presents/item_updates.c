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

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Allocate a world-item object and configure its frames, size and position.
 * Non-present ground items set drawing flag 0x80; kinds 81 and up use the
 * larger 32-by-64 dimensions. */
void EntityCreateLevelItem(item)
struct level_item *item;
{
    struct object_state *object;

    if (item->object = (int)AllocateEntity()) {
        object = &gPlayerInfo[item->object];
        if (item->kind < 64)
            object->frames = &PresentSpriteTable[gPresentInfo[item->kind].sprite];
        else if (item->kind >= 81)
            object->frames = &FramePointers_AF09C[item->kind - 81];
        else {
            object->frames = &Spr_GroundItem[item->kind - 64];
            object->draw_flags |= 128;
        }
        object->sequence = SingleFrameHoldAnimation;
        object->frame_phase = 0;
        object->frame_duration = 1;
        object->x = item->x;
        object->y = item->y;
        object->level = item->level;
        if (item->kind < 81) {
            object->width = 24;
            object->height_pixels = 16;
            object->sprite_count = 1;
            object->tile_count = 12;
        } else {
            object->width = 32;
            object->height_pixels = 64;
            object->sprite_count = 2;
            object->tile_count = 32;
            object->flags_4d |= 1;
        }
        object->tile = 0x8000;
        object->draw_flags |= 8;
    } else if (item->kind < 81) {
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
        first_x = item->x - TOEJAM_CAMERA_NEWX;
        first_y = item->y - TOEJAM_CAMERA_NEWY;
        second_x = item->x - EARL_CAMERA_NEWX;
        second_y = item->y - EARL_CAMERA_NEWY;
        if (item->kind != -1 &&
            ((item->level == gLevel912C &&
              first_x + 20 > 0 && first_x < 340 && first_y + 20 > 0 &&
              ((item->kind < 81 && first_y < 244) ||
               (item->kind >= 81 && first_y < 288))) ||
             (item->level == gLevel912E && SPLIT_SCREEN &&
              second_x + 20 > 0 && second_x < 340 && second_y + 20 > 0 &&
              ((item->kind < 81 && second_y < 132) ||
               (item->kind >= 81 && second_y < 176))))) {
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
    TOEJAMS_HEALTH[player_number] += 8;
    GenerateSpeech(player_number, "yummm", 1);
}

/* Subtract eight health units and display the observed food speech. */
void EatYuckyFood(player_number)
short player_number;
{
    TOEJAMS_HEALTH[player_number] -= 8;
    GenerateSpeech(player_number, "yuck", 1);
}

/* Add twelve health units and request player response 6. */
void EatFudgeSundae(player_number)
short player_number;
{
    TOEJAMS_HEALTH[player_number] += 12;
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
        state->sprite_state == 3 || state->sprite_state == 7 ||
        /* (long) view of flags_52: the signed test reproduces the ROM's mask exchange; storage stays
         * unsigned ([INFERENCE] the original spelling is unknown). */
        state->sprite_state == 77 || ((long)state->flags_52 & 0x41800L))
        return;
    for (index = 0; index < count; index++) {
        item = &items[index];
        if (item->object) {
            collected = 0;
            if (abs(item->x - state->x) < 16 &&
                abs(item->y - state->y) < 8) {
                if (item->kind < 64) {
                    for (slot = 0; slot < 16; slot++) {
                        if (gPlayerInventory[player][slot] == -1) {
                            gPlayerInventory[player][slot] = item->kind;
                            collected = 1;
                            PlaySound(1, 50);
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
                    if (item->kind == 80) {
                        gPlayerBucks[player]++;
                        PlaySoundEffect((long)Sfx_Money, 50, 0);
                    } else {
                        slot = PresentAuxiliarySignedLookup[food];
                        if (slot > 0 && TOEJAMS_HEALTH[player] >= PlayerGetMaximumHealth(player)) {
                            GenerateSpeech(player, "I'm stuffed", 1);
                            PlaySoundEffect((long)Sfx_ChowDown, 50, 0);
                            /* ORIGINAL BUG: The full-health path still removes
                             * the item because collected remains set. */
                            goto pickup_done;
                        } else {
                            TOEJAMS_HEALTH[player] += slot;
                            if (slot > 0)
                                PlaySoundEffect((long)Sfx_ChowDown, 50, 0);
                            else
                                PlaySound(18, 40);
                        }
                    }
                    StartDialogue(food + 5, player);
                }
            }
pickup_done:
            if (collected) {
                item->kind = -1;
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
        if (item->kind == 83 && item->object &&
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
    terrain = (player == 0 ? LAND_UNDER_TOEJAM : LAND_UNDER_EARL) & 0xf0;
    if (gPlayer1Powerup[player] == 5)
        PlaySound(6, 99);
    if (state->height > 0 && (state->flags_52 & 0x4008000L)) {
        StopPlayer(state);
        state->acceleration_height = -32;
        sprite = (state->sprite_state & 3) + 20;
        gPlayerLandingCounterSeed[player] = 2;
    } else if (!(state->flags_52 & 0x20L) && state->height <= 0) {
        StopPlayer(state);
        gPlayer1Powerup[player] = -1;
        HudLoadActionIconSprite(player);
        HudPositionActionIconSprite(player);
        if ((state->flags_52 & 0x40000L) || state->sprite_state == 77 ||
            (((state->flags_52 & 0x7800000L) || (state->flags_52 & 0x8000L)) &&
             terrain == 128)) {
            sprite = (state->sprite_state & 3) + 64;
            state->mode_15 = 0;
            state->draw_flags &= ~64;
        } else if ((state->sprite_state == 0 || state->sprite_state == 4 ||
                    (state->flags_52 & 0x20400000L) || state->sprite_state == 76 ||
                    state->sprite_state == 3 || state->sprite_state == 7 ||
                    (state->flags_52 & 0x7800000L) ||
                    (state->flags_52 & 0x830c000L)) &&
                   !(state->flags_52 & 0x20000000L)) {
            sprite = GetActivePresentSprite(state);
            state->draw_flags &= ~64;
        }
    }
    if (sprite != -1)
        SetSprite(state, sprite);
}

/* Decrement each eligible player's active-present timer and blink the HUD
 * icon near expiration by testing the timer against masks 12 and 2. The
 * (&gPlayerInfo[player])->field spelling reproduces the ROM's schedule. */
void ReducePresentTimer()
{
    short player, show;

    for (player = 0; player <= 1; player++) {
        if (gPlayer1Powerup[player] >= 0 && gPlayerLives[player] >= 0 &&
            (&gPlayerInfo[player])->sprite_state != 5 &&
            (&gPlayerInfo[player])->sprite_state != 6 && !P1_MENU_OPEN[player]) {
            if (ELEVATOR_FLAG[player]) {
                if ((player == 0 && gHardwareSpriteTable[21].y == 1) ||
                    (player == 1 && gHardwareSpriteTable[22].y == 1))
                    HudPositionActionIconSprite(player);
            } else {
                show = 0;
                if (!gPlayer1PowerupTimer[player]) {
                    PresentTryEndActivePowerup(player);
                    if (gPlayer1Powerup[player] < 0)
                        show = 1;
                } else if (--gPlayer1PowerupTimer[player] > 300)
                    show = 1;
                else if (gPlayer1PowerupTimer[player] > 100) {
                    if (gPlayer1PowerupTimer[player] & 12)
                        show = 1;
                } else if (gPlayer1PowerupTimer[player] & 2)
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
        if (LEVEL_PLAYERS_FLAG_1[bank]) {
            item = gObjectTable[bank];
            for (index = 0; index < 28; index++, item++) {
                if (item->kind != -1 && item->kind < 64 && item->object)
                    gPlayerInfo[item->object].sprite_ready = 0;
            }
        }
    }
    item = gDroppedPresentTable;
    for (index = 0; index < 32; index++, item++) {
        if (item->kind != -1 && item->object)
            gPlayerInfo[item->object].sprite_ready = 0;
    }
}

/* Advance loaded and dropped item banks, process each live unpaused
 * player's nearby pickups and hazards, then update presents and the timer. */
void MapUpdateItemsAndPresents()
{
    short bank, player;

    if (P1_MENU_OPEN[0] && P1_MENU_OPEN[1])
        return;
    if (LEVEL_PLAYERS_FLAG_1[0])
        EntityUpdateLevelItemVisibility(gObjectTable[0], 32);
    if (LEVEL_PLAYERS_FLAG_2)
        EntityUpdateLevelItemVisibility(gObjectTable[1], 32);
    EntityUpdateLevelItemVisibility(gDroppedPresentTable, 32);
    for (player = 0; player <= 1; player++) {
        if (gPlayerLives[player] >= 0 && !P1_MENU_OPEN[player]) {
            for (bank = 0; bank < 2; bank++) {
                if (LEVEL_PLAYERS_FLAG_1[bank] &&
                    gPlayerInfo[player].level == LEVEL_LOADED_1[bank]) {
                    PlayerCollectNearbyItems(player, gObjectTable[bank], 28, 1);
                    PlayerCheckLevelItemHazards(player, &gObjectTable[bank][28]);
                }
            }
            PlayerCollectNearbyItems(player, gDroppedPresentTable, 32, 0);
        }
    }
    ReducePresentTimer();
    ProcessPresents();
    if (gLevel912C == 26)
        FunkatronUpdateSceneEntities();
}
