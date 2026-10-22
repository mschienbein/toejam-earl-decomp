| VDP memory clearing, port fills, rectangular copies, and palette upload.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "hardware.inc"

| DBRA loop constants retain the original inclusive counts.
.equ VRAM_WORDS_MINUS1, 0x7fff
.equ VSRAM_WORDS_MINUS1, 39
.equ CRAM_WORDS_MINUS1, 63
.equ PALETTE_COLORS_MINUS1, 15
.equ PALETTE_SLOT_MASK, 3
.equ PALETTE_BYTE_SHIFT, 5
.equ CRAM_WRITE_HIGH_WORD, 0xc000
.equ TILE_VRAM_ADDRESS_SHIFT, 7
.equ TILE_LONG_COUNT_SHIFT, 3

| Clear all 32,768 VRAM words using the existing VDP auto-increment setting.
.globl ClearVideoRAM
.type ClearVideoRAM, @function
ClearVideoRAM:
    move.l #VDP_VRAM_WRITE,(VDP_CONTROL).l
    move.w #VRAM_WORDS_MINUS1,%d0
ClearVideoRAM_Loop:
    move.w #0x0,(VDP_DATA).l
    dbra %d0,ClearVideoRAM_Loop
    rts
.size ClearVideoRAM, .-ClearVideoRAM

| Clear all 40 VSRAM vertical-scroll words.
.globl ClearVerticalScrollRam
.type ClearVerticalScrollRam, @function
ClearVerticalScrollRam:
    move.l #VDP_VSRAM_WRITE,(VDP_CONTROL).l
    move.w #VSRAM_WORDS_MINUS1,%d0
ClearVerticalScrollRam_Loop:
    move.w #0x0,(VDP_DATA).l
    dbra %d0,ClearVerticalScrollRam_Loop
    rts
.size ClearVerticalScrollRam, .-ClearVerticalScrollRam

| Clear all 64 CRAM color words.
.globl ClearColourRAM
.type ClearColourRAM, @function
ClearColourRAM:
    move.l #VDP_CRAM_WRITE,(VDP_CONTROL).l
    move.w #CRAM_WORDS_MINUS1,%d0
ClearColourRAM_Loop:
    move.w #0x0,(VDP_DATA).l
    dbra %d0,ClearColourRAM_Loop
    rts
.size ClearColourRAM, .-ClearColourRAM

| Arguments: source, tile-index destination, tile count (stack longs).
| Pack the tile-index address into a VRAM write command and transfer longwords.
| Preserve the original inclusive counter: this copies (argument3 * 8) + 1 longwords.
.globl CopyLongwordRunToVramTile
.type CopyLongwordRunToVramTile, @function
CopyLongwordRunToVramTile:
    move.l (0x8,%sp),%d0
    lsl.l #TILE_VRAM_ADDRESS_SHIFT,%d0
    lsr.w #0x2,%d0
    swap %d0
    bset #VDP_VRAM_WRITE_BIT,%d0
    move.l %d0,(VDP_CONTROL).l
    move.l (0xc,%sp),%d1
    lsl.l #TILE_LONG_COUNT_SHIFT,%d1
    movea.l (0x4,%sp),%a0
.L_copy_tile_longs:
    move.l (%a0)+,(VDP_DATA).l
    dbra %d1,.L_copy_tile_longs
    rts
.size CopyLongwordRunToVramTile, .-CopyLongwordRunToVramTile

| Fill the current VDP destination with the low word of value, count times.
.globl FillVdpDataWords
.type FillVdpDataWords, @function
FillVdpDataWords:
    move.l (0x4,%sp),%d0
    move.l (0x8,%sp),%d1
    subq.w #0x1,%d1
.L_fill_words:
    move.w %d0,(VDP_DATA).l
    dbra %d1,.L_fill_words
    rts
.size FillVdpDataWords, .-FillVdpDataWords

| Fill the current VDP destination using byte-sized writes, count times.
.globl FillVdpDataBytes
.type FillVdpDataBytes, @function
FillVdpDataBytes:
    move.l (0x4,%sp),%d0
    move.l (0x8,%sp),%d1
    subq.w #0x1,%d1
.L_fill_bytes:
    move.b %d0,(VDP_DATA).l
    dbra %d1,.L_fill_bytes
    rts
.size FillVdpDataBytes, .-FillVdpDataBytes

| Copy count source words to the current VDP destination.
.globl MoveDataToVRAM
.type MoveDataToVRAM, @function
MoveDataToVRAM:
    movea.l (0x4,%sp),%a0
    move.l (0x8,%sp),%d0
    subq.w #0x1,%d0
MoveDataToVRAMLoop:
    move.w (%a0)+,(VDP_DATA).l
    dbra %d0,MoveDataToVRAMLoop
    rts
.size MoveDataToVRAM, .-MoveDataToVRAM

| Arguments: destination byte address, source, words per row, row count, row stride in words.
| Issue one VRAM command per row and copy consecutive source words.
.globl UploadVramWordRectangle
.type UploadVramWordRectangle, @function
UploadVramWordRectangle:
    link %a6,#0
    move.l %d2,-(%sp)
    movea.l (0x8,%a6),%a1
    movea.l (0xc,%a6),%a0
    move.l (0x14,%a6),%d2
    subq.w #0x1,%d2
    move.l (0x18,%a6),%d1
    asl.l #0x1,%d1

    | Pack this row's byte address, then advance the next row destination by its stride.
.L_copy_rectangle_row:
    move.l %a1,%d0
    lsl.l #0x2,%d0
    lsr.w #0x2,%d0
    swap %d0
    bset #VDP_VRAM_WRITE_BIT,%d0
    andi.l #VDP_PACKED_ADDRESS_MASK,%d0
    move.l %d0,(VDP_CONTROL).l
    adda.l %d1,%a1

    | Copy the row using an inclusive, predecremented word counter.
    move.l (0x10,%a6),%d0
    subq.w #0x1,%d0
.L_copy_rectangle_word:
    move.w (%a0)+,(VDP_DATA).l
    dbra %d0,.L_copy_rectangle_word
    dbra %d2,.L_copy_rectangle_row
    move.l (%sp)+,%d2
    unlk %a6
    rts
.size UploadVramWordRectangle, .-UploadVramWordRectangle

| Upload sixteen color words into palette slot (argument & 3).
.globl CopyPaletteToCRAMSlot
.type CopyPaletteToCRAMSlot, @function
CopyPaletteToCRAMSlot:
    movea.l (0x4,%sp),%a0
    move.l (0x8,%sp),%d0
    andi.l #PALETTE_SLOT_MASK,%d0
    lsl.l #PALETTE_BYTE_SHIFT,%d0
    ori.w #CRAM_WRITE_HIGH_WORD,%d0
    swap %d0
    move.l %d0,(VDP_CONTROL).l
    move.w #PALETTE_COLORS_MINUS1,%d0
CopyPaletteEntry:
    move.w (%a0)+,(VDP_DATA).l
    dbra %d0,CopyPaletteEntry
    rts
.size CopyPaletteToCRAMSlot, .-CopyPaletteToCRAMSlot
