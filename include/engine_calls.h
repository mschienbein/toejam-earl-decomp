#ifndef TJE_ENGINE_CALLS_H
#define TJE_ENGINE_CALLS_H

/* K&R return interfaces for engine routines. */
extern void func_01d436();
extern void func_01d802();
extern void func_01d8ac();
extern void func_01da0a();
extern void func_01da9c();
extern int func_01dee8();
extern void func_02836c();

/* ROM-only reset-sequence slot interface. Retain its current caller spelling;
 * the ABI alone does not uniquely establish the historical C return type. */
extern void func_0282c0();

#endif
