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

/* One scripted exchange: who speaks first, who answers, and after how long. */
struct dialogue_descriptor {
    char first_speaker, reply_speaker;   /* speaker indices */
    char *first_line;                    /* opening line */
    short delay;                         /* ticks before the reply */
    char *reply;                         /* reply line */
};
#endif
