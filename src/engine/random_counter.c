extern unsigned short GetRandomNumber();
extern char gPlayer1TomatoRainTimer[2];
extern char TOMATO_RAIN_TIMER[2];

/* Choose a counter in [50, 80] and clear its companion byte. */
void SetTomatoRainTomatoes(index)
int index;
{
    gPlayer1TomatoRainTimer[index] = GetRandomNumber() % 31 + 50;
    TOMATO_RAIN_TIMER[index] = 0;
}
