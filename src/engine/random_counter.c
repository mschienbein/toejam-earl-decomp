extern unsigned short GetRandomNumber();
extern char ram_ffe1e8[2];
extern char ram_ffe1ea[2];

/* Choose a counter in [50, 80] and clear its companion byte. */
void SetTomatoRainTomatoes(index)
int index;
{
    ram_ffe1e8[index] = GetRandomNumber() % 31 + 50;
    ram_ffe1ea[index] = 0;
}
