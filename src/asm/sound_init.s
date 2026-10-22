| Restore the initial music-driver RAM image before loading a song template.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

.equ MUSIC_INITIAL_STATE_BYTES, 0x114

| Copy 276 bytes starting at gMusicFmChannelOwners. The explicit long counter load,
| word predecrement, and byte-at-a-time DBRA loop preserve the original code.
.globl MusicRestoreInitialState
.type MusicRestoreInitialState, @function
MusicRestoreInitialState:
    lea (MusicInitialState,%pc),%a0
    lea (gMusicFmChannelOwners).l,%a1
    move.l #MUSIC_INITIAL_STATE_BYTES,%d0
    subq.w #0x1,%d0
MusicRestoreInitialState_CopyBytes:
    move.b (%a0)+,(%a1)+
    dbra %d0,MusicRestoreInitialState_CopyBytes
    rts
.size MusicRestoreInitialState, .-MusicRestoreInitialState
