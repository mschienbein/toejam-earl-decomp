#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "ram_system.h"

#include "entities.h"
#define TJE_LOCAL_GETMAPTERRAINATWORLDPOSITION

#include "presents.h"
#include "elevator.h"
#include "assets_map.h"
#include "assets_presents.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "random.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared unsigned char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetMapTerrainAtWorldPosition();
/* Reserve the two fixed identities, then assign the other present sprites
 * to random unused identities, scanning forward through collisions. */
void SetupPresentIdentities()
{
    short present_number, identity;

    for (present_number = 0; present_number < 27; ++present_number) {
        gPresentInfo[present_number].appearance = -1;
        gPresentInfo[present_number].identified = 0;
    }
    gPresentInfo[PRESENT_MYSTERY].appearance = 26;
    gPresentInfo[PRESENT_BONUS_HITOPS].appearance = 27;
    gPresentInfo[PRESENT_BONUS_HITOPS].identified = 1;
    for (present_number = 0; present_number < 26; ++present_number) {
        identity = GetRandomNumber() % 27;
        while (gPresentInfo[identity].appearance != -1)
            identity = (identity + 1) % 27;
        gPresentInfo[identity].appearance = present_number;
    }
}

/* Clear both inventories and the three world-item banks. Each active player
 * receives two fixed presents, or four when the other player is absent. */
void SetupInitialInventory()
{
    short slot;

    for (slot = 0; slot < 16; ++slot) {
        gPlayerInventory[PLAYER_TOEJAM][slot] = PRESENT_NONE;
        gPlayerInventory[PLAYER_EARL][slot] = PRESENT_NONE;
    }
    for (slot = 0; slot < 32; ++slot) {
        gDroppedPresentTable[slot].kind = ITEM_UNUSED;
        gObjectTable[0][slot].kind = ITEM_UNUSED;
        gItemBank2Records[slot].kind = ITEM_UNUSED;
    }
    gItemBankLevels[0] = -1;
    gItemBank2Level = -1;
    SetupPresentIdentities();
    if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        LoadLevelItemBank(1);
        gPlayerInventory[PLAYER_TOEJAM][0] = PRESENT_BONUS_HITOPS;
        gPlayerInventory[PLAYER_TOEJAM][1] = PRESENT_BONUS_HITOPS;
        if (gPlayer2Lives < 0) {
            gPlayerInventory[PLAYER_TOEJAM][2] = PRESENT_BONUS_HITOPS;
            gPlayerInventory[PLAYER_TOEJAM][3] = PRESENT_BONUS_HITOPS;
        }
    }
    if (gPlayer2Lives >= 0) {
        LoadLevelItemBank(1);
        gPlayerInventory[PLAYER_EARL][0] = PRESENT_BONUS_HITOPS;
        gPlayerInventory[PLAYER_EARL][1] = PRESENT_BONUS_HITOPS;
        if (gPlayerLives[PLAYER_TOEJAM] < 0) {
            gPlayerInventory[PLAYER_EARL][2] = PRESENT_BONUS_HITOPS;
            gPlayerInventory[PLAYER_EARL][3] = PRESENT_BONUS_HITOPS;
        }
    }
}

/* Release a loaded item bank after its last player leaves this level. */
void MapReleaseLevelItemBank(level)
short level;
{
    short bank, item_number;
    char object;

    for (bank = 0; bank < 2; ++bank) {
        if (gItemBankUsers[bank] && gItemBankLevels[bank] == level) {
            --gItemBankUsers[bank];
            if (!gItemBankUsers[bank]) {
                gItemBankLevels[bank] = -1;
                for (item_number = 0; item_number < 32; ++item_number) {
                    object = gObjectTable[bank][item_number].object;
                    gObjectTable[bank][item_number].kind = ITEM_UNUSED;
                    if (object) {
                        FreeEntity(object);
                        gObjectTable[bank][item_number].object = 0;
                    }
                }
            }
        }
    }
}

