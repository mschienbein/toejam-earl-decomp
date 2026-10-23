#include "earthling_types.h"
#include "terrain.h"
#include "hardware.h"
#define TJE_LOCAL_GETMAPTERRAINATWORLDPOSITION
#define TJE_LOCAL_EARTHLINGCONFIGUREMAILBOXLEVELBANK
#define TJE_LOCAL_GETRANDOMNUMBER

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_earthlings.h"
#include "assets_sprites.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_earthlings.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ship.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "ship.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: (int)AllocateEntity() preserves the observed
 * call-result width; (unsigned short)GetRandomNumber() narrows this file's
 * int RNG view for word draws. Historical spelling remains unknown [INFERENCE]. */

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared unsigned char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetMapTerrainAtWorldPosition();

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int EarthlingConfigureMailboxLevelBank();

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared unsigned short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetRandomNumber();

/* Reject elevator overlap and any occupied earthling inside the combined radii.
 * MATCHING QUALIFICATION: retain the explicit ? 1 : 0 normalization before
 * testing the overlap predicate; no separate per-site removal control is claimed. */
int CheckEarthlingOverlap(definition, bank, x, y, level)
struct earthling_definition *definition;
short bank, x, y, level;
{
    short record_number, clearance;
    struct earthling_state *earthling;
    struct earthling_definition *other_definition;

    if (!ElevatorAllowsWorldPlacement(x, y, level))
        return 0;
    for (record_number = 0; record_number < EARTHLINGS_PER_BANK; ++record_number) {
        earthling = &gEarthlingBanks[bank][record_number];
        if (earthling->kind != EARTHLING_UNUSED) {
            other_definition = &EarthlingDefinitions[earthling->kind];
            clearance = (short)other_definition->radius << 2;
            clearance += (short)definition->radius << 2;
            if (clearance > abs(x - earthling->x) &&
                clearance > abs(y - earthling->y) ? 1 : 0)
                return 0;
        }
    }
    return 1;
}

/* Test the first two records in the bank selected for this level. */
int EarthlingIsSpecialRecordNearby(x, y, level)
short x, y, level;
{
    short bank, record_number, distance_x, distance_y;
    struct earthling_state *earthling;

    if (gEarthlingBankUsers[0] && gEarthlingBankLevels[0] == level)
        bank = 0;
    else
        bank = 1;
    earthling = gEarthlingBanks[bank];
    for (record_number = 0; record_number < 2; ++record_number, ++earthling) {
        if (!earthling->kind || EarthlingIsMailOrderMailbox(earthling)) {
            distance_x = abs(x - earthling->x);
            distance_y = abs(y - earthling->y);
            if (distance_x < 340 && distance_y < 200)
                return 1;
        }
    }
    return 0;
}

/* Try ten random metatiles, then search every tile cyclically for a clear spawn. */
int FindSpawnLocationForEarthling(kind, bank, output_x, output_y, level)
short kind, bank;
short *output_x, *output_y;
short level;
{
    unsigned short metatile;
    short start_column, width, height;
    unsigned short *map;
    short position_x, position_y;
    short column, row, search_counter;
    register unsigned short random;
    unsigned char terrain_mask;
    struct earthling_definition *definition;

    if (gPlayer1MapLevel == level)
        map = gPlayer1LevelMap[0];
    else
        map = gPlayer2LevelMap[0];
    definition = &EarthlingDefinitions[kind];
    terrain_mask = 3;
    if (!kind || kind == EARTHLING_NERD_HERD || kind == EARTHLING_ICE_CREAM_TRUCK || kind == EARTHLING_LAWNMOWER || kind == EARTHLING_MOLE) {
        width = 16;
        height = 8;
    } else if (kind == EARTHLING_WHIRLWIND) {
        terrain_mask = 2;
        width = 2;
        height = 2;
    } else if (kind == EARTHLING_SHARK) {
        terrain_mask = 8;
        width = 3;
        height = 2;
    } else {
        width = 8;
        height = 2;
    }
    for (search_counter = 0; search_counter < 10; ++search_counter) {
        random = (unsigned short)GetRandomNumber();
        row = random % 26;
        random = (unsigned short)GetRandomNumber();
        column = random % 19;
        metatile = map[row * 19 + column];
        if (TerrainValidation(metatile, width, height, &position_x, &position_y, terrain_mask)) {
            position_y += (row + 1) * 64;
            position_x += (column + 1) * 128;
            if (CheckEarthlingOverlap(definition, bank, position_x, position_y, level)) {
                *output_x = position_x;
                *output_y = position_y;
                return 0;
            }
        }
    }
    random = (unsigned short)GetRandomNumber();
    search_counter = random % 26;
    random = (unsigned short)GetRandomNumber();
    start_column = random % 19;
    row = search_counter;
    do {
        column = start_column;
        do {
            metatile = map[row * 19 + column];
            if (TerrainValidation(metatile, width, height, &position_x, &position_y, terrain_mask)) {
                position_y += (row + 1) * 64;
                position_x += (column + 1) * 128;
                if (CheckEarthlingOverlap(definition, bank, position_x, position_y, level)) {
                    *output_x = position_x;
                    *output_y = position_y;
                    return 0;
                }
            }
            column = (column + 1) % 19;
        } while (column != start_column);
        row = (row + 1) % 26;
    } while (row != search_counter);
    return 1;
}

