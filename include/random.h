#ifndef TJE_RANDOM_H
#define TJE_RANDOM_H

/* K&R return interfaces for math routines. */
extern unsigned short GetRandomNumber();
extern void PreGenerateRandoms();
extern long StackBasedRandom();

/* ROM math-runtime entry. Retain its current long caller interface without
 * claiming that the ABI establishes a historical C source spelling. */
extern long abs();

#endif
