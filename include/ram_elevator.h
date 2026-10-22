#ifndef TJE_RAM_ELEVATOR_H
#define TJE_RAM_ELEVATOR_H

#include "elevator.h"

struct object_state;

/* Eight map-cell position records. These existing scalar byte aliases
 * name record one's coordinates and record two's spawn coordinates;
 * they are not additional position allocations. */
extern struct elevator_position ELEVATOR_POS_TABLE[8];
extern char ram_ff911c;
extern char ram_ff911d;
extern char ram_ff911e;
extern char ram_ff911f;

/* Banked elevator records and their existing named byte alias.
 * ARRIVING_ELEV_LEVEL names ELEVATOR_LOCATION[0][1].level. */
extern struct elevator_record ELEVATOR_LOCATION[2][2];
extern char ARRIVING_ELEV_LEVEL;

/* Per-player flags and elevator scene state retain incomplete bounds.
 * ram_ffda6b is the existing scalar alias of ELEVATOR_FLAG[1]. */
extern char ram_ffda66;
extern char ELEVATOR_FLAG[];
extern char ram_ffda6b;
extern char ram_ffda6c;
extern struct object_state *ram_ffda6e;

/* Scene scroll position and acceleration. ram_ffda74 is the existing
 * low-word alias of the ram_ffda72 long on the big-endian target. */
extern long ram_ffda72;
extern short ram_ffda74;
extern short ram_ffda76;
extern short ram_ffda78;
extern unsigned char ram_ffda7a[4];
extern long ram_ffda7e;

/* Transition state and arrival/departure timers retain their byte views. */
extern char ram_ffda82[];
extern char ARRIVING_ELEV_TIME;
extern char DEPARTING_ELEV_CYCLES;
extern char DEPARTING_ELEV_TIME;

#endif
