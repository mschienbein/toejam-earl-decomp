| Direct PCM requests and their bus-release/retry fragment.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Retry entry owned by func_03f62a: release the bus so the Z80 can snapshot its PCM mailbox.
| Delay 24 DBRA iterations and fall through to the normal request entry.
.globl loc_03f61c
.type loc_03f61c, @function
loc_03f61c:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    moveq #PCM_RETRY_DELAY_MINUS1,%d0
.L_pcm_retry_delay:
    dbra %d0,.L_pcm_retry_delay
.size loc_03f61c, .-loc_03f61c

| Submit gMusicUnknownE480's direct PCM pointer/count when the mailbox snapshot lock is clear.
| An accepted request starts the speech timer and clears the pending pointer; timeout retains it.
.globl func_03f62a
.type func_03f62a, @function
func_03f62a:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.w #Z80_BUS_POLL_LIMIT,%d2
.L_pcm_bus_wait:
    subq.w #0x1,%d2
    beq.w .L_pcm_bus_timeout
    move.w (Z80_BUS_REQUEST).l,%d1
    btst #Z80_BUS_BUSY_BIT,%d1
    bne.s .L_pcm_bus_wait

    | A nonzero lock requires releasing the bus and trying again, not holding the Z80 stopped.
    move.b (Z80_PCM_MAILBOX_LOCK).l,%d0
    bne.s loc_03f61c

    | Copy the direct PCM pointer and length into the little-endian mailbox fields.
    movea.l (gMusicUnknownE480).l,%a0
    move.w (%a0)+,%d1
    moveq #0,%d2
    move.w %d1,%d2
    move.l %a0,%d0
    move.b %d0,(Z80_PCM_ADDRESS_LO).l
    lsr.l #0x8,%d0
    move.b %d0,(Z80_PCM_ADDRESS_MID).l
    lsr.l #0x8,%d0
    move.b %d0,(Z80_PCM_ADDRESS_HI).l
    move.b %d1,(Z80_PCM_LENGTH_LO).l
    lsr.l #0x8,%d1
    move.b %d1,(Z80_PCM_LENGTH_HI).l

    | The high address byte is also written to the PSG-ID field; the PCM driver does not read that field.
    move.b %d0,(Z80_PSG_EFFECT_ID).l
    move.b #Z80_CMD_PCM,(Z80_COMMAND).l

    | Derive the speech timer as floor(sample_length / 138) + 5.
    divu.w #PCM_SPEECH_BYTES_PER_TICK,%d2
    addq.w #PCM_SPEECH_EXTRA_TICKS,%d2
    move.w %d2,(TOEJAM_SPEECH_COUNTER).l
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    move.w #SOUND_REQUEST_NONE,(gMusicUnknownE47E).l
    clr.l (gMusicUnknownE480).l
    rts

    | Bus timeout: release the bus and cancel only the queued PSG request.
.L_pcm_bus_timeout:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    move.w #SOUND_REQUEST_NONE,(gMusicUnknownE47E).l
    rts
.size func_03f62a, .-func_03f62a

| Submit an immediate PCM pointer only when speech/inhibit/PSG-active checks allow it.
| Unlike func_03f62a, this entry preserves d0-d2/a0 and does not start the speech timer.
.globl func_03f6c6
.type func_03f6c6, @function
func_03f6c6:
    link %a6,#0
    movem.l %d0-%d2/%a0,-(%sp)
    tst.w (TOEJAM_SPEECH_COUNTER).l
    bne.w .L_immediate_pcm_return
    tst.w (ram_ffe476).l
    bne.w .L_immediate_pcm_return
    bra.s .L_immediate_pcm_request

    | Retry after releasing the bus for the mailbox snapshot.
.L_immediate_pcm_retry:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    moveq #PCM_RETRY_DELAY_MINUS1,%d0
.L_immediate_pcm_delay:
    dbra %d0,.L_immediate_pcm_delay
.L_immediate_pcm_request:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.w #Z80_BUS_POLL_LIMIT,%d2
.L_immediate_pcm_bus_wait:
    subq.w #0x1,%d2
    beq.w .L_immediate_pcm_release
    move.w (Z80_BUS_REQUEST).l,%d1
    btst #Z80_BUS_BUSY_BIT,%d1
    bne.s .L_immediate_pcm_bus_wait

    | Wait for an unlocked mailbox, then abort if the driver's PSG-active byte is nonzero.
    move.b (Z80_PCM_MAILBOX_LOCK).l,%d0
    bne.s .L_immediate_pcm_retry
    move.b (Z80_PSG_ACTIVE).l,%d0
    bne.w .L_immediate_pcm_release

    | Copy the caller's direct PCM pointer and length and post command 2.
    movea.l (0x8,%a6),%a0
    move.w (%a0)+,%d1
    moveq #0,%d2
    move.w %d1,%d2
    move.l %a0,%d0
    move.b %d0,(Z80_PCM_ADDRESS_LO).l
    lsr.l #0x8,%d0
    move.b %d0,(Z80_PCM_ADDRESS_MID).l
    lsr.l #0x8,%d0
    move.b %d0,(Z80_PCM_ADDRESS_HI).l
    move.b %d1,(Z80_PCM_LENGTH_LO).l
    lsr.l #0x8,%d1
    move.b %d1,(Z80_PCM_LENGTH_HI).l

    | Preserve the original redundant write of the address high byte to the PSG-ID field.
    move.b %d0,(Z80_PSG_EFFECT_ID).l
    move.b #Z80_CMD_PCM,(Z80_COMMAND).l

    | Release the bus on both success and an aborted request.
.L_immediate_pcm_release:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
.L_immediate_pcm_return:
    movem.l (%sp)+,%d0-%d2/%a0
    unlk %a6
    rts
.size func_03f6c6, .-func_03f6c6
