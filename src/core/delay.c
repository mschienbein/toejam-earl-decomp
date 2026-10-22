/* Original 68000 busy-wait; the argument counts loop iterations, not frames. */
void DelayRoutine1(iterations)
short iterations;
{
    while (iterations)
        --iterations;
}
