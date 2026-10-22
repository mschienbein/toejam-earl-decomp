#define TJE_LOCAL_TILEALTERNATIVEPOINTERS

#include "map.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_map.h"
#include "random.h"

/* Compatible incomplete-array compiler view. The actual complete shared
 * declaration changes the observed instructions; historical source spelling
 * is unknown [INFERENCE]. */
extern unsigned char *TileAlternativePointers[];

/* Refine marked map cells using the low-byte shapes of adjacent cells. */
void MapRefineFilledShapeMasks()
{
    short row, column;
    unsigned short shape;
    char upper_right, upper_left, lower_right, lower_left;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if (shape & 0x800) {
                shape &= 0xff;
                if (shape == 3) {
                    shape = gPlayer1LevelMap[row][column - 1] & 0xff;
                    if ((gPlayer1LevelMap[row][column + 1] & 0xff) != 7 &&
                        (shape == 11 || shape == 19))
                        gPlayer1LevelMap[row][column] |= 0x10;
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if (shape & 0x800) {
                shape &= 0xff;
                if (shape == 12) {
                    shape = gPlayer1LevelMap[row][column - 1] & 0xff;
                    if (shape == 4 || shape == 28)
                        gPlayer1LevelMap[row][column] |= 0x10;
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if ((shape & 0x800) && (shape & 15) == 15) {
                upper_right = upper_left = 0;
                lower_right = lower_left = 0;
                shape = gPlayer1LevelMap[row - 1][column] & 31;
                if (shape == 21 || shape == 13 || shape == 12)
                    ++upper_left;
                if (shape == 14 || shape == 28 || shape == 26)
                    ++upper_right;
                shape = gPlayer1LevelMap[row + 1][column] & 31;
                if (shape == 7 || shape == 3)
                    ++lower_left;
                if (shape == 19 || shape == 11)
                    ++lower_right;
                if (upper_right) {
                    if (lower_left)
                        shape = 23;
                    else if (lower_right)
                        shape = 26;
                    else
                        shape = 20;
                } else if (upper_left) {
                    if (lower_right)
                        shape = 24;
                    else if (lower_left)
                        shape = 21;
                    else
                        shape = 20;
                } else
                    shape = 20;
                gPlayer1LevelMap[row][column] &= 0xff00;
                gPlayer1LevelMap[row][column] |= shape;
            }
        }
}

/* Mark the cells with class 3, compute their filled corner bits, and select
 * a low-byte shape from the two lookup banks. */
void MapBuildFilledTerrainShapes()
{
    short row, column;
    unsigned short shape;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if ((shape & 0x3000) == 0x3000)
                gPlayer1LevelMap[row][column] |= 0x800;
            else
                gPlayer1LevelMap[row][column] &= 0xf7ff;
            if ((shape & 0x3000) != 0x2000)
                gMapPathDirectionBanks[row][column] &= 0xf0;
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if (shape & 0x800) {
                shape = 0;
                if (row > 0 && column > 0 &&
                    (gPlayer1LevelMap[row][column - 1] & 0x800) &&
                    (gPlayer1LevelMap[row - 1][column - 1] & 0x800) &&
                    (gPlayer1LevelMap[row - 1][column] & 0x800))
                    shape |= 1;
                if (row > 0 && column < 18 &&
                    (gPlayer1LevelMap[row - 1][column] & 0x800) &&
                    (gPlayer1LevelMap[row - 1][column + 1] & 0x800) &&
                    (gPlayer1LevelMap[row][column + 1] & 0x800))
                    shape |= 2;
                if (row < 25 && column < 18 &&
                    (gPlayer1LevelMap[row][column + 1] & 0x800) &&
                    (gPlayer1LevelMap[row + 1][column + 1] & 0x800) &&
                    (gPlayer1LevelMap[row + 1][column] & 0x800))
                    shape |= 4;
                if (row < 25 && column > 0 &&
                    (gPlayer1LevelMap[row + 1][column] & 0x800) &&
                    (gPlayer1LevelMap[row + 1][column - 1] & 0x800) &&
                    (gPlayer1LevelMap[row][column - 1] & 0x800))
                    shape |= 8;
                gPlayer1LevelMap[row][column] |= shape;
            }
        }
    MapRefineFilledShapeMasks();
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if (shape & 0x800) {
                shape &= 31;
                gPlayer1LevelMap[row][column] &= 0xff00;
                if (gPlayer1LevelMap[row][column] & 0x8000)
                    gPlayer1LevelMap[row][column] |=
                        (unsigned short)MapGenerationLookupTables.filled_alternate[shape];
                else
                    gPlayer1LevelMap[row][column] |=
                        (unsigned short)MapGenerationLookupTables.filled_plain[shape];
            }
        }
}

/* Convert class-1 boundary cells next to class-2 cells, then fold the
 * per-cell corner mask into the class-2 shape lookup. */
void MapBuildDirectionalPathShapes()
{
    short row, column, shape;
    short replacement;
    unsigned char corners;

    for (row = 1; row < 25; ++row)
        for (column = 1; column < 18; ++column) {
            shape = gPlayer1LevelMap[row][column];
            if ((shape & 0x3000) == 0x1000) {
                replacement = 0;
                if ((gPlayer1LevelMap[row - 1][column] & 0x3004) == 0x2004) {
                    if (gMapPathDirectionBanks[row - 1][column] & 4)
                        gPlayer1LevelMap[row][column] |= 0x4000;
                    else
                        replacement = 16;
                } else if ((gPlayer1LevelMap[row][column + 1] & 0x3008) == 0x2008) {
                    if (gMapPathDirectionBanks[row][column + 1] & 8)
                        gPlayer1LevelMap[row][column] |= 0x4000;
                    else
                        replacement = 17;
                } else if ((gPlayer1LevelMap[row + 1][column] & 0x3001) == 0x2001) {
                    if (gMapPathDirectionBanks[row + 1][column] & 1)
                        gPlayer1LevelMap[row][column] |= 0x4000;
                    else
                        replacement = 18;
                } else if ((gPlayer1LevelMap[row][column - 1] & 0x3002) == 0x2002) {
                    if (gMapPathDirectionBanks[row][column - 1] & 2)
                        gPlayer1LevelMap[row][column] |= 0x4000;
                    else
                        replacement = 19;
                }
                if (replacement) {
                    if ((gPlayer1LevelMap[row][column] &= 0xff00) & 0x8000)
                        gPlayer1LevelMap[row][column] |=
                            (unsigned short)MapBoundaryShapesAlternate[replacement];
                    else
                        gPlayer1LevelMap[row][column] |=
                            (unsigned short)MapBoundaryShapesPlain[replacement];
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gPlayer1LevelMap[row][column];
            corners = gMapPathDirectionBanks[row][column] & 15;
            if (gMapDirectionalPathMode)
                corners = 0;
            if ((shape & 0x3000) == 0x2000) {
                shape ^= (unsigned short)corners;
                gPlayer1LevelMap[row][column] =
                    MapGenerationLookupTables.tiles[shape & 15] | 0x2000;
                if (corners)
                    gPlayer1LevelMap[row][column] |= 0x4000;
            }
        }
}

/* Populate unmasked cells with individual shapes or a four-cell block.
 * The surrounding class-1 cells and the random thresholds select the bank. */
void MapScatterTerrainFeatures()
{
    short row, column, scan_row, scan_column;
    short nearby_plain, nearby_alternate, nearby_feature, alternate_chance;
    unsigned short shape;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            if (!(gMapPathDirectionBanks[row][column] & 15)) {
                shape = gPlayer1LevelMap[row][column] & 0xff;
                if (!shape) {
                    nearby_plain = 0;
                    nearby_alternate = 0;
                    nearby_feature = 0;
                    if (row > 0 &&
                        (shape = gPlayer1LevelMap[row - 1][column]) != 27 &&
                        shape != 45 && column > 0 &&
                        (shape = gPlayer1LevelMap[row][column - 1]) != 27 &&
                        shape != 45) {
                        for (scan_row = row - 3; scan_row < row + 3; ++scan_row)
                            for (scan_column = column - 2; scan_column < column + 2; ++scan_column)
                                if (scan_row >= 0 && scan_column >= 0 &&
                                    scan_row < 26 && scan_column < 19) {
                                    shape = gPlayer1LevelMap[scan_row][scan_column];
                                    if ((shape & 0x3000) == 0x1000) {
                                        if (shape & 0x8000)
                                            nearby_alternate = 1;
                                        else
                                            nearby_plain = 1;
                                    }
                                    if (shape == 27 || shape == 45)
                                        nearby_feature = 1;
                                }
                        if ((nearby_alternate || nearby_plain) &&
                            ((nearby_feature && GetRandomNumber() % 100 < 18) ||
                             GetRandomNumber() % 100 < 7)) {
                            if (nearby_alternate)
                                alternate_chance = 70;
                            else
                                alternate_chance = 20;
                            if (row < 25 && column < 18 &&
                                GetRandomNumber() % 100 < 50 &&
                                !gPlayer1LevelMap[row][column + 1] &&
                                !gPlayer1LevelMap[row + 1][column] &&
                                !gPlayer1LevelMap[row + 1][column + 1])
                                shape = 1;
                            else
                                shape = 0;
                            if (alternate_chance > GetRandomNumber() % 100) {
                                if (shape) {
                                    gPlayer1LevelMap[row][column] = 0x801e;
                                    gPlayer1LevelMap[row][column + 1] = 0x801f;
                                    gPlayer1LevelMap[row + 1][column] = 0x801d;
                                    gPlayer1LevelMap[row + 1][column + 1] = 0x8020;
                                } else
                                    gPlayer1LevelMap[row][column] = 45;
                            } else {
                                if (shape) {
                                    gPlayer1LevelMap[row][column] = 2;
                                    gPlayer1LevelMap[row][column + 1] = 3;
                                    gPlayer1LevelMap[row + 1][column] = 1;
                                    gPlayer1LevelMap[row + 1][column + 1] = 4;
                                } else
                                    gPlayer1LevelMap[row][column] = 27;
                            }
                        }
                    }
                } else if (shape == 46) {
                    if (GetRandomNumber() % 100 < 10)
                        gPlayer1LevelMap[row][column] = 0x902f;
                } else if (GetRandomNumber() % 100 < MapGetRandomGenerationParameter(7)) {
                    if (shape == 75)
                        gPlayer1LevelMap[row][column] = 73;
                    else if (shape == 76)
                        gPlayer1LevelMap[row][column] = 74;
                    else if (shape == 80)
                        gPlayer1LevelMap[row][column] = 77;
                }
            }
        }
}

/* Choose a tile alternative, using the transition descriptor when the
 * map word carries a descriptor number in bits 8 through 12. */
void MapChooseMetatileAlternatives()
{
    short row, column, choice, shape_number, shape_count;
    unsigned char old_shape, transition, bank;
    unsigned short new_cell;
    unsigned char *shapes, *alternatives;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            old_shape = gPlayer1LevelMap[row][column] & 0xff;
            new_cell = gPlayer1LevelMap[row][column] & 0xc000;
            if (!(gPlayer1LevelMap[row][column] & 0x1f00)) {
                alternatives = TileAlternativePointers[old_shape];
                choice = GetRandomNumber() % alternatives[0];
                ++choice;
            } else {
                transition = (gPlayer1LevelMap[row][column] & 0x1f00) >> 8;
                --transition;
                bank = MapTransitionDescriptors[transition].bank;
                shapes = MapTransitionDescriptors[transition].shapes;
                alternatives = MapTransitionDescriptors[transition].alternatives;
                shape_count = *shapes++;
                for (shape_number = 0; shape_number < shape_count; ++shape_number) {
                    if (old_shape == *shapes) {
                        choice = GetRandomNumber();
                        choice &= 1;
                        if (shape_number >= shape_count / 2 && (bank & 15) != 15)
                            bank += 7;
                        new_cell |= (unsigned short)(bank << 8);
                        break;
                    }
                    ++shapes;
                    alternatives += 2;
                }
            }
            new_cell |= (unsigned short)alternatives[choice];
            if (new_cell & 0x4000)
                new_cell |= 0x2000;
            gPlayer1LevelMap[row][column] = new_cell;
        }
}

/* Exchange the two map planes and exchange each visibility byte's nibbles. */
void MapSwapPlayerBanks()
{
    unsigned short row, column, saved_cell;
    unsigned char saved_mask;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            saved_cell = gPlayer1LevelMap[row][column];
            gPlayer1LevelMap[row][column] = gPlayer2LevelMap[row][column];
            gPlayer2LevelMap[row][column] = saved_cell;
            saved_mask = gMapPathDirectionBanks[row][column];
            gMapPathDirectionBanks[row][column] <<= 4;
            gMapPathDirectionBanks[row][column] |= saved_mask >> 4;
        }
}


