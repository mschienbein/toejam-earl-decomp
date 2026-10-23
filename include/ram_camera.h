#ifndef TJE_RAM_CAMERA_H
#define TJE_RAM_CAMERA_H

/* MoveCameraUp/Down/Right/Left set these entering-edge bits for BuildPendingCameraTiles. */
#define CAMERA_TILE_ENTER_TOP 1
#define CAMERA_TILE_ENTER_RIGHT 2
#define CAMERA_TILE_ENTER_BOTTOM 4
#define CAMERA_TILE_ENTER_LEFT 8
/* BuildPendingCameraTiles suppresses a transfer when both opposed edges are pending. */
#define CAMERA_TILE_VERTICAL_EDGES 5
#define CAMERA_TILE_HORIZONTAL_EDGES 10

/* Committed and pending camera coordinates and split-screen selection. */
extern short gCamera1CommittedX;
extern short gCamera1CommittedY;
extern short gCamera2CommittedX;
extern short gCamera2CommittedY;
extern short gCamera1PendingX;
extern short gCamera1PendingY;
extern short gCamera2PendingX;
extern short gCamera2PendingY;
extern short gCamera1PendingTileColumn;
extern short gCamera2PendingTileColumn;
extern char gSplitScreenEnabled;

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
