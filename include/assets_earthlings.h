#ifndef TJE_ASSETS_EARTHLINGS_H
#define TJE_ASSETS_EARTHLINGS_H

#include "earthlings.h"

/* EarthlingDefinitions contains 25 logical 38-byte entries. Entry 24's final
 * reserved byte overlaps EarthlingCoordinatePairs[0]; asset partitions are
 * not logical array bounds. */
extern struct earthling_definition EarthlingDefinitions[25];
extern char EarthlingCoordinatePairs[];
extern char LevelEarthlingChoices[26][20];

/* Projectile-aim slope thresholds and launch-velocity vectors. */
extern short EarthlingThresholds[8];
extern short EarthlingVectorPairs[9][2];

#endif
