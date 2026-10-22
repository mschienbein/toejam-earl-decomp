#ifndef TJE_SOUND_TYPES_H
#define TJE_SOUND_TYPES_H

/* Observed38-byte sound-channel reset view; only pointer+4 is named. */
struct music_channel_reset {
    char reserved_00[4];
    char *position;
    char reserved_08[30];
};

#endif
