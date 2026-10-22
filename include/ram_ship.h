#ifndef TJE_RAM_SHIP_H
#define TJE_RAM_SHIP_H

/* Ten per-piece level assignments; zero marks an already collected piece. */
extern char gShipPieceLevelAssignments[10];

/* The canonical gShipScreenState name is retained for this existing
 * ten-byte ship-display state view, not substituted for gShipPieceLevelAssignments.
 * gShipBackgroundAnimationTimer and gShipPieceIndicatorEntity also name its bytes [2] and [6]. */
extern char gShipScreenState[10];
extern char gShipBackgroundAnimationTimer;
extern char gShipPieceIndicatorEntity;

#endif
