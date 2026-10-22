extern long gRNGState;

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
