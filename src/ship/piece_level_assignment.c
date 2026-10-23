#include "assets_scenes.h"
#include "ram_map.h"
#include "ram_ship.h"
#include "random.h"
#include "ship.h"

/* In a fixed world copy the ten known levels; otherwise choose five stratified
 * levels, five distinct remaining levels, and force one piece onto level 25. */
void AssignShipPieceLevels()
{
    short level, previous_piece, group, piece;

    if (gWorldType) {
        for (group = 0; group < 10; ++group)
            gShipPieceLevelAssignments[group] = FixedWorldShipPieces[group];
        return;
    }
    for (group = 0; group < 5; ++group) {
        do {
            piece = GetRandomNumber() % 10;
        } while (gShipPieceLevelAssignments[piece]);
        do {
            gShipPieceLevelAssignments[piece] = GetRandomNumber() %
                ((group * 5 + 5) - (group * 5 + 1) + 1) + (group * 5 + 1);
        } while (gShipPieceLevelAssignments[piece] <= LEVEL_FIRST || gShipPieceLevelAssignments[piece] >= LEVEL_LAST_ORDINARY);
    }
    for (group = 0; group < 5; ++group) {
        do {
            piece = GetRandomNumber() % 10;
        } while (gShipPieceLevelAssignments[piece]);
        do {
            level = GetRandomNumber() % 23 + 2;
            for (previous_piece = 0; previous_piece < 10; ++previous_piece)
                if (gShipPieceLevelAssignments[previous_piece] == level)
                    break;
        } while (previous_piece != 10);
        gShipPieceLevelAssignments[piece] = level;
    }
    piece = GetRandomNumber() % 10;
    gShipPieceLevelAssignments[piece] = LEVEL_LAST_ORDINARY;
}