/* Level 1 always permits placement. On other levels reject positions within
 * 30 X units and 20 Y units of either converted elevator coordinate. */
int ElevatorAllowsWorldPlacement(world_x, world_y, level)
short world_x, world_y, level;
{
    short elevator_number, elevator_x, elevator_y;
    char map_x, map_y;

    if (level == 1)
        return 1;
    for (elevator_number = 0; elevator_number < 2; ++elevator_number) {
        map_x = gLevelAnchorPositions[elevator_number].x;
        map_y = gLevelAnchorPositions[elevator_number].y;
        elevator_x = (map_x + 2) * 16 * 8;
        elevator_x -= 16;
        elevator_y = (map_y + 2) * 8 * 8;
        elevator_y -= 5;
        if (abs(world_x - elevator_x) < 30 &&
            abs(world_y - (elevator_y + 5)) < 20)
            return 0;
    }
    return 1;
}

/* Test the low seven terrain bits against a 23-entry terrain table. */
int MapIsLargePlacementTerrainExcluded(terrain)
short terrain;
{
    short entry;

    terrain &= METATILE_BASE_MASK;
    for (entry = 0; entry < 23; ++entry)
        if (LargeItemExcludedTerrainIds[entry] == terrain)
            return 1;
    return 0;
}

/* Try ten random eligible metatiles, then scan cyclically from a random tile. */
void MapFindItemPlacementPosition(output_x, output_y, level, large)
short *output_x, *output_y;
short level, large;
{
    unsigned short metatile;
    short start_column, width, height;
    unsigned short *map;
    short position_x, position_y;
    short column, row, search_counter;
    register unsigned short random;

    if (gPlayer1MapLevel == level)
        map = gPlayer1LevelMap[0];
    else
        map = gPlayer2LevelMap[0];
    if (large) {
        width = 4;
        height = 2;
    } else {
        width = 3;
        height = 1;
    }
    for (search_counter = 0; search_counter < 10; ++search_counter) {
        random = GetRandomNumber();
        row = random % 26;
        random = GetRandomNumber();
        column = random % 19;
        metatile = map[row * 19 + column];
        if ((metatile & 0x2000) || (large && MapIsLargePlacementTerrainExcluded(metatile))) {
            /* ORIGINAL BUG: rejected marked tiles do not consume an attempt. */
            --search_counter;
            continue;
        }
        if (TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
            position_y += (row + 1) * 64;
            position_x += (column + 1) * 128;
            if (ElevatorAllowsWorldPlacement(position_x, position_y, level)) {
                *output_x = position_x;
                *output_y = position_y;
                return;
            }
        }
    }
    random = GetRandomNumber();
    search_counter = random % 26;
    random = GetRandomNumber();
    start_column = random % 19;
    row = search_counter;
    do {
        column = start_column;
        do {
            metatile = map[row * 19 + column];
            if ((metatile & 0x2000) || (large && MapIsLargePlacementTerrainExcluded(metatile))) {
                column = (column + 1) % 19;
                continue;
            }
            if (TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
                position_y += (row + 1) * 64;
                position_x += (column + 1) * 128;
                if (ElevatorAllowsWorldPlacement(position_x, position_y, level)) {
                    *output_x = position_x;
                    *output_y = position_y;
                    return;
                }
            }
            column = (column + 1) % 19;
        } while (column != start_column);
        row = (row + 1) % 26;
    } while (row != search_counter);
}

/* Place four special objects after the first 28 item slots, avoiding nearby
 * items, elevators and positions rejected by EarthlingIsSpecialRecordNearby. */
