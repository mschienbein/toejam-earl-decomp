| Upload the Z80 sound driver and perform its reset/bus handshake.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Request the Z80 bus with a bounded poll, upload SoundDriverProgram, and pulse Z80 reset.
| SoundDriverProgramSize is the inclusive DBRA counter, not a byte count to decrement here.
.globl LoadSoundDriverToZ80
.type LoadSoundDriverToZ80, @function
LoadSoundDriverToZ80:
    move.l %d2,-(%sp)
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.w #Z80_BUS_POLL_LIMIT,%d2
FurtherZ80Initialisation_Loop1:
    subq.w #0x1,%d2
    beq.w FurtherZ80Initialisation_End
    move.w (Z80_BUS_REQUEST).l,%d1
    btst #Z80_BUS_BUSY_BIT,%d1
    bne.s FurtherZ80Initialisation_Loop1

    | Copy the inclusive driver byte range while the bus is granted.
    lea (SoundDriverProgram).l,%a0
    lea (Z80_RAM).l,%a1
    move.w (SoundDriverProgramSize).l,%d0
FurtherZ80Initialisation_Loop2:
    move.b (%a0)+,(%a1)+
    dbra %d0,FurtherZ80Initialisation_Loop2

    | Pulse reset and release the bus; MULU instructions retain the original fixed instruction delays.
    move.w #Z80_RESET_ASSERT,(Z80_RESET).l
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    mulu.w %d1,%d0
    move.w #Z80_RESET_RELEASE,(Z80_RESET).l
    mulu.w %d1,%d0
FurtherZ80Initialisation_End:
    movem.l (%sp)+,%d2
    move.w #SOUND_STARTUP_TICKS,(ram_ffe486).l
    rts
.size LoadSoundDriverToZ80, .-LoadSoundDriverToZ80
