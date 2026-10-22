#ifndef TJE_ASSETS_EARTHLINGS_H
#define TJE_ASSETS_EARTHLINGS_H

#include "earthlings.h"

/* EARTHLING_DATA contains 25 logical 38-byte entries. Entry 24's final
 * reserved byte overlaps EarthlingCoordinatePairs[0]; asset partitions are
 * not logical array bounds. */
extern struct earthling_definition EARTHLING_DATA[25];
extern char EarthlingCoordinatePairs[];
extern char LevelEarthlingChoices[26][20];

#endif
