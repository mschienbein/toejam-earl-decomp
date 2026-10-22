#ifndef TJE_RAM_CAMERA_H
#define TJE_RAM_CAMERA_H

/* Committed and pending camera coordinates and split-screen selection. */
extern short P1_CAMERA_X;
extern short P1_CAMERA_Y;
extern short P2_CAMERA_X;
extern short P2_CAMERA_Y;
extern short TOEJAM_CAMERA_NEWX;
extern short TOEJAM_CAMERA_NEWY;
extern short EARL_CAMERA_NEWX;
extern short EARL_CAMERA_NEWY;
extern short TOEJAM_CAMERA_X;
extern short EARL_CAMERA_X;
extern char SPLIT_SCREEN;

/* Scroll positions and pending row/column transfer state. Incomplete
 * outer bounds retain the existing views, not inferred allocations. */
extern short ram_ff803e;
extern short ram_ff8040;
extern short ram_ff8042;
extern short ram_ff8044;
extern short ram_ff8046;
extern short ram_ff804a;
extern unsigned short ram_ff804e[2][45];
extern char ram_ff8102[];
extern char ram_ff8104[];
extern char ram_ff8106[];
extern char ram_ff8108[];
extern unsigned short ram_ff810a[2][32];
extern char ram_ff818a[];
extern char ram_ff818c[];
extern char ram_ff818e[];
extern char ram_ff8190[];
extern char ram_ff8192;
extern char ram_ff8194;
extern unsigned short ram_ff8196;

/* ram_ff8199 is the existing low-byte alias of the ram_ff8198 word
 * on the big-endian target, not an independent scalar allocation. */
extern short ram_ff8198;
extern char ram_ff8199;

/* Fixed-point follow positions shared with camera setup. */
extern long ram_ff819a;
extern long ram_ff819e;
extern long ram_ff81a2;
extern long ram_ff81a6;

#endif
