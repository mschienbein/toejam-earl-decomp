| PSG effect requests and their longer bus-release/retry fragment.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Retry entry owned by func_03f7aa: release the bus, delay 124 iterations, then try again.
.globl loc_03f79c
.type loc_03f79c, @function
loc_03f79c:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    moveq #PSG_RETRY_DELAY_MINUS1,%d0
.L_psg_retry_delay:
    dbra %d0,.L_psg_retry_delay
.size loc_03f79c, .-loc_03f79c

| Submit the pending PSG effect ID using command 1 once the PCM mailbox lock is clear.
| Mark the request consumed even on bus timeout; an accepted command starts the 15-tick status timer.
.globl func_03f7aa
.type func_03f7aa, @function
func_03f7aa:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.w #Z80_BUS_POLL_LIMIT,%d2
.L_psg_bus_wait:
    subq.w #0x1,%d2
    beq.w .L_psg_release
    move.w (Z80_BUS_REQUEST).l,%d1
    btst #Z80_BUS_BUSY_BIT,%d1
    bne.s .L_psg_bus_wait

    | Let the Z80 finish a PCM-mailbox snapshot before posting a PSG command.
    move.b (Z80_PCM_MAILBOX_LOCK).l,%d0
    bne.s loc_03f79c

    | Post the effect ID and command 1, then start the status countdown.
    move.w (gMusicUnknownE47E).l,%d0
    move.b %d0,(Z80_PSG_EFFECT_ID).l
    move.b #Z80_CMD_PSG,(Z80_COMMAND).l
    move.w #PSG_STATUS_TICKS,(gMusicUnknownE46E).l

    | Release the bus and consume the pending effect request, including the timeout path.
.L_psg_release:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    move.w #SOUND_REQUEST_NONE,(gMusicUnknownE47E).l
    rts
.size func_03f7aa, .-func_03f7aa
