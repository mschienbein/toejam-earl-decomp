| Advance the sound restart/countdown state and call the existing request wrappers.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Clear this channel's callback, then advance the restart/countdown state.
| The -2 sentinel and the zero transition select existing wrapper calls; a busy request defers one tick.
.globl func_03e4aa
.type func_03e4aa, @function
func_03e4aa:
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
    tst.w (ram_ffe476).l
    bne.s .L_schedule_next
    move.l %d0,-(%sp)
    bsr.s func_03e46a
    addq.w #0x4,%sp
    rts

    | At zero, request the next selection unless the busy flag requires one-tick deferral.
.L_schedule_next:
    tst.w (ram_ffe474).l
    bne.s .L_schedule_defer
    addq.b #SOUND_NEXT_SELECTION_BIAS,%d0
    move.l %d0,-(%sp)
    bsr.s func_03e47e
    addq.l #0x4,%sp
    rts
.L_schedule_defer:
    move.w #0x1,(ram_ffe486).l
    rts
.size func_03e4aa, .-func_03e4aa
