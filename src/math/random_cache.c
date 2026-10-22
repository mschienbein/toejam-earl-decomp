#include "ram.h"
#include "random.h"
/* Park-Miller generator, using Schrage's split to avoid overflowing 31 bits.
 * The original public result is the low unsigned 16 bits of the new seed. */
unsigned short GetRandomNumber()
{
    long high, low, next;

    high = gRNGState / 127773L;
    low = gRNGState % 127773L;
    next = 16807L * low - 2836L * high;
    if (next > 0)
        gRNGState = next;
    else
        gRNGState = next + 2147483647L;
    return gRNGState;
}

/* Normalize the seed and return the preceding generator state.
 * ORIGINAL BUG: negating the minimum 32-bit seed leaves it negative on
 * the 68000, so that input is not normalized into the positive range. */
long StackBasedRandom(seed)
long seed;
{
    long previous;

    previous = gRNGState;
    if (seed < 0)
        seed = -seed;
    if (seed >= 2147483647L)
        seed -= 2147483647L;
    if (!seed)
        seed = 1;
    gRNGState = seed;
    return previous;
}

/* Cache 59 draws without advancing the externally visible generator state. */
void PreGenerateRandoms()
{
    long saved_state;
    short random_index;

    saved_state = gRNGState;
    for (random_index = 0; random_index < 59; ++random_index)
        PREGEN_RANDOMS[random_index] = GetRandomNumber();
    ram_ff91e0 = 0;
    gRNGState = saved_state;
}
