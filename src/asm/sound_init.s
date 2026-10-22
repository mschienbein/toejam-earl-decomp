| Restore the initial music-driver RAM image before loading a song template.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

.equ MUSIC_INITIAL_STATE_BYTES, 0x114

| Copy 276 bytes starting at gMusicPointer1. The explicit long counter load,
| word predecrement, and byte-at-a-time DBRA loop preserve the original code.
.globl RAMWritesRoutine1
.type RAMWritesRoutine1, @function
RAMWritesRoutine1:
    lea (RAMWritesRoutine1ValuesA,%pc),%a0
    lea (gMusicPointer1).l,%a1
    move.l #MUSIC_INITIAL_STATE_BYTES,%d0
    subq.w #0x1,%d0
RAMWritesRoutine1_Loop:
    move.b (%a0)+,(%a1)+
    dbra %d0,RAMWritesRoutine1_Loop
    rts
.size RAMWritesRoutine1, .-RAMWritesRoutine1
