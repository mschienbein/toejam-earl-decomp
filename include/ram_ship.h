#ifndef TJE_RAM_SHIP_H
#define TJE_RAM_SHIP_H

/* Ten per-piece level assignments; zero marks an already collected piece. */
extern char gShipPieceLevelAssignments[10];

/* Ten-byte heterogeneous display record, distinct from the piece-level array.
 * The catalogue's gShipBackgroundAnimationTimer and gShipPieceIndicatorEntity
 * symbols alias background_timer and indicator_entity; they are not allocations. */
struct ship_screen_state {
    char background_frame;          /* 00: selected ship/transition background frame. */
    char unused_01;                 /* 01: no observed access. */
    char background_timer;          /* 02: ticks until the next background frame. */
    char unused_03;                 /* 03: no observed access. */
    char pending_piece_notice_level;/* 04: newly reached piece-bearing level, zero when consumed. */
    char unused_05;                 /* 05: no observed access. */
    char indicator_entity;          /* 06: pending-notice entity handle, zero when absent. */
    char unused_tail[3];            /* 07..09: no observed access. */
};
extern struct ship_screen_state gShipScreenState;
extern char gShipBackgroundAnimationTimer;
extern char gShipPieceIndicatorEntity;

#endif