#include "elevator.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "random.h"

/* Copy one player's complete map and visibility nibble to the other. */
void CopyLevelBetweenPlayers(player)
short player;
{
    unsigned short row, column;
    unsigned short *source, *destination;
    unsigned char mask;

    if (!player) {
        source = gPlayer1LevelMap[0];
        destination = gPlayer2LevelMap[0];
    } else {
        source = gPlayer2LevelMap[0];
        destination = gPlayer1LevelMap[0];
    }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            *destination++ = *source++;
            mask = gMapPathDirectionBanks[row][column];
            if (!player)
                gMapPathDirectionBanks[row][column] = (mask << 4) | (mask & 15);
            else
                gMapPathDirectionBanks[row][column] = (mask >> 4) | (mask & 0xf0);
        }
}

/* Exchange a player's map with the saved map, preserving the other
 * player's visibility nibble in the active mask plane. */
void MapSwapPlayerWithSavedBank(player)
short player;
{
    unsigned short row, column, saved_cell;
    unsigned short *map, *saved_map = gSavedLevelMap[0];
    unsigned char mask;

    if (!player) {
        gSavedMapLevel = gPlayer1MapLevel;
        map = gPlayer1LevelMap[0];
    } else {
        gSavedMapLevel = gPlayer2MapLevel;
        map = gPlayer2LevelMap[0];
    }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            saved_cell = *map;
            *map++ = *saved_map;
            *saved_map++ = saved_cell;
            mask = gMapPathDirectionBanks[row][column];
            if (!player) {
                gMapPathDirectionBanks[row][column] = (mask & 0xf0) | (gSavedMapPathDirections[row][column] & 15);
                gSavedMapPathDirections[row][column] = mask;
            } else {
                gMapPathDirectionBanks[row][column] = (gSavedMapPathDirections[row][column] << 4) | (mask & 15);
                gSavedMapPathDirections[row][column] = mask >> 4;
            }
        }
}

