| Reset-time machine initialization; the setup records remain typed ROM assets.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "hardware.inc"

| SetupValues supplies VDP command words, bus/reset pointers, and loop counts.
.equ IO_VERSION_FROM_BUS, -0x10ff
.equ TMSS_FROM_BUS, 0x2f00
.equ TMSS_SIGNATURE, 0x53454741
.equ IO_VERSION_MASK, 0x0f
.equ VDP_INITIAL_REGS_MINUS1, 23
.equ Z80_BOOT_BYTES_MINUS1, 37
.equ CRAM_LONGS_MINUS1, 31
.equ VSRAM_LONGS_MINUS1, 19
.equ PSG_INITIAL_BYTES_MINUS1, 3
.equ PSG_FROM_VDP_DATA, 0x11
.equ SR_RESET_MASKED, 0x2700

| Skip the reset sequence when the existing I/O control state is nonzero.
| Otherwise consume SetupValues, initialize TMSS/VDP/Z80/RAM/PSG, then enter GameProgram.
.globl _start
.type _start, @function
_start:
    tst.l (IO_CONTROL_AB_CHECK).l
    bne.s PortA_OK
    tst.w (IO_CONTROL_C_CHECK).l
PortA_OK:
    bne.s PortC_OK

    | Load the reset command stream and hardware-port pointers.
    lea (SetupValues,%pc),%a5
    movem.w (%a5)+,%d5-%d7
    movem.l (%a5)+,%a0-%a4
    move.b (IO_VERSION_FROM_BUS,%a1),%d0
    andi.b #IO_VERSION_MASK,%d0
    beq.s SkipSecurity

    | TMSS-equipped hardware receives the four-character SEGA signature.
    move.l #TMSS_SIGNATURE,(TMSS_FROM_BUS,%a1)

    | Reset user-stack state and write the 24 VDP registers.
SkipSecurity:
    move.w (%a4),%d0
    moveq #0,%d0
    movea.l %d0,%a6
    move.l %a6,%usp
    moveq #VDP_INITIAL_REGS_MINUS1,%d1
VDPInitLoop:
    move.b (%a5)+,%d5
    move.w %d5,(%a4)
    add.w %d7,%d5
    dbra %d1,VDPInitLoop

    | Select the initial VDP destination, then acquire and initialize the Z80.
    move.l (%a5)+,(%a4)
    move.w %d0,(%a3)
    move.w %d7,(%a1)
    move.w %d7,(%a2)
WaitForZ80:
    btst %d0,(%a1)
    bne.s WaitForZ80
    moveq #Z80_BOOT_BYTES_MINUS1,%d2
Z80InitLoop:
    move.b (%a5)+,(%a0)+
    dbra %d2,Z80InitLoop
    move.w %d0,(%a2)
    move.w %d0,(%a1)
    move.w %d7,(%a2)

    | Clear the mirrored work RAM backwards from address zero.
ClearRAMLoop:
    move.l %d0,-(%a6)
    dbra %d6,ClearRAMLoop

    | Clear CRAM and VSRAM using the following setup commands.
    move.l (%a5)+,(%a4)
    move.l (%a5)+,(%a4)
    moveq #CRAM_LONGS_MINUS1,%d3
ClearCRAMLoop:
    move.l %d0,(%a3)
    dbra %d3,ClearCRAMLoop
    move.l (%a5)+,(%a4)
    moveq #VSRAM_LONGS_MINUS1,%d4
ClearVDPStuff:
    move.l %d0,(%a3)
    dbra %d4,ClearVDPStuff

    | Silence PSG channels, assert reset, clear registers, and mask interrupts.
    moveq #PSG_INITIAL_BYTES_MINUS1,%d5
PSGInitLoop:
    move.b (%a5)+,(PSG_FROM_VDP_DATA,%a3)
    dbra %d5,PSGInitLoop
    move.w %d0,(%a2)
    movem.l (%a6),%d0-%d7/%a0-%a6
    move.w #SR_RESET_MASKED,%sr
PortC_OK:
    | GAS range-checks an unresolved BRA.S against zero before linking.
    | Keep BRA as an instruction and attach its external PC8 target explicitly.
    | The displacement byte lies one byte before the instruction's PC base.
    .reloc .+1, R_68K_PC8, GameProgram-1
    bra.s .
.size _start, .-_start
