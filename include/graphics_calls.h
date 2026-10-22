#ifndef TJE_GRAPHICS_CALLS_H
#define TJE_GRAPHICS_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern short MoveCameraDown();
extern short MoveCameraLeft();
extern short MoveCameraRight();
extern short MoveCameraUp();
extern void TryUpdateCameraX();
extern void TryUpdateCameraY();
extern void UpdateCameraX();
extern void UpdateCameraY();
extern void ClearCameraTileRow();
extern unsigned short GetLevelMetatileTile();
extern unsigned long GetOverlayMetatileTile();
extern void BuildCameraTileRow();
extern void ClearCameraTileColumn();
extern void BuildCameraTileColumn();
extern void RedrawCameraTileRows();
extern void SetCameraViewOrigin();
extern void BuildPendingCameraTiles();
extern void GetPlayerCameraVelocity();
extern void UpdateCameraFollowing();
extern void AnimateSplitScreenWipe();
extern void FunkatronCreateSceneEntity();
extern void FunkatronUpdateSceneEntities();
extern void FillVdpDataWords();
extern void UploadVramWordRectangle();

#endif
