| Stock Microtec MCC68K 3.3K runtime modules ABS, IDIV, IMUL, and UDIV.
| Independently restated from decoded ROM behavior under ruling R007.
.text

| These modules were identified byte for byte in the 3.3K IEEE-695 libraries.
| ABS takes its argument on the stack. Other entries take d0/d1, return d0,
| preserve d2, and use a0/a1 as scratch. ulmodt and uldivt share UDIV's body.
.equ DIVU_WORD_MAX, 0xffff
.equ DIV_RESTORING_BITS_MINUS1, 15
.equ UNSIGNED_RETURN_REMAINDER, 0
.equ UNSIGNED_RETURN_QUOTIENT, 1

| ABS module: stack long -> d0; negate only when the sign bit is set.
.globl abs
.type abs, @function
abs:
    move.l (0x4,%sp),%d0
    bpl.s ValueIsPositive
    neg.l %d0
ValueIsPositive:
    rts
.size abs, .-abs

| IDIV module, signed remainder: d0 dividend, d1 divisor -> d0 remainder.
| Remember the dividend sign, divide magnitudes, and apply that sign to the remainder.
.globl lmodt
.type lmodt, @function
lmodt:
    move.l %d2,-(%sp)
    move.l %d1,%d2
    bge.s .L_mod_abs_divisor
    neg.l %d2
.L_mod_abs_divisor:
    move.l %d0,%d1
    moveq #0,%d0
    tst.l %d1
    bge.s .L_mod_unsigned
    neg.l %d1
    not.l %d0

    | Divisors fitting sixteen bits use two native DIVU operations.
.L_mod_unsigned:
    movea.l %d0,%a0
    cmpi.l #DIVU_WORD_MAX,%d2
    bhi.s .L_mod_large_divisor
    movea.w %d1,%a1
    clr.w %d1
    swap %d1
    divu.w %d2,%d1
    move.l %d1,%d0
    swap %d1
    move.w %a1,%d0
    divu.w %d2,%d0
    move.w %d0,%d1
    clr.w %d0
    swap %d0
    bra.s .L_mod_apply_sign

    | Larger divisors use sixteen restoring-division bit steps.
.L_mod_large_divisor:
    move.l %d1,%d0
    clr.w %d0
    swap %d0
    swap %d1
    clr.w %d1
    movea.l %d2,%a1
    moveq #DIV_RESTORING_BITS_MINUS1,%d2
.L_mod_shift_bit:
    add.l %d1,%d1
    addx.l %d0,%d0
    cmpa.l %d0,%a1
    bhi.s .L_mod_next_bit
    sub.l %a1,%d0
    addq.w #0x1,%d1
.L_mod_next_bit:
    dbra %d2,.L_mod_shift_bit

    | A remainder has the original dividend's sign.
.L_mod_apply_sign:
    move.w %a0,%d2
    beq.s MathMod32_Return
    neg.l %d0
MathMod32_Return:
    move.l (%sp)+,%d2
    rts
.size lmodt, .-lmodt

| IDIV module, signed quotient: d0 dividend, d1 divisor -> d0 quotient.
| XOR operand signs while taking magnitudes, then restore the quotient sign.
.globl ldivt
.type ldivt, @function
ldivt:
    move.l %d2,-(%sp)
    movea.l %d0,%a0
    moveq #0,%d0
    move.l %d1,%d2
    bge.s .L_div_abs_dividend
    neg.l %d2
    not.l %d0
.L_div_abs_dividend:
    move.l %a0,%d1
    bge.s .L_div_unsigned
    neg.l %d1
    not.l %d0

    | Divisors fitting sixteen bits use the native two-halfword path.
.L_div_unsigned:
    movea.l %d0,%a0
    cmpi.l #DIVU_WORD_MAX,%d2
    bhi.s .L_div_large_divisor
    movea.w %d1,%a1
    clr.w %d1
    swap %d1
    divu.w %d2,%d1
    move.l %d1,%d0
    swap %d1
    move.w %a1,%d0
    divu.w %d2,%d0
    move.w %d0,%d1
    clr.w %d0
    swap %d0
    bra.s .L_div_apply_sign

    | Use restoring division when the divisor is wider than a word.
