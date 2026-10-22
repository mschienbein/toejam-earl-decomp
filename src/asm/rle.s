| Sprite graphics unpacking into the shared staging buffer.
| Independently restated from decoded ROM behavior under ruling R007.
.text

| Token bits and counts are part of the sprite asset compression format.
.equ SPRITE_DATA_POINTER, 6
.equ TILE_BYTE_SHIFT, 5
.equ TILE_BYTES_PER_LANE_SHIFT, 3
.equ RLE_LANES, 4
.equ RLE_KIND_BIT, 6
.equ RLE_ALTERNATING_BIT, 5
.equ RLE_ZERO_TOKEN_BIAS, 0x40
.equ RLE_SHORT_COUNT_MASK, 0x1f
.equ RLE_COUNT_MASK, 0x3f
.equ RLE_ALL_PIXELS, 0xff
.equ RLE_NIBBLE_SHIFT, 4

| Arguments: render-piece pointer and tile count. Decode four independent byte lanes.
| Each lane produces eight bytes per tile at a four-byte stride in ram_ffb8da.
.globl SpriteUnpackInterleavedRLE
.type SpriteUnpackInterleavedRLE, @function
SpriteUnpackInterleavedRLE:
    link %a6,#-4
    movem.l %d2-%d6/%a2,-(%sp)
    movea.l (0x8,%a6),%a0
    move.w (0xe,%a6),%d2
    asl.w #TILE_BYTES_PER_LANE_SHIFT,%d2
    movea.l (SPRITE_DATA_POINTER,%a0),%a1
    movea.l #ram_ffb8da,%a0
    adda.w (SPRITE_STATE_TABLE).l,%a0
    lea (0x1,%a0),%a2
    clr.w %d0

    | Begin another of the four interleaved output lanes.
OuterLoopStart:
    move.w %d2,%d1
    bra.w OuterLoopCondition

    | Read and sign-extend the next run token.
InnerLoopStart:
    move.b (%a1)+,%d4
    ext.w %d4
    tst.b %d4
    bge.s TokenGE00
    btst #RLE_KIND_BIT,%d4
    beq.s TokenBetween80AndBF

    | 0xc0..0xff: repeat the following byte -token times.
    move.b (%a1)+,%d3
    bra.s TokenBetweenC0AndFF

    | 0x80..0xbf: repeat zero; setting bit 6 converts the token to a negative count.
TokenBetween80AndBF:
    clr.b %d3
    ori.b #RLE_ZERO_TOKEN_BIAS,%d4
TokenBetweenC0AndFF:
    neg.w %d4
    move.w %d4,%d6
    subq.w #0x1,%d6
CopyLoop1:
    move.b %d3,(%a0)
    addq.l #RLE_LANES,%a0
    dbra %d6,CopyLoop1
    bra.w InnerLoopCondition

    | For nonnegative tokens, bit 6 distinguishes literals from compact fill modes.
TokenGE00:
    btst #RLE_KIND_BIT,%d4
    beq.w TokenBetween00And3F
    btst #RLE_ALTERNATING_BIT,%d4
    beq.s TokenBetween60And7F

    | Bit 5 set (0x60..0x7f): alternate a byte with its nibble-swapped copy.
    | The canonical TokenBetween40And5F label is historical; the bit test selects 0x60..0x7f here.
TokenBetween40And5F:
    move.b (%a1)+,%d3
    move.b %d3,%d5
    rol.b #RLE_NIBBLE_SHIFT,%d5
    andi.w #RLE_SHORT_COUNT_MASK,%d4
    move.w %d4,%d6
    lsr.w #0x1,%d6
    subq.w #0x1,%d6
CopyLoop4:
    move.b %d3,(%a0)
    addq.l #RLE_LANES,%a0
    move.b %d5,(%a0)
    addq.l #RLE_LANES,%a0
    dbra %d6,CopyLoop4
    btst #0x0,%d4
    beq.s EndCopyLoop4
    move.b %d3,(%a0)
    addq.l #RLE_LANES,%a0
EndCopyLoop4:
    bra.s .L_interleaved_run_done

    | Bit 5 clear (0x40..0x5f): emit 0xff for the low-five-bit count.
    | The canonical TokenBetween60And7F label is historical; this is the 0x40..0x5f path.
TokenBetween60And7F:
    move.b #RLE_ALL_PIXELS,%d3
    andi.w #RLE_SHORT_COUNT_MASK,%d4
    move.w %d4,%d6
    subq.w #0x1,%d6
CopyLoop2:
    move.b %d3,(%a0)
    addq.l #RLE_LANES,%a0
    dbra %d6,CopyLoop2
.L_interleaved_run_done:
    bra.s InnerLoopCondition

    | 0x00..0x3f: copy literal source bytes at the four-byte output stride.
TokenBetween00And3F:
    move.w %d4,%d6
    subq.w #0x1,%d6
