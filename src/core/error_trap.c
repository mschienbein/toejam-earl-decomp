/* ErrorTrap: the unhandled-exception vectors (bus/address error, illegal
 * instruction, traps, spurious and unused interrupts) all point here; the
 * machine stops in a two-byte self-branch. */

void ErrorTrap()
{
    for (;;)
        ;
}
