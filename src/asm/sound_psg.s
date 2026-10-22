| PSG effect requests and their longer bus-release/retry fragment.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Retry entry owned by AudioSubmitPendingIndexedCommand: release the bus, delay 124 iterations, then try again.
.globl AudioRetryPendingPsgMailbox
.type AudioRetryPendingPsgMailbox, @function
AudioRetryPendingPsgMailbox:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    moveq #PSG_RETRY_DELAY_MINUS1,%d0
.L_psg_retry_delay:
    dbra %d0,.L_psg_retry_delay
.size AudioRetryPendingPsgMailbox, .-AudioRetryPendingPsgMailbox

| Submit the pending PSG effect ID using command 1 once the PCM mailbox lock is clear.
| Mark the request consumed even on bus timeout; an accepted command starts the 15-tick status timer.
.globl AudioSubmitPendingIndexedCommand
.type AudioSubmitPendingIndexedCommand, @function
AudioSubmitPendingIndexedCommand:
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
    bne.s AudioRetryPendingPsgMailbox

    | Post the effect ID and command 1, then start the status countdown.
    move.w (gPendingPsgEffect).l,%d0
    move.b %d0,(Z80_PSG_EFFECT_ID).l
    move.b #Z80_CMD_PSG,(Z80_COMMAND).l
    move.w #PSG_STATUS_TICKS,(gPsgEffectStatus).l

    | Release the bus and consume the pending effect request, including the timeout path.
.L_psg_release:
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    move.w #SOUND_REQUEST_NONE,(gPendingPsgEffect).l
    rts
.size AudioSubmitPendingIndexedCommand, .-AudioSubmitPendingIndexedCommand