/* Install the fixed seed table or draw a seed for each of the 26 levels. */
void GenerateLevelSeeds()
{
    short level;

    for (level = 0; level < 26; ++level)
        if (gWorldType)
            gLevelSeeds[level] = FixedWorldSeeds[level];
        else
            gLevelSeeds[level] = GetRandomNumber();
    gPlayer1MapLevel = 1;
    gPlayer2MapLevel = 1;
    gSavedMapLevel = -1;
    gHighestLevel = 1;
}

/* Generate separated positions inside the level-dependent coordinate
 * bounds and return the requested position count. */
int MapGenerateSeparatedAnchors()
{
    short previous, position, min_y, min_x, count, max_y, max_x;

    for (position = 0; position < 8; ++position) {
        gLevelAnchorPositions[position].x = 0;
        gLevelAnchorPositions[position].y = 0;
    }
    if (gRequestedMapLevel < 4) {
        min_x = 5;
        max_x = 13;
        min_y = 8;
        max_y = 17;
    } else {
        min_x = 2;
        max_x = 16;
        min_y = 2;
        max_y = 23;
    }
    count = MapGetRandomGenerationParameter(6);
    for (position = 0; position < count; ++position) {
        gLevelAnchorPositions[position].x =
            GetRandomNumber() % (max_x - min_x + 1) + min_x;
        gLevelAnchorPositions[position].y =
            GetRandomNumber() % (max_y - min_y + 1) + min_y;
        for (previous = position - 1; previous >= 0; --previous)
            if (abs(gLevelAnchorPositions[position].x - gLevelAnchorPositions[previous].x) < 3 &&
                abs(gLevelAnchorPositions[position].y - gLevelAnchorPositions[previous].y) < 5) {
                --position;
                break;
            }
    }
    return count;
}


