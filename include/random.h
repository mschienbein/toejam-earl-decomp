#ifndef TJE_RANDOM_H
#define TJE_RANDOM_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
#ifndef TJE_LOCAL_GETRANDOMNUMBER
extern unsigned short GetRandomNumber();
#endif
extern void PreGenerateRandoms();
extern long StackBasedRandom();
extern long abs();

#endif
