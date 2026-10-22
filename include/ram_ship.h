#ifndef TJE_RAM_SHIP_H
#define TJE_RAM_SHIP_H

/* Ten per-piece level assignments; zero marks an already collected piece. */
extern char ram_ffe212[10];

/* The canonical gShipPieceLevels name is retained for this existing
 * ten-byte ship-display state view, not substituted for ram_ffe212.
 * ram_ffe21e and ram_ffe222 also name its bytes [2] and [6]. */
extern char gShipPieceLevels[10];
extern char ram_ffe21e;
extern char ram_ffe222;

#endif