#include "ram_elevator.h"
#include "map_calls.h"
#include "random.h"

/* Connect generated positions and extend a random path from each one.
 * MATCHING QUALIFICATION: the three identity assignments below emit no
 * instructions. They retain the original cursor/target/probability register
 * allocation; they are compiler coaxing, not claimed historical operations.
 * Removing any one of the three, or using natural split arithmetic instead,
 * does not match. All assigned values are initialized before being read.
 */
void MapConnectAnchorPositions(count)
short count;
{
    short position, target_x, target_y;

    MapCarveDirectionalPath(gLevelAnchorPositions[0].y, gLevelAnchorPositions[0].x,
               gLevelAnchorPositions[1].y, gLevelAnchorPositions[1].x, 20, 60);
    MapCarveDirectionalPath(gLevelAnchorPositions[1].y, gLevelAnchorPositions[1].x,
               gLevelAnchorPositions[2].y, gLevelAnchorPositions[2].x, 20, 60);
    for (position = 0; position < count; ++position) {
        target_x = ((short)GetRandomNumber() & 3) + 4;
        target_x = target_x;
        if (gLevelAnchorPositions[position].x + target_x < 17 &&
            GetRandomNumber() % 100 < 50)
            target_x += gLevelAnchorPositions[position].x;
        else {
            if (gLevelAnchorPositions[position].x - target_x <= 2) {
                --position;
                continue;
            }
            target_x = gLevelAnchorPositions[position].x - target_x;
        }
        target_y = ((short)GetRandomNumber() & 3) + 5;
        target_y = target_y;
        if (gLevelAnchorPositions[position].y + target_y < 24 &&
            GetRandomNumber() % 100 < 50) {
            target_y += gLevelAnchorPositions[position].y;
            target_y = target_y;
        } else {
            if (gLevelAnchorPositions[position].y - target_y <= 2) {
                --position;
                continue;
            }
            target_y = gLevelAnchorPositions[position].y - target_y;
        }
        MapCarveDirectionalPath(gLevelAnchorPositions[position].y, gLevelAnchorPositions[position].x,
                   target_y, target_x, 1, 0);
    }
}