/* Convert the stored byte coordinates to the ship-piece world position. */
void MapGetShipPieceWorldPosition(output_x, output_y)
short *output_x, *output_y;
{
    short map_x, map_y;

    map_x = gShipPieceSpawnMapX;
    map_y = gShipPieceSpawnMapY;
    *output_x = (map_x + 2) * 16 * 8;
    *output_y = (map_y + 2) * 8 * 8 + 8;
}

/* Retain an already loaded bank, or populate an unused bank from level choices. */
void EarthlingRetainLevelBank(level)
short level;
{
    char ship_piece = 0;
    short bank, record_number, position_x, position_y;
    char special, kind;
    long saved_random;

    if (level >= 26)
        return;
    for (bank = 0; bank < EARTHLING_BANK_COUNT; ++bank) {
        if (gEarthlingBankUsers[bank] && gEarthlingBankLevels[bank] == level) {
            ++gEarthlingBankUsers[bank];
            return;
        }
    }
    bank = 0;
    if (gEarthlingBankUsers[0])
        bank = 1;
    saved_random = StackBasedRandom(gLevelSeeds[level]);
    if (level) {
        for (record_number = 0; record_number < 10; ++record_number) {
            if (gShipPieceLevelAssignments[record_number] == level)
                ship_piece = 1;
        }
    }
    special = EarthlingConfigureMailboxLevelBank(bank, level);
    for (record_number = 0; record_number < EARTHLINGS_PER_BANK; ++record_number) {
        if (level == 1) {
            if (saved_random) {
                StackBasedRandom(saved_random);
                saved_random = 0;
            }
            if (!record_number)
                break;
            kind = 18;
        } else if (special) {
            kind = 9;
            special = 0;
        } else if (ship_piece) {
            kind = 0;
            MapGetShipPieceWorldPosition(&position_x, &position_y);
            ship_piece = 0;
            if (gHighestLevel == level)
                gShipScreenState.pending_piece_notice_level = level;
        } else {
            if (saved_random) {
                StackBasedRandom(saved_random);
                saved_random = 0;
            }
            kind = LevelEarthlingChoices[level][record_number];
            if (kind == EARTHLING_SANTA && gHighestLevel > level)
                kind = 1;
        }
        if (kind > 0) {
            if (!level)
                GetLevelZeroEarthlingPosition(kind, &position_x, &position_y);
            else if (FindSpawnLocationForEarthling(kind, bank, &position_x, &position_y, level))
                gEarthlingSpawnFailureMasks[level] |= (unsigned long)0x80000000 >> record_number;
        }
        if (!kind || (kind >= 0 && !(gEarthlingSpawnFailureMasks[level] & ((unsigned long)0x80000000 >> record_number)))) {
            gEarthlingBanks[bank][record_number].kind = kind;
            gEarthlingBanks[bank][record_number].level = level;
            gEarthlingBanks[bank][record_number].x = position_x;
            gEarthlingBanks[bank][record_number].y = position_y;
            gEarthlingBanks[bank][record_number].saved_y = position_y;
            if (kind == EARTHLING_ICE_CREAM_TRUCK || kind == EARTHLING_MAILBOX) {
                gEarthlingBanks[bank][record_number].saved_x_or_status = 0;
                gEarthlingBanks[bank][record_number].saved_y = 0;
            } else if (kind == EARTHLING_CUPID)
                gEarthlingBanks[bank][record_number].saved_x_or_status = 1;
            else
                gEarthlingBanks[bank][record_number].saved_x_or_status = position_x;
            gEarthlingBanks[bank][record_number].health_visibility[0] = EarthlingDefinitions[kind].health;
            gEarthlingBanks[bank][record_number].health_visibility[1] = 0;
        } else
            gEarthlingBanks[bank][record_number].kind = EARTHLING_UNUSED;
    }
    ++gEarthlingBankUsers[bank];
    gEarthlingBankLevels[bank] = level;
}