CopyLoop3:
    move.b (%a1)+,(%a0)
    addq.l #RLE_LANES,%a0
    dbra %d6,CopyLoop3

    | Subtract the run length, then advance the lane start after its byte quota is exhausted.
InnerLoopCondition:
    sub.w %d4,%d1
OuterLoopCondition:
    tst.w %d1
    bgt.w InnerLoopStart
    movea.l %a2,%a0
    addq.l #0x1,%a2
    addq.w #0x1,%d0
    cmpi.w #RLE_LANES,%d0
    blt.w OuterLoopStart
    movem.l (-0x1c,%a6),%d2-%d6/%a2
    unlk %a6
    rts
.size SpriteUnpackInterleavedRLE, .-SpriteUnpackInterleavedRLE

| Arguments: render-piece pointer and tile count. Expand a contiguous simple-RLE stream.
| Negative tokens repeat a byte or zero; positive tokens select literals or 0xff runs.
.globl SpriteUnpackRLE
.type SpriteUnpackRLE, @function
SpriteUnpackRLE:
    movem.l %d2-%d4,-(%sp)
    movea.l (0x10,%sp),%a0
    move.w (0x16,%sp),%d4
    movea.l (SPRITE_DATA_POINTER,%a0),%a1
    move.w (SPRITE_STATE_TABLE).l,%d1
    movea.l #ram_ffb8da,%a0
    adda.w %d1,%a0
    asl.w #TILE_BYTE_SHIFT,%d4
    bra.w .L_rle_remaining

    | Read the next simple-RLE token.
.L_rle_read_token:
    move.b (%a1)+,%d1
    ext.w %d1
    tst.w %d1
    bge.s SpriteUnpackRLE_TokenGE00
    btst #RLE_KIND_BIT,%d1
    beq.s SpriteUnpackRLE_TokenBetween80AndBF
    move.b (%a1)+,%d2
    bra.s SpriteUnpackRLE_TokenBetweenC0AndFF

    | Zero run: force the sign/count bits to the same representation as a byte-repeat run.
SpriteUnpackRLE_TokenBetween80AndBF:
    clr.b %d2
    ori.b #RLE_ZERO_TOKEN_BIAS,%d1
SpriteUnpackRLE_TokenBetweenC0AndFF:
    neg.w %d1
    move.w %d1,%d3
    subq.w #0x1,%d3
SpriteUnpackRLE_CopyLoop1:
    move.b %d2,(%a0)+
    dbra %d3,SpriteUnpackRLE_CopyLoop1
    bra.s .L_rle_run_done

    | Positive token: bit 6 selects an all-ones fill rather than literals.
SpriteUnpackRLE_TokenGE00:
    btst #RLE_KIND_BIT,%d1
    beq.s SpriteUnpackRLE_TokenBetween00And3F
SpriteUnpackRLE_TokenBetween40And7F:
    move.b #RLE_ALL_PIXELS,%d2
    andi.w #RLE_COUNT_MASK,%d1
    move.w %d1,%d3
    subq.w #0x1,%d3
SpriteUnpackRLE_CopyLoop2:
    move.b %d2,(%a0)+
    dbra %d3,SpriteUnpackRLE_CopyLoop2
    bra.s .L_rle_run_done

    | Copy the requested number of literal bytes.
SpriteUnpackRLE_TokenBetween00And3F:
    move.w %d1,%d3
    subq.w #0x1,%d3
SpriteUnpackRLE_CopyDirect:
    move.b (%a1)+,(%a0)+
    dbra %d3,SpriteUnpackRLE_CopyDirect

    | Account for the expanded run; stop after tile_count * 32 bytes.
.L_rle_run_done:
    sub.w %d1,%d4
.L_rle_remaining:
    tst.w %d4
    bgt.s .L_rle_read_token
    movem.l (%sp)+,%d2-%d4
    rts
.size SpriteUnpackRLE, .-SpriteUnpackRLE

| Copy 32 bytes (eight longwords) per tile from render_piece.data into the staging buffer.
| The canonical loop name says Copy8Bytes, but each iteration actually copies eight longs.
.globl SpriteUnpackRaw
.type SpriteUnpackRaw, @function
SpriteUnpackRaw:
    movea.l (0x4,%sp),%a0
    move.w (0xa,%sp),%d0
    movea.l (SPRITE_DATA_POINTER,%a0),%a1
    move.w (SPRITE_STATE_TABLE).l,%d1
    movea.l #ram_ffb8da,%a0
    adda.w %d1,%a0
    subq.w #0x1,%d0
SpriteUnpackRaw_Copy8Bytes:
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    move.l (%a1)+,(%a0)+
    dbra %d0,SpriteUnpackRaw_Copy8Bytes
    rts
.size SpriteUnpackRaw, .-SpriteUnpackRaw