void MapPlaceLargeLevelItems(items, level)
struct level_item *items;
short level;
{
    short current_x, current_y, slot, attempts, map_index;
    short position_x, position_y, item_number, blocked;
    char map_x, map_y;

    if (gPlayer1MapLevel == level)
        map_index = 0;
    else
        map_index = 1;
    for (slot = 28; slot < 32; ++slot) {
        for (attempts = 0; attempts < 10; ++attempts) {
            MapFindItemPlacementPosition(&position_x, &position_y, level, 1);
            blocked = 0;
            for (item_number = 28; item_number < slot; ++item_number) {
                current_x = items[item_number].x;
                current_y = items[item_number].y;
                if (abs(position_x - current_x) < 340 &&
                    abs(position_y - current_y) < 150) {
                    blocked = 1;
                    break;
                }
            }
            if (blocked)
                continue;
            for (item_number = 0; item_number < 28; ++item_number) {
                if (items[item_number].kind != -1) {
                    current_x = items[item_number].x;
                    current_y = items[item_number].y;
                    if (abs(position_x - current_x) < 16 &&
                        abs(position_y - current_y) < 10) {
                        blocked = 1;
                        break;
                    }
                }
            }
            if (blocked)
                continue;
            for (item_number = 0; item_number < 2; ++item_number) {
                map_x = gLevelAnchorPositions[item_number].x;
                map_y = gLevelAnchorPositions[item_number].y;
                current_x = (map_x + 2) * 16 * 8;
                current_x -= 16;
                current_y = (map_y + 2) * 8 * 8;
                current_y -= 5;
                if (abs(position_x - current_x) < 340 &&
                    abs(position_y - current_y) < 150) {
                    blocked = 1;
                    break;
                }
            }
            if (blocked)
                continue;
            if (EarthlingIsSpecialRecordNearby(position_x, position_y, level))
                blocked = 1;
            if (!blocked)
                break;
        }
        if (attempts == 10)
            items[slot].kind = ITEM_UNUSED;
        else {
            if (GetMapTerrainAtWorldPosition(map_index, position_x, position_y) == TERRAIN_SINKING) {
                if (GetRandomNumber() & 1)
                    items[slot].kind = ITEM_LARGE_UNIDENTIFIED;
                else
                    items[slot].kind = ITEM_HURT_HAZARD;
            } else
                items[slot].kind = ITEM_LARGE_BASE;
            items[slot].level = level;
            items[slot].x = position_x;
            items[slot].y = position_y;
        }
    }
}

/* Select a present using the 27 cumulative byte weights, retrying a zero draw. */
int SelectRandomPresent()
{
    short total, selected, present;
    register unsigned short random;

    total = 0;
    for (present = 0; present < 27; ++present)
        total += PresentWeightTable[present];
    do {
        random = GetRandomNumber();
        selected = random % (total + 1);
    } while (!selected);
    total = 0;
    for (present = 0; present < 27; ++present) {
        total += PresentWeightTable[present];
        if (selected <= total)
            return present;
    }
    return present;
}

/* Reuse an already loaded item bank or populate a free bank from its level seed. */
void LoadLevelItemBank(level)
short level;
{
    short bank, slot, count;
    short position_x, position_y;
    char kind;
    long saved_seed;

    if (level >= 26)
        return;
    for (bank = 0; bank < 2; ++bank) {
        if (gItemBankUsers[bank] && gItemBankLevels[bank] == level) {
            ++gItemBankUsers[bank];
            return;
        }
    }
    bank = 0;
    if (gItemBankUsers[0])
        bank = 1;
    saved_seed = StackBasedRandom(gLevelSeeds[level]);
    if (gPlayerLives[PLAYER_TOEJAM] < 0 || gPlayer2Lives < 0)
        count = 12;
    else
        count = 16;
    if (level > 2)
        count += level - 2;
    if (count > 28)
        count = 28;
    for (slot = 0; slot < count; ++slot) {
        if (!level)
            break;
        if (level == 1) {
            if (slot == 12)
                break;
            position_x = Level1PresentLocations[slot].x;
            position_y = Level1PresentLocations[slot].y;
        } else
            MapFindItemPlacementPosition(&position_x, &position_y, level, 0);
        if (level == 1)
            goto get_present;
        if (GetRandomNumber() % 100 < 50) {
get_present:
            kind = SelectRandomPresent();
            if (level == 1 && slot < 4) {
                while (kind == 0 || kind == 2 || kind == 5 || kind == 26 ||
                       kind == 18 || kind == 16)
                    kind = SelectRandomPresent();
            }
        } else {
            if (GetRandomNumber() % 100 < 75) {
                kind = GetRandomNumber() % 16;
                kind += 64;
            } else
                kind = 80;
        }
        if (!(gCollectedLevelItemBits[level] & ((unsigned long)0x80000000 >> slot))) {
            gObjectTable[bank][slot].kind = kind;
            gObjectTable[bank][slot].level = level;
            gObjectTable[bank][slot].x = position_x;
            gObjectTable[bank][slot].y = position_y;
        } else {
            gObjectTable[bank][slot].kind = ITEM_UNUSED;
        }
    }
    if (!level)
        InitializeLevelZeroFixedItems(
            &gObjectTable[bank][28]);
    else if (level != 1)
        MapPlaceLargeLevelItems(gObjectTable[bank], level);
    ++gItemBankUsers[bank];
    gItemBankLevels[bank] = level;
    StackBasedRandom(saved_seed);
}