/* Mark both banks empty and request level one for every player with lives. */
void SetupInitialEarthlings()
{
    short record_number;

    for (record_number = 0; record_number < EARTHLINGS_PER_BANK; ++record_number) {
        gEarthlingBanks[0][record_number].kind = EARTHLING_UNUSED;
        gEarthlingBank2Records[record_number].kind = EARTHLING_UNUSED;
    }
    if (gPlayerLives[PLAYER_TOEJAM] >= 0)
        EarthlingRetainLevelBank(1);
    if (gPlayer2Lives >= 0)
        EarthlingRetainLevelBank(1);
    AssignShipPieceLevels();
}

/* Install the state animation, scan its frame controls, and release a secondary
 * object when leaving state 2. */
void SetEarthlingState(earthling, state)
struct earthling_state *earthling;
short state;
{
    struct timed_animation *animation;
    struct object_state *object;
    struct earthling_definition *data;
    short sequence_index;

    earthling->state = state;
    if (!earthling->object)
        return;
    data = &EarthlingDefinitions[earthling->kind];
    animation = &data->states[state];
    object = &gPlayerInfo[earthling->object];
    object->sequence = animation->sequence;
    object->frames = animation->frames;
    object->frame_phase = animation->period;
    object->frame_duration = 1;
    object->sprite_state = state;
    if (object->frame >= 0)
        --object->frame;
    for (sequence_index = 0; sequence_index <= object->frame; ++sequence_index)
        if ((object->sequence[sequence_index] & 0xf0) == 0xf0)
            object->frame = -1;
    if (state != 2 && earthling->second_object) {
        FreeEntity(earthling->second_object);
        earthling->second_object = 0;
    }
}

/* Release both displayed objects, then choose a spawn point outside elevators. */
void EarthlingRelocateRecord(earthling)
struct earthling_state *earthling;
{
    char other_level = 0;

    if (earthling->object) {
        FreeEntity(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        FreeEntity(earthling->second_object);
        earthling->second_object = 0;
    }
    if (!gEarthlingBankUsers[0] || gEarthlingBankLevels[0] != earthling->level)
        other_level = 1;
    do {
        FindSpawnLocationForEarthling(earthling->kind, other_level,
                                     &earthling->x, &earthling->y,
                                     earthling->level);
    } while (ElevatorContainsPosition(earthling->x, earthling->y, earthling->level));
}

/* Release displayed objects and restore the two saved coordinates. */
void EarthlingRestoreSavedPosition(earthling)
struct earthling_state *earthling;
{
    if (earthling->object) {
        FreeEntity(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        FreeEntity(earthling->second_object);
        earthling->second_object = 0;
    }
    earthling->x = earthling->saved_x_or_status;
    earthling->y = earthling->saved_y;
}

/* Allocate display objects and install the initial animation and type fields. */
void EarthlingAllocateEntities(earthling)
struct earthling_state *earthling;
{
    struct earthling_definition *data;
    struct object_state *object;
    struct timed_animation *animation;

