#ifndef TJE_RAM_SPEECH_H
#define TJE_RAM_SPEECH_H

/* Speech cooldown and four parallel bubble handle/lifetime slots. */
extern short TOEJAM_SPEECH_COUNTER;
extern char ram_ffe1ee[4];
extern char ram_ffe1f2[4];
extern char ram_ffe1f6;

/* Two delayed-reply timers, text pointers and speaker bytes. */
extern short ram_ffe1f8[2];
extern char *ram_ffe1fc[2];
extern char ram_ffe204[2];

/* Existing canonical alias for ram_ffe1f8[1], the second reply timer. */
extern short gPlayer1TomatoRainThrottle;

/* Existing scalar byte alias of ram_ffe204[1], the second reply speaker. */
extern char ram_ffe205;

/* Word lifetime override consumed by the next allocated bubble. */
extern short ram_ffe206;

/* Low byte of the word lifetime override ram_ffe206 on this big-endian target. */
extern char ram_ffe207;

#endif
