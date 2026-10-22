#ifndef TJE_UI_H
#define TJE_UI_H

/* Sixteen-byte ROM exchange: two optional second lines accompany the first
 * character's line and the replying character's line. */
struct ride_speech {
    char *first_line, *second_line, *reply_first_line, *reply_second_line;
};

/* Eight-byte menu entry: byte tile coordinates, text pointer, signed selection. */
struct menu_item {
    unsigned char x, y;
    char *text;
    short selection;
};

/* Reuse the resolved opening speaker, which may differ from the incoming caller. */
#define DIALOGUE_REPLY_FIRST_SPEAKER 2

/* One scripted exchange: who speaks first, who answers, and after how long. */
struct dialogue_descriptor {
    char first_speaker, reply_speaker;   /* 0/1 force a player; reply 2 reuses the opening speaker. */
    char *first_line;                    /* opening line */
    short delay;                         /* ticks before the reply */
    char *reply;                         /* reply line */
};
#endif