#include "entities.h"
#include "elevator.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "random.h"

/* Reuse a player or saved map when possible; otherwise generate the requested
 * level under its stored random seed. Successful loads restore the prior random
 * state; a rejected generation returns 1 without restoring it. */
int ChangeLevel(player)
short player;
{
    char loaded = 0;
    short row, column, position_count;
    unsigned short filled_cells;
    long saved_random;

    saved_random = StackBasedRandom(gLevelSeeds[gRequestedMapLevel]);
    if (gRequestedMapLevel > 1)
        position_count = MapGenerateSeparatedAnchors();
    if (!player) {
        if (gPlayer1MapLevel == gRequestedMapLevel)
            loaded = 1;
        else {
            if (gPlayer2MapLevel == gRequestedMapLevel) {
                if (gPlayer1MapLevel)
                    MapSwapPlayerWithSavedBank(0);
                CopyLevelBetweenPlayers(1);
                loaded = 1;
            } else if (gSavedMapLevel == gRequestedMapLevel) {
                MapSwapPlayerWithSavedBank(0);
                loaded = 1;
            } else {
                if (gPlayer1MapLevel > 1)
                    MapSwapPlayerWithSavedBank(0);
            }
            gPlayer1MapLevel = gRequestedMapLevel;
        }
    } else {
        if (gPlayer2MapLevel == gRequestedMapLevel)
            loaded = 1;
        else {
            if (gPlayer1MapLevel == gRequestedMapLevel) {
                if (gPlayer1MapLevel)
                    MapSwapPlayerWithSavedBank(1);
                CopyLevelBetweenPlayers(0);
                loaded = 1;
            } else if (gSavedMapLevel == gRequestedMapLevel) {
                MapSwapPlayerWithSavedBank(1);
                loaded = 1;
            } else {
                if (gPlayer2MapLevel > 1)
                    MapSwapPlayerWithSavedBank(1);
                MapSwapPlayerBanks();
            }
            gPlayer2MapLevel = gRequestedMapLevel;
        }
    }
    if (!loaded && gRequestedMapLevel <= 1) {
        if (!gRequestedMapLevel) {
            for (row = 0; row < 26; ++row)
                for (column = 0; column < 19; ++column)
                    gPlayer1LevelMap[row][column] = 34;
            for (row = 0; row < 5; ++row)
                for (column = 0; column < 4; ++column)
                    gPlayer1LevelMap[row + 12][column + 9] = Level0MapData[row][column];
        } else {
            for (row = 0; row < 26; ++row)
                for (column = 0; column < 19; ++column)
                    gPlayer1LevelMap[row][column] = Level1MapData[row][column];
        }
        if (player == 1)
            MapSwapPlayerBanks();
        loaded = 1;
    }
    if (loaded) {
        StackBasedRandom(saved_random);
        return 0;
    }
    while (gSpriteGraphicsUploadPending)
        ;
    if (gRequestedMapLevel > gHighestLevel)
        gHighestLevel = gRequestedMapLevel;
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            gPlayer1LevelMap[row][column] = 0;
            gMapPathDirectionBanks[row][column] &= 0xf0;
        }
    MapConnectAnchorPositions(position_count);
    filled_cells = MapGetRandomGenerationParameter(3);
    filled_cells = filled_cells * 19 * 26;
    filled_cells /= 100;
    for (row = 0; row < position_count; ++row)
        MapGrowTerrainRegion(4096, row, (row + 1) * (filled_cells / position_count),
                    gLevelAnchorPositions[row].y, gLevelAnchorPositions[row].x);
    MapStabilizeRegionConnections();
    if (MapIsAnchorConnectionMissing())
        return 1;
    MapSeedFilledTerrainRegions();
    MapBuildRegionEdgeShapes(position_count);
    MapBuildFilledTerrainShapes();
    MapBuildDirectionalPathShapes();
    MapScatterTerrainFeatures();
    MapAssignTransitionChain();
    MapChooseMetatileAlternatives();
    if (player == 1)
        MapSwapPlayerBanks();
    StackBasedRandom(saved_random);
    return 0;
}

/* Copy the complete final-level map to both player planes. The level bytes
 * are written once per cell, as in the observed loop. */
void LoadFinalLevel(level)
short level;
{
    short row, column;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            gPlayer1LevelMap[row][column] = FunkatronMapData[row][column];
            gPlayer2LevelMap[row][column] = FunkatronMapData[row][column];
            gPlayer1MapLevel = level;
            gPlayer2MapLevel = level;
        }
}

/* Load the final map directly, or retry ordinary generation with a fresh
 * seed until ChangeLevel accepts the generated map. */
void MapLoadPlayerLevel(player, level)
short player, level;
{
    if (level >= 26) {
        if (gPlayer1MapLevel != level)
            LoadFinalLevel(level);
        return;
    }
    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        player = 0;
    gRequestedMapLevel = level;
    while (ChangeLevel(player)) {
        gLevelSeeds[level] = GetRandomNumber();
        if (!player)
            gPlayer1MapLevel = -1;
        else
            gPlayer2MapLevel = -1;
    }
}