/* Search the sample perimeter in a randomized orientation, retrying without
 * the dropped-present overlap check.
 *
 * MATCHING QUALIFICATION: scratch is deliberately shared by the coordinate
 * exchange and the later dropped-present scan, including its 101 sentinel.
 * These lifetimes do not overlap: the scan initializes scratch before use,
 * and the next exchange overwrites it before use. Separate meaningful locals
 * select different registers. No dummy state or no-op expression is needed.
 * No natural spelling tried matched with the pinned MCC68K 3.3K compiler.
 */
void DropPresent(player_number, present)
short player_number, present;
{
    short row, column, scratch, offset_x, offset_y, position_x, position_y;
    unsigned short random;
    char pass;
    unsigned char tile;
    unsigned char *land;
    struct object_state *player;
    struct level_item *destination;

    player = &gPlayerInfo[player_number];
    if (((long)player->animation_group_mask & (long)PLAYER_GROUP_SWIM) ||
        player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE)
        return;
    if (player_number == PLAYER_TOEJAM)
        land = &gPlayer1NearbyTerrain[0][0];
    else
        land = &gPlayer2NearbyTerrain[0][0];
    destination = &gDroppedPresentTable[gDroppedPresentTableIdx];
    if (destination->kind != PRESENT_NONE && destination->object) {
        FreeEntity(destination->object);
        destination->object = 0;
    }
    random = gCachedRandomValues[gRandomCacheIndex == 58 ?
        (gRandomCacheIndex = 0) : ++gRandomCacheIndex];
    for (pass = 0; pass < 2; ++pass)
        for (row = 0; row < 5; ++row)
            for (column = 0; column < 5; ++column) {
                offset_y = row;
                if (random & 1)
                    offset_y = 4 - offset_y;
                offset_x = column;
                if (random & 2)
                    offset_x = 4 - offset_x;
                if (random & 4) {
                    scratch = offset_y;
                    offset_y = offset_x;
                    offset_x = scratch;
                }
                if (row && row != 4 && column && column != 4)
                    continue;
                tile = land[offset_y * 5 + offset_x];
                if (tile == TERRAIN_VOID || tile == TERRAIN_WATER)
                    continue;
                position_x = offset_x * 8;
                position_x += player->x;
                position_x -= 16;
                position_y = offset_y * 8;
                position_y += player->y;
                position_y -= 16;
                if (!pass) {
                    for (scratch = 0; scratch < 32; ++scratch)
                        if (gDroppedPresentTable[scratch].kind != PRESENT_NONE &&
                            gDroppedPresentTable[scratch].level == player->level &&
                            gDroppedPresentTable[scratch].x == position_x &&
                            gDroppedPresentTable[scratch].y == position_y)
                            scratch = 101;
                    if (scratch > 100)
                        continue;
                }
                destination->kind = present;
                destination->level = player->level;
                destination->x = position_x;
                destination->y = position_y;
                gDroppedPresentTableIdx = (gDroppedPresentTableIdx + 1) & 31;
                return;
            }
}
