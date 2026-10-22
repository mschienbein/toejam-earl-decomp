#ifndef TJE_SOUND_TYPES_H
#define TJE_SOUND_TYPES_H

/* Observed 38-byte sound-channel view; byte arrays retain the existing ABI. */
struct music_channel_reset {
    char restart_stream_bytes[4];    /* 00: saved stream pointer bytes; song/template loading writes. */
    char *current_stream;            /* 04: active bytecode cursor; sound loading/sequencing writes. */
    char callback_and_channel_bytes[30]; /* 08..25: callback, FM, note and loop state; sound driver writes. */
};

#endif