    data = &EarthlingDefinitions[earthling->kind];
    if (earthling->object = (int)AllocateEntity()) {
        object = &gPlayerInfo[earthling->object];
        object->earthling = earthling;
        object->process = data->update;
        object->animation_motion_axes = data->movement_mode;
        object->x = earthling->x;
        object->y = earthling->y;
        if (earthling->kind == EARTHLING_CUPID)
            object->height = 40;
        object->level = earthling->level;
        object->width = data->width;
        object->height_pixels = data->height_pixels;
        object->sprite_count = data->sprite_count;
        object->tile_count = data->tile_count;
        if (EarthlingIsMailOrderMailbox(earthling)) {
            object->sprite_count = 1;
            object->tile_count = 12;
            object->animation_motion_axes = ANIMATION_MOTION_NONE;
            object->draw_flags |= ENTITY_SKIP_PHYSICS;
        }
        object->tile = VDP_TILE_PRIORITY;
        if (earthling->kind != EARTHLING_SHARK)
            object->behavior_flags |= ENTITY_COLLIDABLE;
        if ((data->behavior_flags & 1) &&
            (gCachedRandomValues[gRandomCacheIndex == 58 ?
                           (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1) &&
            (earthling->second_object = (int)AllocateEntity())) {
            SetEarthlingState(earthling, 2);
            object = &gPlayerInfo[earthling->second_object];
            object->x = earthling->x;
            object->y = earthling->y + 1;
            object->x += EarthlingCoordinatePairs[earthling->kind * 2];
            object->height = EarthlingCoordinatePairs[earthling->kind * 2 + 1];
            object->level = earthling->level;
            object->width = 32;
            object->height_pixels = 32;
            object->sprite_count = 1;
            object->tile_count = 9;
            object->tile = VDP_TILE_PRIORITY;
            animation = EarthlingCompanionAnimations;
            object->sequence = animation->sequence;
            object->frames = animation->frames;
            object->frame_phase = animation->period;
            object->frame_duration = 1;
        } else if (earthling->kind == EARTHLING_WHIRLWIND) {
            object->behavior_flags = ENTITY_HIDDEN;
            SetEarthlingState(earthling, 3);
        } else
            SetEarthlingState(earthling, 0);
        earthling->timer = 0;
    } else if (!earthling->kind)
        EntityReleaseOneByPriority();
    else
        EarthlingRelocateRecord(earthling);
}

/* Refresh alternating earthlings and load objects visible to either camera. */
void LoadInEarthlings(earthlings)
struct earthling_state *earthlings;
{
    short index, toejam_x, toejam_y, earl_x, earl_y;
    struct earthling_state *earthling;
    struct object_state *object;

    for (index = gEarthlingUpdateParity; index < EARTHLINGS_PER_BANK; index += 2) {
        earthling = &earthlings[index];
        if (earthling->object) {
            object = &gPlayerInfo[earthling->object];
            earthling->x = object->x;
            earthling->y = object->y;
        }
        toejam_x = earthling->x - gCamera1PendingX;
        toejam_y = earthling->y - gCamera1PendingY;
        earl_x = earthling->x - gCamera2PendingX;
        earl_y = earthling->y - gCamera2PendingY;
        if (earthling->kind != EARTHLING_UNUSED) {
            if (earthling->level == gPlayer1MapLevel &&
                toejam_x + 160 > 0 && toejam_x < 480 &&
                toejam_y + 112 > 0 && toejam_y < 336)
                earthling->health_visibility[1] |= 2;
            else
                earthling->health_visibility[1] &= ~2;
            if (earthling->level == gPlayer2MapLevel && gSplitScreenEnabled &&
                earl_x + 160 > 0 && earl_x < 480 &&
                earl_y + 112 > 0 && earl_y < 336)
                earthling->health_visibility[1] |= 4;
            else
                earthling->health_visibility[1] &= ~4;
        } else
            earthling->health_visibility[1] = 0;
        if (earthling->health_visibility[1] & 6) {
            if (!earthling->object)
                EarthlingAllocateEntities(earthling);
        } else {
            if (earthling->object) {
                FreeEntity(earthling->object);
                earthling->object = 0;
            }
            if (earthling->second_object) {
                FreeEntity(earthling->second_object);
                earthling->second_object = 0;
            }
        }
    }
}

/* Choose a nearby position and an available or inactive earthling record. */
void OpenEarthlingPresent(player, terrain)
struct object_state *player;
short terrain;
{
    short x, y, bank = 0, player_number = player->entity_index;
    short index, selection, height, object_number, terrain_player;
    unsigned short random;
    struct earthling_state *earthling;

    for (index = 0; index < 4; ++index) {
        x = player->x;
        if ((unsigned short)GetRandomNumber() & 1)
            x += 40;
        else
            x -= 40;
        y = player->y;
        if ((unsigned short)GetRandomNumber() & 1)
            y += 25;
        else
            y -= 25;
        if (player->draw_flags & ENTITY_PRIMARY_VIEW)
            terrain_player = 0;
        else
            terrain_player = 1;
        selection = GetMapTerrainAtWorldPosition(terrain_player, x, y);
        if ((terrain == TERRAIN_WATER && selection == TERRAIN_WATER) ||
            (terrain != TERRAIN_WATER && selection != TERRAIN_WATER))
            break;
    }
    terrain = selection;
    if (!gEarthlingBankUsers[bank] || gEarthlingBankLevels[bank] != player->level)
        ++bank;
    earthling = gEarthlingBanks[bank];
    for (index = 0; index < EARTHLINGS_PER_BANK; ++earthling, ++index)
        if (earthling->kind == EARTHLING_UNUSED)
            break;
    /* ORIGINAL BUG: an exhausted scan leaves earthling one past this bank;
     * the original tests that record before looking for an inactive slot. */
    if (earthling->kind != EARTHLING_UNUSED) {
        earthling = gEarthlingBanks[bank];
        for (index = 0; index < EARTHLINGS_PER_BANK; ++earthling, ++index)
            if (earthling->kind != EARTHLING_SHIP_PIECE && earthling->kind != EARTHLING_MAILBOX &&
                !earthling->object)
                break;
    }
    /* ORIGINAL BUG: exhaustion of the second scan is not checked either. */
    if (!player->level)
        earthling->kind = EARTHLING_HULA_DANCER;
    else if (terrain == TERRAIN_WATER)
        earthling->kind = EARTHLING_SHARK;
    else if (terrain == TERRAIN_VOID || terrain == TERRAIN_WALKABLE_MAX)
        earthling->kind = EARTHLING_BEES_VARIANT;
    else {
        selection = 0;
        while (!selection) {
            earthling->kind = (random = GetRandomNumber()) % 22 + 1;
            if (earthling->kind == EARTHLING_OPERA_SINGER || earthling->kind == EARTHLING_SANTA ||
                earthling->kind == EARTHLING_WISEMAN || earthling->kind == EARTHLING_WIZARD ||
                earthling->kind == EARTHLING_MAILBOX) {
                for (index = 0; index < EARTHLINGS_PER_BANK; ++index) {
                    if (earthling != &gEarthlingBanks[bank][index] &&
                        gEarthlingBanks[bank][index].kind == earthling->kind) {
                        if (!gEarthlingBanks[bank][index].object) {
                            earthling = &gEarthlingBanks[bank][index];
                            selection = 1;
                        }
                        break;
                    }
                }
                if (index == EARTHLINGS_PER_BANK) {
                    if (earthling->kind != EARTHLING_MAILBOX || EarthlingConfigureMailboxLevelBank(bank, player->level))
                        selection = 1;
                }
            } else if (earthling->kind != EARTHLING_SHARK &&
                       (earthling->kind != EARTHLING_WHIRLWIND || terrain == TERRAIN_SINKING) &&
                       (earthling->kind != EARTHLING_LAWNMOWER || terrain == TERRAIN_GROUND))
                selection = 1;
        }
    }
    earthling->level = player->level;
    earthling->x = x;
    earthling->y = y;
    earthling->saved_y = y;
    height = 5;
    if (earthling->kind == EARTHLING_MOLE || earthling->kind == EARTHLING_ICE_CREAM_TRUCK) {
        earthling->saved_x_or_status = 0;
        earthling->saved_y = 0;
    } else if (earthling->kind == EARTHLING_CUPID) {
        earthling->saved_x_or_status = 1;
        height = 30;
    } else
        earthling->saved_x_or_status = x;
    earthling->health_visibility[0] = EarthlingDefinitions[earthling->kind].health;
    earthling->health_visibility[1] = 0;
    PresentStartTransientEffect(player_number, earthling->x, earthling->y + 1,
                height, earthling->level);
    if (object_number = (int)AllocateEntity())
        FreeEntity(object_number);
    else if (object_number = AllocateEntityWithEviction(1))
        FreeEntity(object_number);
}

/* Invoke eligible loaded earthlings' per-type abilities on alternating slots. */
void EarthlingDispatchAbilities()
{
    short bank, index;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    for (bank = 0; bank < EARTHLING_BANK_COUNT; ++bank) {
        if (gEarthlingBankUsers[bank]) {
            for (index = gEarthlingUpdateParity; index < EARTHLINGS_PER_BANK; index += 2) {
                earthling = &gEarthlingBanks[bank][index];
                if (earthling->kind != EARTHLING_UNUSED && earthling->object &&
                    earthling->state != EARTHLING_STATE_SLEEP && !EarthlingIsMailOrderMailbox(earthling)) {
                    data = &EarthlingDefinitions[earthling->kind];
                    if (data->ability && !EarthlingIsUpdateBlocked(earthling))
                        (*data->ability)(earthling);
                }
            }
        }
    }
}

/* Visit alternating occupied earthling slots and test both eligible players
 * against the type's distance limits before calling its touch handler. */
void EarthlingDispatchPlayerContacts()
{
    short buffer, slot, player_number, trial;
    struct earthling_state *earthling;
    struct earthling_definition *data;
    struct object_state *object, *player;

    for (buffer = 0; buffer < EARTHLING_BANK_COUNT; ++buffer) {
        if (gEarthlingBankUsers[buffer]) {
            for (slot = gEarthlingUpdateParity; slot < EARTHLINGS_PER_BANK; slot += 2) {
                earthling = &gEarthlingBanks[buffer][slot];
                if (earthling->kind == EARTHLING_UNUSED || !earthling->object ||
                    earthling->state == EARTHLING_STATE_SLEEP)
                    continue;
                data = &EarthlingDefinitions[earthling->kind];
                object = &gPlayerInfo[earthling->object];
                player_number = gCachedRandomValues[gRandomCacheIndex == 58 ?
                                (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1;
                for (trial = 0; trial < 2; ++trial) {
                    player = &gPlayerInfo[player_number];
                    if (!gPlayerInventoryOpen[player_number] &&
                        !gPlayerElevatorFlags[player_number] &&
                        !(player->behavior_flags & ENTITY_PLAYER_SUPPRESSED) &&
                        EntityIsWithinRange(object, player, data->contact_distance_x,
                                    data->contact_distance_y)) {
                        if ((data->behavior_flags & 1) && earthling->state == 2) {
                            SetEarthlingState(earthling, 0);
                            earthling->timer = 0;
                        }
                        (*data->touch)(player_number, earthling);
                    }
                    player_number ^= 1;
                }
                if (earthling->kind == EARTHLING_CUPID)
                    EarthlingCheckDecoyAttackRange(earthling, data);
            }
        }
    }
}

/* Nearby active objects in gPlayerBoomBoxEntities put eligible earthlings into state 99,
 * install their sleep animation, and release any secondary object. */
void EarthlingApplyNearbySleep()
{
    short source, buffer, slot;
    struct earthling_state *earthling;
    struct object_state *source_object, *object;

    for (source = 0; source <= 1; ++source) {
        if (gPlayerBoomBoxEntities[source]) {
            source_object = &gPlayerInfo[gPlayerBoomBoxEntities[source]];
            if (gEarthlingBankUsers[0] &&
                gEarthlingBankLevels[0] == source_object->level)
                buffer = 0;
            else
                buffer = 1;
            for (slot = gEarthlingUpdateParity; slot < EARTHLINGS_PER_BANK; slot += 2) {
                earthling = &gEarthlingBanks[buffer][slot];
                if (earthling->kind == EARTHLING_UNUSED || earthling->state == EARTHLING_STATE_SLEEP ||
                    !earthling->object || earthling->kind == EARTHLING_SHIP_PIECE ||
                    earthling->kind == EARTHLING_WHIRLWIND || earthling->kind == EARTHLING_SHARK ||
                    earthling->kind == EARTHLING_SANTA || earthling->kind == EARTHLING_LEMONADE_STAND ||
                    earthling->kind == EARTHLING_HOT_TUB || EarthlingIsMailOrderMailbox(earthling))
                    continue;
                if (earthling->kind == EARTHLING_CUPID) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->kind == EARTHLING_MOLE) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->kind == EARTHLING_WISEMAN) {
                    if (earthling->state < 2)
                        continue;
                } else if (earthling->kind == EARTHLING_OPERA_SINGER || earthling->kind == EARTHLING_WIZARD) {
                    if ((earthling->state & 0xfc) != 4)
                        continue;
                }
                if (abs(earthling->x - source_object->x) >= 320 ||
                    abs(earthling->y - source_object->y) >= 224)
                    continue;
                object = &gPlayerInfo[earthling->object];
                object->velocity_x = 0;
                object->velocity_y = 0;
                object->sequence = SleepAnimationDescriptors[earthling->kind].sequence;
                object->frames = SleepAnimationDescriptors[earthling->kind].frames;
                object->frame_phase = 6;
                object->frame_duration = 1;
                object->frame = -1;
                object->sprite_state = EARTHLING_STATE_SLEEP;
                earthling->state = EARTHLING_STATE_SLEEP;
                if (earthling->second_object) {
                    FreeEntity(earthling->second_object);
                    earthling->second_object = 0;
                }
            }
        }
    }
}

/* Wake each occupied state-99 record in the selected loaded level. */
void EarthlingWakeLevel(level)
short level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (gEarthlingBankUsers[0] && gEarthlingBankLevels[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = gEarthlingBanks[buffer];
    for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot, ++earthling) {
        if (earthling->kind != EARTHLING_UNUSED && earthling->state == EARTHLING_STATE_SLEEP)
            SetEarthlingState(earthling, 0);
    }
}

/* Reset state-2 earthlings whose type flag and object draw mask permit it. */
int EarthlingResetStateTwoByDrawMask(draw_mask)
char draw_mask;
{
    short buffer, slot;
    int changed;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    changed = 0;
    for (buffer = 0; buffer < EARTHLING_BANK_COUNT; ++buffer) {
        if (gEarthlingBankUsers[buffer]) {
            for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot) {
                earthling = &gEarthlingBanks[buffer][slot];
                if (earthling->kind != EARTHLING_UNUSED && earthling->object &&
                    earthling->state == 2) {
                    data = &EarthlingDefinitions[earthling->kind];
                    if ((data->behavior_flags & 1) &&
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

/* Release both allocated objects, mark the record empty, and record its slot
 * in the level's high-bit-first removed-earthling mask. */
void EarthlingRemoveFromLevel(earthling)
struct earthling_state *earthling;
{
    short buffer, slot;

    if (earthling->object) {
        FreeEntity(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        FreeEntity(earthling->second_object);
        earthling->second_object = 0;
    }
    earthling->kind = EARTHLING_UNUSED;
    for (buffer = 0; buffer < EARTHLING_BANK_COUNT; ++buffer) {
        if (gEarthlingBankUsers[buffer] &&
            gEarthlingBankLevels[buffer] == earthling->level) {
            for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot) {
                if (earthling == &gEarthlingBanks[buffer][slot]) {
                    short level;
                    level = gEarthlingBankLevels[buffer];
                    gEarthlingSpawnFailureMasks[level] |= (unsigned long)0x80000000 >> slot;
                }
            }
        }
    }
}

/* Decrement the hit-point byte; nice-earthling handling can preserve one hit.
 * Removal also creates the effect at the record's saved position. */
void DamageEarthling(earthling)
struct earthling_state *earthling;
{
    if (earthling && earthling->kind > 0) {
        if (!EarthlingIsMailOrderMailbox(earthling) && earthling->kind != EARTHLING_SANTA &&
            earthling->kind < EARTHLING_LEMONADE_STAND && --earthling->health_visibility[0] <= 0) {
            if (earthling->kind == EARTHLING_OPERA_SINGER || earthling->kind == EARTHLING_WIZARD ||
                earthling->kind == EARTHLING_WISEMAN) {
                if (NiceEarthlingCheck(earthling)) {
                    earthling->health_visibility[0] = 1;
                    return;
                }
            }
            EarthlingRemoveFromLevel(earthling);
            EntitySpawnTimedEffect(earthling->x, earthling->y, earthling->level);
        } else if (earthling->second_object) {
            SetEarthlingState(earthling, 0);
            earthling->timer = 0;
        }
    }
}

/* Remove eligible earthlings whose saved coordinates now lie on terrain 0x90. */
void EarthlingDamageOnHazardTerrain(player_number, level)
short player_number, level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (gEarthlingBankUsers[0] && gEarthlingBankLevels[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = gEarthlingBanks[buffer];
    for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot, ++earthling) {
        if (earthling->kind > 0 && earthling->object &&
            earthling->kind != EARTHLING_BEES && earthling->kind != EARTHLING_BEES_VARIANT &&
            earthling->kind != EARTHLING_CUPID && earthling->kind != EARTHLING_SANTA &&
            earthling->kind != EARTHLING_WHIRLWIND && earthling->kind != EARTHLING_ICE_CREAM_TRUCK &&
            GetMapTerrainAtWorldPosition(player_number, earthling->x, earthling->y) == 0x90) {
            earthling->health_visibility[0] = 0;
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

    for (buffer = 0; buffer < EARTHLING_BANK_COUNT; ++buffer) {
        if (gEarthlingBankUsers[buffer]) {
            for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot) {
                earthling = &gEarthlingBanks[buffer][slot];
                if (earthling->kind != EARTHLING_UNUSED && earthling->kind != EARTHLING_OPERA_SINGER &&
                    earthling->object &&
                    (gPlayerInfo[earthling->object].draw_flags & draw_mask) &&
                    (earthling->health_visibility[0] > 1 || EntityHasFreeTimedEffectSlot()))
                    DamageEarthling(earthling);
            }
        }
    }
}

/* Clear the player's per-mole contact flag for every allocated mole object. */
void EarthlingClearMolePlayerContactFlags(player)
struct object_state *player;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (gEarthlingBankUsers[0] && gEarthlingBankLevels[0] == player->level)
        buffer = 0;
    else
        buffer = 1;
    earthling = gEarthlingBanks[buffer];
    for (slot = 0; slot < EARTHLINGS_PER_BANK; ++slot, ++earthling) {
        if (earthling->kind == EARTHLING_MOLE && earthling->object) {
            if (!player->entity_index)
                gPlayerInfo[earthling->object].attachment_x_or_latch = 0;
            else
                gPlayerInfo[earthling->object].attachment_y_or_latch = 0;
        }
    }
}

/* Advance loaded earthlings unless both the menu and HBlank flags are set. */
void ProcessEarthlings()
{
    if (gPlayerInventoryOpen[PLAYER_TOEJAM] && gPlayer2InventoryOpen)
        return;
    if (gEarthlingBankUsers[0])
        LoadInEarthlings(gEarthlingBanks[0]);
    if (gEarthlingBank2Users)
        LoadInEarthlings(gEarthlingBank2Records);
    EarthlingDispatchAbilities();
    EarthlingDispatchPlayerContacts();
    EarthlingApplyNearbySleep();
    ShipUpdatePendingPieceIndicator();
    ReleaseFinishedMoleEffectsAndStolenPresents();
    gEarthlingUpdateParity ^= 1;
}
