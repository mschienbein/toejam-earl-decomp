| Clear the six music-channel owner pointers and stream pointers.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Clear six owner-table entries and their corresponding stream pointers; reset the request flag.
.globl func_03e418
.type func_03e418, @function
func_03e418:
    lea (gMusicPointer1).l,%a0
    lea (gMusicChannel1Ptr1).l,%a1
    moveq #MUSIC_CHANNEL_COUNT_MINUS1,%d0
.L_clear_music_channels:
    clr.l (%a0)+
    clr.l (MUSIC_STREAM,%a1)
    adda.w #MUSIC_CHANNEL_SIZE,%a1
    dbra %d0,.L_clear_music_channels
    clr.w (gMusicUnknownE47A).l
    rts
.size func_03e418, .-func_03e418
