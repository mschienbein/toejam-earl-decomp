#ifndef UI_H
#define UI_H

/* Sixteen-byte ROM exchange: two optional second lines accompany the first
 * character's line and the replying character's line. */
struct ride_speech {
    char *first_line, *second_line, *reply_first_line, *reply_second_line;
};

#endif
