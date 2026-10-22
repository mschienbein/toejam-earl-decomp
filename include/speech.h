#ifndef TJE_SPEECH_H
#define TJE_SPEECH_H

/* Shared K&R speech return interfaces. */
extern void func_01df1a();
extern void func_01e012();

/* Represented C definitions establish these return types. */
extern short GenerateSpeech();
extern void func_01e2c4();
extern int func_01e312();
extern void func_01e4e4();

/* ROM-only speech interface: retain its current int caller spelling without
 * claiming that ABI normalization proves the historical C return type. */
extern int func_01e38e();

#endif
