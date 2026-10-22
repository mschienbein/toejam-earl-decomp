#ifndef TJE_RAM_MAP_H
#define TJE_RAM_MAP_H

/* World/level selection and per-level seeds. */
extern char gLevel912A;
extern char gLevel912C;
extern char gLevel912E;
extern char gLevel9130;
extern char gHighestLevel;
extern char gWorldType;
extern unsigned short gLevelSeeds[26];

/* Canonical map banks. ram_ff8f2c is the existing byte view beginning
 * at gMapDataD[26][0], not an additional independent terrain allocation. */
extern unsigned short gMapDataA[26][19];
extern unsigned short gMapDataB[26][19];
extern unsigned short gMapDataC[26][19];
extern unsigned char gMapDataD[52][19];
extern unsigned char ram_ff8f2c[26][19];

/* Transition-path coordinates and map-generation state. */
extern short ram_ff91e6;
extern short ram_ff91e8;
extern char ram_ff91ea;

/* DisplayMap accesses ram_ff91ec and ram_ff92a2 with the same seven-byte
 * row stride; the former 26-row extent is [INFERENCE] from the next table. */
extern unsigned char ram_ff91ec[26][7];
extern unsigned char ram_ff92a2[26][7];

/* Map-display state and the terrain-query word. ram_ffda1d is its
 * existing low-byte alias on the big-endian target. */
extern short ram_ff9358;
extern short ram_ff935a;
extern unsigned short ram_ffda1c;
extern unsigned char ram_ffda1d;

/* Eight land-animation object handles and the current traversal direction. */
extern char ram_ffe208[8];
extern char ram_ffe210;

#endif
