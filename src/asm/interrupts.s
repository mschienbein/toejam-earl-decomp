| Game-program startup, interrupt masking, exception wrappers, and software reset.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "hardware.inc"

.equ SR_INTERRUPT_MASK, 0x0700
.equ SR_INTERRUPT_UNMASK, 0xf8ff
.equ WORK_RAM_LONGS_MINUS1, 0x3fff

| Startup continuation: read VDP status, raise the SR interrupt-priority mask,
| clear all 64 KiB of work RAM, and enter VDPInitialisation.
.globl GameProgram
.type GameProgram, @function
GameProgram:
    tst.w (VDP_CONTROL).l
    jsr (DisableAllInterrupts,%pc)
    | ASM68K kept the six-byte reservation for this forward same-file call:
    | a four-byte PC-relative JSR followed by this explicit two-byte NOP.
    nop
InitialiseTheMegadrive_InitialiseRAM:
    movea.l #gWorkRamStart,%a0
    move.w #WORK_RAM_LONGS_MINUS1,%d0
GameClearRAMLoop:
    move.l #0,(%a0)+
    dbra %d0,GameClearRAMLoop
    jsr (VDPInitialisation).l
.size GameProgram, .-GameProgram

| Set SR's interrupt-priority mask to 7 without changing other bits.
| Level 7 interrupts remain non-maskable.
.globl DisableAllInterrupts
.type DisableAllInterrupts, @function
DisableAllInterrupts:
    ori #SR_INTERRUPT_MASK,%sr
    rts
.size DisableAllInterrupts, .-DisableAllInterrupts

| Clear only the SR interrupt-priority bits.
.globl EnableAllInterrupts
.type EnableAllInterrupts, @function
EnableAllInterrupts:
    andi #SR_INTERRUPT_UNMASK,%sr
    rts
.size EnableAllInterrupts, .-EnableAllInterrupts

| VBlank exception wrapper: preserve every data/address register before calling C.
.globl Level6Isr
.type Level6Isr, @function
Level6Isr:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    jsr (HandleVBlank).l
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte
.size Level6Isr, .-Level6Isr

| HBlank exception wrapper: preserve the C-call volatile registers.
.globl Level4Isr
.type Level4Isr, @function
Level4Isr:
    movem.l %d0-%d1/%a0-%a1,-(%sp)
    jsr (HandleHBlank).l
    movem.l (%sp)+,%d0-%d1/%a0-%a1
    rte
.size Level4Isr, .-Level4Isr

| Software reset: mask interrupts, reload the cartridge's initial SP, and re-enter _start.
.globl RestartFromResetEntry
.type RestartFromResetEntry, @function
RestartFromResetEntry:
    jsr (DisableAllInterrupts,%pc)
    movea.l (StartOfRom).w,%sp
    jmp (_start,%pc)
.size RestartFromResetEntry, .-RestartFromResetEntry
