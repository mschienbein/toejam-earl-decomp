#include "assets_scenes.h"
#include "ram_map.h"
#include "ram_ship.h"
#include "random.h"
#include "ship.h"

/* In a fixed world copy the ten known levels; otherwise choose five stratified
 * levels, five distinct remaining levels, and force one piece onto level 25. */
void func_01f98e()
{
    short level, previous_piece, group, piece;

    if (gWorldType) {
        for (group = 0; group < 10; ++group)
            ram_ffe212[group] = FixedWorldShipPieces[group];
        return;
    }
    for (group = 0; group < 5; ++group) {
        do {
            piece = GetRandomNumber() % 10;
        } while (ram_ffe212[piece]);
        do {
            ram_ffe212[piece] = GetRandomNumber() %
                ((group * 5 + 5) - (group * 5 + 1) + 1) + (group * 5 + 1);
        } while (ram_ffe212[piece] <= 1 || ram_ffe212[piece] >= 25);
    }
    for (group = 0; group < 5; ++group) {
        do {
            piece = GetRandomNumber() % 10;
        } while (ram_ffe212[piece]);
        do {
            level = GetRandomNumber() % 23 + 2;
            for (previous_piece = 0; previous_piece < 10; ++previous_piece)
                if (ram_ffe212[previous_piece] == level)
                    break;
        } while (previous_piece != 10);
        ram_ffe212[piece] = level;
    }
    piece = GetRandomNumber() % 10;
    ram_ffe212[piece] = 25;
}