.L_div_large_divisor:
    move.l %d1,%d0
    clr.w %d0
    swap %d0
    swap %d1
    clr.w %d1
    movea.l %d2,%a1
    moveq #DIV_RESTORING_BITS_MINUS1,%d2
.L_div_shift_bit:
    add.l %d1,%d1
    addx.l %d0,%d0
    cmpa.l %d0,%a1
    bhi.s .L_div_next_bit
    sub.l %a1,%d0
    addq.w #0x1,%d1
.L_div_next_bit:
    dbra %d2,.L_div_shift_bit

    | Select the quotient and restore the combined operand sign.
.L_div_apply_sign:
    move.l %a0,%d2
    beq.s .L_div_return_quotient
    neg.l %d1
.L_div_return_quotient:
    move.l %d1,%d0
    move.l (%sp)+,%d2
    rts
.size ldivt, .-ldivt

| IMUL module: multiply the low halves and the two cross-products, retaining the low 32 bits.
.globl lmult
.type lmult, @function
lmult:
    movea.l %d2,%a1
    move.l %d0,%d2
    mulu.w %d1,%d2
    movea.l %d2,%a0
    move.l %d0,%d2
    swap %d2
    tst.w %d2
    beq.s .L_mul_high_rhs
    mulu.w %d1,%d2

    | Form the other cross-product only if the high multiplier half is nonzero.
.L_mul_high_rhs:
    swap %d1
    tst.w %d1
    beq.s .L_mul_zero_cross
    mulu.w %d1,%d0
    bra.s .L_mul_combine
.L_mul_zero_cross:
    clr.w %d0

    | Shift the cross-product sum into the upper half and add the low-half product.
.L_mul_combine:
    add.l %d2,%d0
    swap %d0
    clr.w %d0
    adda.l %d0,%a0
    move.l %a0,%d0
    move.l %a1,%d2
    rts
.size lmult, .-lmult

| UDIV remainder entry: select d0 from the shared unsigned divide/remainder body.
.globl ulmodt
.type ulmodt, @function
ulmodt:
    movea.l #UNSIGNED_RETURN_REMAINDER,%a0
    bra.s .L_unsigned_divmod
.size ulmodt, .-ulmodt

| UDIV quotient entry: select the computed quotient from the same body.
.globl uldivt
.type uldivt, @function
uldivt:
    movea.l #UNSIGNED_RETURN_QUOTIENT,%a0

    | Shared UDIV body: d0 dividend, d1 divisor, a0 result selector.
.L_unsigned_divmod:
    move.l %d2,-(%sp)
    move.l %d1,%d2
    move.l %d0,%d1
    cmpi.l #DIVU_WORD_MAX,%d2
    bhi.s .L_unsigned_large_divisor
    movea.w %d1,%a1
    clr.w %d1
    swap %d1
    divu.w %d2,%d1
    move.l %d1,%d0
    swap %d1
    move.w %a1,%d0
    divu.w %d2,%d0
    move.w %d0,%d1
    clr.w %d0
    swap %d0
    bra.s .L_unsigned_select_result

    | Sixteen restoring steps compute both quotient and remainder.
.L_unsigned_large_divisor:
    move.l %d1,%d0
    clr.w %d0
    swap %d0
    swap %d1
    clr.w %d1
    movea.l %d2,%a1
    moveq #DIV_RESTORING_BITS_MINUS1,%d2
.L_unsigned_shift_bit:
    add.l %d1,%d1
    addx.l %d0,%d0
    cmpa.l %d0,%a1
    bhi.s .L_unsigned_next_bit
    sub.l %a1,%d0
    addq.w #0x1,%d1
.L_unsigned_next_bit:
    dbra %d2,.L_unsigned_shift_bit

    | a0 == 0 selects remainder; a0 != 0 selects quotient.
.L_unsigned_select_result:
    cmpa.l #0x0,%a0
    beq.s .L_unsigned_restore
    move.l %d1,%d0
.L_unsigned_restore:
    move.l (%sp)+,%d2
    rts
.size uldivt, .-uldivt
