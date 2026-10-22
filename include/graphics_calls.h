#ifndef TJE_GRAPHICS_CALLS_H
#define TJE_GRAPHICS_CALLS_H

/* K&R return interfaces for graphics routines. */
extern short MoveCameraDown();
extern short MoveCameraLeft();
extern short MoveCameraRight();
extern short MoveCameraUp();
extern void TryUpdateCameraX();
extern void TryUpdateCameraY();
extern void UpdateCameraX();
extern void UpdateCameraY();
extern void func_0015a8();
extern unsigned short func_001612();
extern unsigned long func_001670();
extern void func_001b48();
extern void func_00200a();
extern void func_00219c();
extern void func_00267a();
extern void func_0028f6();
extern void func_002e6a();
extern void func_023086();
extern void func_023154();

/* ROM-only camera interfaces retain their current caller spellings;
 * the ABI alone does not uniquely establish historical C return types. */
extern void func_0016d2();
extern void func_001ba8();
extern void func_0020aa();

#endif
