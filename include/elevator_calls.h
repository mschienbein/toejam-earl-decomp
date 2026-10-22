#ifndef TJE_ELEVATOR_CALLS_H
#define TJE_ELEVATOR_CALLS_H

/* K&R return interfaces for elevator routines. */
extern void ElevatorRideVBlank();
extern void ElevatorInvalidateSecondaryRecords();
extern void LoadFinalLevelMap();
extern int ProcessElevator();
extern void ProcessElevatorStep1();
extern void ProcessElevatorStep2();
extern void ProcessElevators();
extern char SetupElevator();
extern void SetupInitialElevator();
extern void ElevatorSetState();
extern void PlayerSetPositionFromSavedMapCell();
extern void ElevatorCreateEntityPair();
extern void ElevatorUpdateEntityVisibility();
extern int ElevatorBlocksPlayerMovement();
extern int ElevatorBlocksEntityMovement();
extern int ElevatorContainsPosition();
extern void MapClearActiveLevelObjects();
extern void MapEnterFinalLevel();
extern void ElevatorLoadRideBackgroundTiles();
extern void ElevatorAlternateRideEntities();
extern void ElevatorPrepareRideDialogue();
extern void ElevatorUpdateRideDialogue();
extern void ElevatorBeginRideScene();
extern void ElevatorEndRideScene();
extern void ElevatorAdvanceRidersToNextLevel();
extern int ElevatorCheckPlayerProximityAndInput();
extern int ElevatorStepDepartureHeightCycles();

#endif
