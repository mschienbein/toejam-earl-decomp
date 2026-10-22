| Emit each of the six channels' stored FM operator-key masks.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Walk FM channels 5 down to 0, preserving the loop index across func_03ec74.
.globl func_03e446
.type func_03e446, @function
func_03e446:
    movem.l %d3/%a5,-(%sp)
    moveq #MUSIC_CHANNEL_COUNT_MINUS1,%d1
.L_write_channel_masks:
    movem.l %d1,-(%sp)
    jsr (func_03ec74,%pc)
    nop
    movem.l (%sp)+,%d1
    dbra %d1,.L_write_channel_masks
    movem.l (%sp)+,%d3/%a5
    clr.w (gMusicUnknownE478).l
    rts
.size func_03e446, .-func_03e446
