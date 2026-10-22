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
extern short gCamera1TileRowCursor;
extern short gCamera1TileColumnCursor;
extern short gCamera2TileRowCursor;
extern short gCamera2TileColumnCursor;
extern short gCamera1PendingTileRow;
extern short gCamera2PendingTileRow;
extern unsigned short gCameraRowUploadWords[2][45];
extern char gCameraRowUploadVramRow[];
extern char gCameraRowUploadStartColumn[];
extern char gCameraRowUploadPending[];
extern char gCameraRowUploadWordCount[];
extern unsigned short gCameraColumnUploadWords[2][32];
extern char gCameraColumnUploadStartRow[];
extern char gCameraColumnUploadVramColumn[];
extern char gCameraColumnUploadPending[];
extern char gCameraColumnUploadWordCount[];
extern char gCamera1PendingTileDirections;
extern char gCamera2PendingTileDirections;
extern unsigned short gCameraTileAttributes;

/* gCameraColumnUploadHeightLowByte is the existing low-byte alias of the gCameraColumnUploadHeight word
 * on the big-endian target, not an independent scalar allocation. */
extern short gCameraColumnUploadHeight;
extern char gCameraColumnUploadHeightLowByte;

/* Fixed-point follow positions shared with camera setup. */
extern long gCamera1FollowXFixed;
extern long gCamera1FollowYFixed;
extern long gCamera2FollowXFixed;
extern long gCamera2FollowYFixed;

#endif
