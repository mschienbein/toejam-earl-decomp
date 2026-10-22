| Three-button controller reads, with the original Z80/DAC coordination accesses.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "hardware.inc"

.equ DAC_PAD_READ_STEP, 2

| Read pad 1: TH low supplies Start/A; TH high supplies directions/B/C.
| Invert the active-low result; adjust the Z80 DAC delay counter and mirror PSG-active status.
.globl GetCtrl1Input
.type GetCtrl1Input, @function
GetCtrl1Input:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.b #PAD_TH,(IO_CONTROL_1).l
    move.b #0x0,(IO_DATA_1).l
    move.w #PAD_READ_DELAY,%d1
ReadPad1Input_LoopA:
    dbra %d1,ReadPad1Input_LoopA
    moveq #0,%d0
    move.b (IO_DATA_1).l,%d0
    lsl.b #0x2,%d0
    andi.b #PAD_START_A_MASK,%d0
    move.b #PAD_TH,(IO_DATA_1).l
    move.w #PAD_READ_DELAY,%d1
ReadPad1Input_LoopB:
    dbra %d1,ReadPad1Input_LoopB
    move.b (IO_DATA_1).l,%d1
    andi.b #PAD_UDLR_BC_MASK,%d1
    or.b %d1,%d0
    not.b %d0
    subq.b #DAC_PAD_READ_STEP,(Z80_DAC_DELAY).l
    move.b (Z80_PSG_ACTIVE).l,(gPsgEffectStatus).l
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    rts
.size GetCtrl1Input, .-GetCtrl1Input

| Read pad 2 using the same TH sequence and sound-driver coordination as pad 1.
.globl GetCtrl2Input
.type GetCtrl2Input, @function
GetCtrl2Input:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    move.b #PAD_TH,(IO_CONTROL_2).l
    move.b #0x0,(IO_DATA_2).l
    move.w #PAD_READ_DELAY,%d1
ReadPad2Input_LoopA:
    dbra %d1,ReadPad2Input_LoopA
    moveq #0,%d0
    move.b (IO_DATA_2).l,%d0
    lsl.b #0x2,%d0
    andi.b #PAD_START_A_MASK,%d0
    move.b #PAD_TH,(IO_DATA_2).l
    move.w #PAD_READ_DELAY,%d1
ReadPad2Input_LoopB:
    dbra %d1,ReadPad2Input_LoopB
    move.b (IO_DATA_2).l,%d1
    andi.b #PAD_UDLR_BC_MASK,%d1
    or.b %d1,%d0
    not.b %d0
    subq.b #DAC_PAD_READ_STEP,(Z80_DAC_DELAY).l
    move.b (Z80_PSG_ACTIVE).l,(gPsgEffectStatus).l
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    rts
.size GetCtrl2Input, .-GetCtrl2Input
