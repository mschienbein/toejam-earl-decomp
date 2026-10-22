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

    saved_random = StackBasedRandom(gLevelSeeds[gLevel912A]);
    if (gLevel912A > 1)
        position_count = MapGenerateSeparatedAnchors();
    if (!player) {
        if (gLevel912C == gLevel912A)
            loaded = 1;
        else {
            if (gLevel912E == gLevel912A) {
                if (gLevel912C)
                    MapSwapPlayerWithSavedBank(0);
                CopyLevelBetweenPlayers(1);
                loaded = 1;
            } else if (gLevel9130 == gLevel912A) {
                MapSwapPlayerWithSavedBank(0);
                loaded = 1;
            } else {
                if (gLevel912C > 1)
                    MapSwapPlayerWithSavedBank(0);
            }
            gLevel912C = gLevel912A;
        }
    } else {
        if (gLevel912E == gLevel912A)
            loaded = 1;
        else {
            if (gLevel912C == gLevel912A) {
                if (gLevel912C)
                    MapSwapPlayerWithSavedBank(1);
                CopyLevelBetweenPlayers(0);
                loaded = 1;
            } else if (gLevel9130 == gLevel912A) {
                MapSwapPlayerWithSavedBank(1);
                loaded = 1;
            } else {
                if (gLevel912E > 1)
                    MapSwapPlayerWithSavedBank(1);
                MapSwapPlayerBanks();
            }
            gLevel912E = gLevel912A;
        }
    }
    if (!loaded && gLevel912A <= 1) {
        if (!gLevel912A) {
            for (row = 0; row < 26; ++row)
                for (column = 0; column < 19; ++column)
                    gMapDataA[row][column] = 34;
            for (row = 0; row < 5; ++row)
                for (column = 0; column < 4; ++column)
                    gMapDataA[row + 12][column + 9] = Level0MapData[row][column];
        } else {
            for (row = 0; row < 26; ++row)
                for (column = 0; column < 19; ++column)
                    gMapDataA[row][column] = Level1MapData[row][column];
        }
        if (player == 1)
            MapSwapPlayerBanks();
        loaded = 1;
    }
    if (loaded) {
        StackBasedRandom(saved_random);
        return 0;
    }
    while (VBLANK_FLAG_2)
        ;
    if (gLevel912A > gHighestLevel)
        gHighestLevel = gLevel912A;
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            gMapDataA[row][column] = 0;
            gMapDataD[row][column] &= 0xf0;
        }
    MapConnectAnchorPositions(position_count);
    filled_cells = MapGetRandomGenerationParameter(3);
    filled_cells = filled_cells * 19 * 26;
    filled_cells /= 100;
    for (row = 0; row < position_count; ++row)
        MapGrowTerrainRegion(4096, row, (row + 1) * (filled_cells / position_count),
                    ELEVATOR_POS_TABLE[row].y, ELEVATOR_POS_TABLE[row].x);
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
            gMapDataA[row][column] = FunkatronMapData[row][column];
            gMapDataB[row][column] = FunkatronMapData[row][column];
            gLevel912C = level;
            gLevel912E = level;
        }
}

/* Load the final map directly, or retry ordinary generation with a fresh
 * seed until ChangeLevel accepts the generated map. */
void MapLoadPlayerLevel(player, level)
short player, level;
{
    if (level >= 26) {
        if (gLevel912C != level)
            LoadFinalLevel(level);
        return;
    }
    if (gPlayerLives[0] < 0)
        player = 0;
    gLevel912A = level;
    while (ChangeLevel(player)) {
        gLevelSeeds[level] = GetRandomNumber();
        if (!player)
            gLevel912C = -1;
        else
            gLevel912E = -1;
    }
}
