| 68000-to-VDP DMA command construction.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "hardware.inc"

.equ VDP_DMA_SOURCE_MID_SHIFT, 9
.equ VDP_DMA_SOURCE_HI_SHIFT, 17
.equ VDP_ADDRESS_HIGH_SHIFT, 14
.equ VDP_ADDRESS_LOW_SHIFT, 16

| Arguments: source address, VRAM byte address, DMA length in words (all stack longs).
| Program the mode-2 shadow, DMA count/source registers, then issue the split VRAM command.
.globl VDPWritesRoutine1
.type VDPWritesRoutine1, @function
VDPWritesRoutine1:
    movem.l %d2-%d4,-(%sp)
    move.l (0x10,%sp),%d3
    move.l (0x14,%sp),%d2
    move.l (0x18,%sp),%d4
    moveq #0,%d0
    move.b (gMode2RegSoftcopy).l,%d0
    moveq #VDP_MODE2_DMA_ENABLE,%d1
    or.l %d1,%d0
    addi.l #VDP_REGISTER_MODE2,%d0
    move.w %d0,(VDP_CONTROL).l

    | Set the low and high DMA word-count registers.
    move.l %d4,%d0
    andi.l #VDP_REGISTER_BYTE_MASK,%d0
    addi.l #VDP_DMA_LENGTH_LO,%d0
    move.w %d0,(VDP_CONTROL).l
    move.l %d4,%d0
    asr.l #0x8,%d0
    andi.l #VDP_REGISTER_BYTE_MASK,%d0
    addi.l #VDP_DMA_LENGTH_HI,%d0
    move.w %d0,(VDP_CONTROL).l

    | Convert the source byte address into the three DMA word-address registers.
    move.l %d3,%d0
    asr.l #0x1,%d0
    andi.l #VDP_REGISTER_BYTE_MASK,%d0
    addi.l #VDP_DMA_SOURCE_LO,%d0
    move.w %d0,(VDP_CONTROL).l
    move.l %d3,%d0
    moveq #VDP_DMA_SOURCE_MID_SHIFT,%d1
    asr.l %d1,%d0
    andi.l #VDP_REGISTER_BYTE_MASK,%d0
    addi.l #VDP_DMA_SOURCE_MID,%d0
    move.w %d0,(VDP_CONTROL).l
    move.l %d3,%d0
    moveq #VDP_DMA_SOURCE_HI_SHIFT,%d1
    asr.l %d1,%d0
    moveq #VDP_DMA_SOURCE_HI_MASK,%d1
    and.l %d1,%d0
    addi.l #VDP_DMA_SOURCE_HI,%d0
    move.w %d0,(VDP_CONTROL).l

    | Pack the VRAM address and DMA-start bit, then write command halves with the original NOPs.
    move.l %d2,%d0
    andi.l #VDP_ADDRESS_HIGH_MASK,%d0
    moveq #VDP_ADDRESS_HIGH_SHIFT,%d1
    asr.l %d1,%d0
    move.l %d2,%d1
    andi.l #VDP_ADDRESS_LOW_MASK,%d1
    moveq #VDP_ADDRESS_LOW_SHIFT,%d2
    asl.l %d2,%d1
    addi.l #VDP_DMA_VRAM_WRITE,%d1
    add.l %d1,%d0
    move.l %d0,(USED_BY_VDP_1).l
    move.w (USED_BY_VDP_1).l,(VDP_CONTROL).l
    nop
    move.w (USED_BY_VDP_2).l,(VDP_CONTROL).l
    nop
    movem.l (%sp)+,%d2-%d4
    rts
.size VDPWritesRoutine1, .-VDPWritesRoutine1
