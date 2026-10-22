extern unsigned short ram_ff8196;

/* Store the high bit of the flag, or clear the word. */
void ToggleLOC_00FF8196(enabled)
short enabled;
{
    if (enabled)
        ram_ff8196 = 0x8000;
    else
        ram_ff8196 = 0;
}
