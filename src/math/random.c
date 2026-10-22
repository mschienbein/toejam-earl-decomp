extern long ram_ff91e2;

/* Park-Miller generator, using Schrage's split to avoid overflowing 31 bits.
 * The original public result is the low unsigned 16 bits of the new seed. */
unsigned short GetRandomNumber()
{
    long high, low, next;

    high = ram_ff91e2 / 127773L;
    low = ram_ff91e2 % 127773L;
    next = 16807L * low - 2836L * high;
    if (next > 0)
        ram_ff91e2 = next;
    else
        ram_ff91e2 = next + 2147483647L;
    return ram_ff91e2;
}
