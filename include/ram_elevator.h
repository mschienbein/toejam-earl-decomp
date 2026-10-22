#ifndef TJE_RAM_ELEVATOR_H
#define TJE_RAM_ELEVATOR_H

#include "elevator.h"

struct object_state;

/* Eight map-cell position records. These existing scalar byte aliases
 * name record one's coordinates and record two's spawn coordinates;
 * they are not additional position allocations. */
extern struct elevator_position gLevelAnchorPositions[8];
extern char gArrivalElevatorMapX;
extern char gArrivalElevatorMapY;
extern char gShipPieceSpawnMapX;
extern char gShipPieceSpawnMapY;

/* Banked elevator records and their existing named byte alias.
 * gPrimaryArrivalElevatorLevel names gElevatorRecords[0][1].level. */
extern struct elevator_record gElevatorRecords[2][2];
extern char gPrimaryArrivalElevatorLevel;

/* Per-player flags and elevator scene state retain incomplete bounds.
 * gPlayer2ElevatorId is the existing scalar alias of gPlayerElevatorFlags[1]. */
extern char gSecondaryArrivalElevatorLevel;
extern char gPlayerElevatorFlags[];
extern char gPlayer2ElevatorId;
extern char gElevatorDepartureInProgress;
extern struct object_state *gElevatorRideEntity;

/* Scene scroll position and acceleration. gElevatorRideScrollYLowWord is the existing
 * low-word alias of the gElevatorRideScrollYFixed long on the big-endian target. */
extern long gElevatorRideScrollYFixed;
extern short gElevatorRideScrollYLowWord;
extern short gElevatorRideScrollVelocity;
extern short gElevatorRideScrollAcceleration;
extern unsigned char gElevatorRideSpeechEntities[4];
extern long gElevatorRideUsedDialogueMask;

/* Transition state and arrival/departure timers retain their byte views. */
extern char gPlayerElevatorApproachInputGate[];
extern char gArrivalElevatorCloseDelay;
extern char gDepartureElevatorShakeCycles;
extern char gDepartureElevatorShakeDelay;

#endif
