| Song-template bank selection, channel-template loading, and restart scheduling.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

.equ SONG_TEMPLATE_BYTES, (MUSIC_CHANNEL_COUNT_MINUS1+1)*MUSIC_CHANNEL_SIZE

| Set the restart countdown to two ticks. An enabled alternate bank adds seven
| to the low word of the caller's stack argument before sharing the loader.
.globl MusicSelectSongTemplateBank
.type MusicSelectSongTemplateBank, @function
MusicSelectSongTemplateBank:
    move.w #SOUND_STARTUP_TICKS,(ram_ffe486).l
    tst.w (gMusicAlternateTemplateBankEnabled).l
    beq.s MusicLoadSongChannelTemplates
    addq.w #SOUND_NEXT_SELECTION_BIAS,(0x6,%sp)
    | The alternate-bank path falls through; do not insert an RTS here.
.size MusicSelectSongTemplateBank, .-MusicSelectSongTemplateBank

| Release all FM channels, restore the initial driver state, then select
| and copy one six-channel (228-byte) ROM template using the caller's index.
.globl MusicLoadSongChannelTemplates
.type MusicLoadSongChannelTemplates, @function
MusicLoadSongChannelTemplates:
    jsr (AudioReleaseAllFmChannels,%pc)
    jsr (RAMWritesRoutine1,%pc)
    move.l (0x4,%sp),%d0
    mulu.w #SONG_TEMPLATE_BYTES,%d0
    lea (SongChannelTemplates,%pc),%a0
    adda.l %d0,%a0
    lea (gMusicChannel1Ptr1).l,%a1
    move.l #SONG_TEMPLATE_BYTES,%d0
    subq.w #0x1,%d0
.L_copy_song_template_bytes:
    move.b (%a0)+,(%a1)+
    dbra %d0,.L_copy_song_template_bytes
    rts
.size MusicLoadSongChannelTemplates, .-MusicLoadSongChannelTemplates

| Clear this channel's callback, then advance the restart/countdown state.
| The -2 sentinel and the zero transition select existing wrapper calls; a busy request defers one tick.
.globl MusicProcessTemplateRestartHook
.type MusicProcessTemplateRestartHook, @function
MusicProcessTemplateRestartHook:
    clr.l (MUSIC_CALLBACK,%a0)
    moveq #0,%d0
    move.b (VBLANK_FLAG_8).l,%d0
    subq.w #0x1,(ram_ffe486).l
    beq.s .L_schedule_next
    move.w (ram_ffe486).l,%d1
    cmpi.w #SOUND_RESTART_SENTINEL,%d1
    beq.s .L_schedule_restart
    rts

    | At the -2 sentinel, restart only when the inhibit flag is clear.
.L_schedule_restart:
    tst.w (gMusicAlternateTemplateBankEnabled).l
    bne.s .L_schedule_next
    move.l %d0,-(%sp)
    bsr.s MusicSelectSongTemplateBank
    addq.w #0x4,%sp
    rts

    | At zero, request the next selection unless the busy flag requires one-tick deferral.
.L_schedule_next:
    tst.w (gBoomBoxMusicTemplateHold).l
    bne.s .L_schedule_defer
    addq.b #SOUND_NEXT_SELECTION_BIAS,%d0
    move.l %d0,-(%sp)
    bsr.s MusicLoadSongChannelTemplates
    addq.l #0x4,%sp
    rts
.L_schedule_defer:
    move.w #0x1,(ram_ffe486).l
    rts
.size MusicProcessTemplateRestartHook, .-MusicProcessTemplateRestartHook
