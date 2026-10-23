| Collision queries that the pinned MCC68K 3.3K cannot reproduce from plain C
| (ruling R011): they are compiler-shaped, but their twelve SUB.W/BPL/NEG.W sign
| tests appear nowhere in the matched C corpus or in the plain-C probes reviewed
| for R011; whether another compiler release, inline assembly or hand editing
| produced them is unresolved. Kept as readable 68000 source; not credited as
| matching C.
| All five walk gEntityDrawListHead, apply their own eligibility/bounds tests,
| and return the first matching entity index plus one, or zero for no match.
| The broad phase uses the querying entity's current Y, including when a
| proposed Y is supplied. Preserve the original word arithmetic and N-only
| SUB.W/BPL/NEG.W magnitude tests; replacing BPL with BGE changes overflow cases.
.text

| object_state offsets from include/entities.h (128-byte, two-byte-aligned record).
.equ OBJ_X, 0x00
.equ OBJ_Y, 0x02
.equ OBJ_HEIGHT, 0x04
.equ OBJ_WIDTH, 0x29
.equ OBJ_HEIGHT_PIXELS, 0x2a
.equ OBJ_DRAW_FLAGS, 0x2f
.equ OBJ_LEVEL, 0x4c
.equ OBJ_BEHAVIOR_FLAGS, 0x4d
.equ OBJ_NEXT, 0x5a
.equ OBJ_ENTITY_INDEX, 0x5e
.equ ENTITY_ACTIVE_BIT, 0
.equ ENTITY_COLLIDABLE_BIT, 0
.equ PLAYER_EARL, 1

| Collision-specific bounds. Strict versus inclusive comparisons remain explicit.
.equ BROAD_PHASE_Y_PADDING, 13
.equ NARROW_WIDTH_INSET, 10
.equ NARROW_Y_LIMIT, 8
.equ HEIGHT_TOLERANCE, 10
.equ PROJECTILE_WIDTH_INSET, 1
.equ PROJECTILE_Y_LIMIT, 8
.equ PROJECTILE_ABOVE_PADDING, 8
.equ PROJECTILE_BELOW_PADDING, 7

| Arguments: entity, proposed X/Y, and first eligible entity index.
| Exclude self; require active/collidable flags and the same level. Narrow bounds
| use both half-widths minus ten, Y distance < 8, and current height distance <= 10.
.globl EntityFindNarrowCollisionAtPosition
.type EntityFindNarrowCollisionAtPosition, @function
EntityFindNarrowCollisionAtPosition:
    movem.l %d2-%d5/%a2-%a3,-(%sp)
    movea.l (0x1c,%sp),%a2
    move.w (0x22,%sp),%d3
    move.w (0x26,%sp),%d4
    movea.l (gEntityDrawListHead).l,%a3
    bra.w .L_narrow_test

    | Current-Y broad phase: skip above the candidate band, stop below it.
.L_narrow_candidate:
    move.w (OBJ_Y,%a2),%d1
    move.w (OBJ_Y,%a3),%d0
    move.w %d0,%d2
    subi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    blt.w .L_narrow_next
    move.w %d0,%d2
    addi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    bgt.w .L_narrow_none

    | Reject self, indices below the threshold, inactive/noncollidable objects, and other levels.
    cmpa.l %a3,%a2
    beq.w .L_narrow_next
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    cmp.w (0x2a,%sp),%d0
    blt.w .L_narrow_next
    btst #ENTITY_ACTIVE_BIT,(OBJ_DRAW_FLAGS,%a3)
    beq.w .L_narrow_next
    btst #ENTITY_COLLIDABLE_BIT,(OBJ_BEHAVIOR_FLAGS,%a3)
    beq.w .L_narrow_next
    move.b (OBJ_LEVEL,%a3),%d0
    cmp.b (OBJ_LEVEL,%a2),%d0
    bne.w .L_narrow_next

    | Compute combined half-width reach minus the narrow inset.
    moveq #0,%d5
    move.b (OBJ_WIDTH,%a3),%d5
    lsr.w #0x1,%d5
    moveq #0,%d0
    move.b (OBJ_WIDTH,%a2),%d0
    lsr.w #0x1,%d0
    add.w %d0,%d5
    subi.w #NARROW_WIDTH_INSET,%d5

    | Take the wrapped word X difference magnitude using N only.
    move.w %d3,%d0
    move.w (OBJ_X,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_narrow_x_magnitude
    neg.w %d0
.L_narrow_x_magnitude:
    cmp.w %d0,%d5
    ble.s .L_narrow_next

    | Check proposed Y against the target with a strict eight-pixel bound.
    move.w %d4,%d0
    move.w (OBJ_Y,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_narrow_y_magnitude
    neg.w %d0
.L_narrow_y_magnitude:
    cmpi.w #NARROW_Y_LIMIT,%d0
    bge.s .L_narrow_next

    | Check current elevation with an inclusive ten-pixel tolerance.
    move.w (OBJ_HEIGHT,%a2),%d2
    move.w (OBJ_HEIGHT,%a3),%d0
    sub.w %d0,%d2
    bpl.s .L_narrow_height_magnitude
    neg.w %d2
.L_narrow_height_magnitude:
    cmpi.w #HEIGHT_TOLERANCE,%d2
    bgt.s .L_narrow_next

    | Return the signed-byte entity index plus one.
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    ext.l %d0
    addq.l #0x1,%d0
    bra.s .L_narrow_return

    | Advance through the draw-list link; zero terminates the scan.
.L_narrow_next:
    movea.l (OBJ_NEXT,%a3),%a3
.L_narrow_test:
    move.l %a3,%d0
    bne.w .L_narrow_candidate
.L_narrow_none:
    moveq #0,%d0
.L_narrow_return:
    movem.l (%sp)+,%d2-%d5/%a2-%a3
    rts
.size EntityFindNarrowCollisionAtPosition, .-EntityFindNarrowCollisionAtPosition

| Arguments: entity and proposed X/Y. Apply the same narrow bounds, but indices
| <= PLAYER_EARL participate only when their indexed gPlayerInventoryOpen byte is nonzero.
.globl EntityFindMenuAwareCollisionAtPosition
.type EntityFindMenuAwareCollisionAtPosition, @function
EntityFindMenuAwareCollisionAtPosition:
    movem.l %d2-%d5/%a2-%a3,-(%sp)
    movea.l (0x1c,%sp),%a2
    move.w (0x22,%sp),%d3
    move.w (0x26,%sp),%d4
    movea.l (gEntityDrawListHead).l,%a3
    bra.w .L_menu_test

    | The broad phase still uses current entity Y, not the proposed Y argument.
.L_menu_candidate:
    move.w (OBJ_Y,%a2),%d1
    move.w (OBJ_Y,%a3),%d0
    move.w %d0,%d2
    subi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    blt.w .L_menu_next
    move.w %d0,%d2
    addi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    bgt.w .L_menu_none

    | Indices <= 1 consult their menu-open byte; other indices bypass this gate.
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    cmpi.b #PLAYER_EARL,%d0
    bgt.s .L_menu_eligible
    ext.w %d0
    movea.l #gPlayerInventoryOpen,%a0
    tst.b (%a0,%d0.w)
    beq.w .L_menu_next

    | Apply self/active/collidable/level eligibility after the menu gate.
.L_menu_eligible:
    cmpa.l %a3,%a2
    beq.w .L_menu_next
    btst #ENTITY_ACTIVE_BIT,(OBJ_DRAW_FLAGS,%a3)
    beq.w .L_menu_next
    btst #ENTITY_COLLIDABLE_BIT,(OBJ_BEHAVIOR_FLAGS,%a3)
    beq.w .L_menu_next
    move.b (OBJ_LEVEL,%a3),%d0
    cmp.b (OBJ_LEVEL,%a2),%d0
    bne.w .L_menu_next

    | Combined half-widths minus ten, then proposed-X magnitude.
    moveq #0,%d5
    move.b (OBJ_WIDTH,%a3),%d5
    lsr.w #0x1,%d5
    moveq #0,%d0
    move.b (OBJ_WIDTH,%a2),%d0
    lsr.w #0x1,%d0
    add.w %d0,%d5
    subi.w #NARROW_WIDTH_INSET,%d5
    move.w %d3,%d0
    sub.w (OBJ_X,%a3),%d0
    bpl.s .L_menu_x_magnitude
    neg.w %d0
.L_menu_x_magnitude:
    cmp.w %d0,%d5
    ble.s .L_menu_next

    | Proposed Y must be closer than eight pixels.
    move.w %d4,%d0
    sub.w (OBJ_Y,%a3),%d0
    bpl.s .L_menu_y_magnitude
    neg.w %d0
.L_menu_y_magnitude:
    cmpi.w #NARROW_Y_LIMIT,%d0
    bge.s .L_menu_next

    | Current elevation distance may equal ten pixels.
    move.w (OBJ_HEIGHT,%a2),%d2
    sub.w (OBJ_HEIGHT,%a3),%d2
    bpl.s .L_menu_height_magnitude
    neg.w %d2
.L_menu_height_magnitude:
    cmpi.w #HEIGHT_TOLERANCE,%d2
    bgt.s .L_menu_next

    | Preserve this query's word-sized increment of the sign-extended index.
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    ext.l %d0
    addq.w #0x1,%d0
    bra.s .L_menu_return

    | Advance to the next draw-list object, or finish with zero.
.L_menu_next:
    movea.l (OBJ_NEXT,%a3),%a3
.L_menu_test:
    move.l %a3,%d0
    bne.w .L_menu_candidate
.L_menu_none:
    moveq #0,%d0
.L_menu_return:
    movem.l (%sp)+,%d2-%d5/%a2-%a3
    rts
.size EntityFindMenuAwareCollisionAtPosition, .-EntityFindMenuAwareCollisionAtPosition

| Arguments: entity, proposed X/Y/height, and first eligible entity index.
| Apply narrow X/Y bounds, then test overlap of the proposed and target height
| intervals. This query sign-extends height_pixels; retain its asymmetric endpoints.
.globl EntityFindVolumeCollisionAtPosition
.type EntityFindVolumeCollisionAtPosition, @function
EntityFindVolumeCollisionAtPosition:
    movem.l %d2-%d5/%a2-%a3,-(%sp)
    movea.l (0x1c,%sp),%a2
    move.w (0x22,%sp),%d3
    move.w (0x26,%sp),%d4
    movea.l (gEntityDrawListHead).l,%a3
    bra.w .L_volume_test

    | Cull by current entity Y before testing the proposed volume.
.L_volume_candidate:
    move.w (OBJ_Y,%a2),%d1
    move.w (OBJ_Y,%a3),%d0
    move.w %d0,%d2
    subi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    blt.w .L_volume_next
    addi.w #BROAD_PHASE_Y_PADDING,%d0
    cmp.w %d0,%d1
    bgt.w .L_volume_none

    | Apply self, minimum-index, active/collidable, and same-level filters.
    cmpa.l %a3,%a2
    beq.w .L_volume_next
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    cmp.w (0x2e,%sp),%d0
    blt.w .L_volume_next
    btst #ENTITY_ACTIVE_BIT,(OBJ_DRAW_FLAGS,%a3)
    beq.w .L_volume_next
    btst #ENTITY_COLLIDABLE_BIT,(OBJ_BEHAVIOR_FLAGS,%a3)
    beq.w .L_volume_next
    move.b (OBJ_LEVEL,%a3),%d0
    cmp.b (OBJ_LEVEL,%a2),%d0
    bne.w .L_volume_next

    | Narrow X reach is the two half-widths minus ten.
    moveq #0,%d5
    move.b (OBJ_WIDTH,%a3),%d5
    lsr.w #0x1,%d5
    moveq #0,%d0
    move.b (OBJ_WIDTH,%a2),%d0
    lsr.w #0x1,%d0
    add.w %d0,%d5
    subi.w #NARROW_WIDTH_INSET,%d5
    move.w %d3,%d0
    move.w (OBJ_X,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_volume_x_magnitude
    neg.w %d0
.L_volume_x_magnitude:
    cmp.w %d0,%d5
    ble.w .L_volume_next

    | Compute proposed-Y magnitude in the low word.
    move.w %d4,%d0
    move.w (OBJ_Y,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_volume_y_magnitude
    neg.w %d0

    | Keep the long comparison of this zero-extended word distance.
.L_volume_y_magnitude:
    cmpi.l #NARROW_Y_LIMIT,%d0
    bge.s .L_volume_next

    | When the target starts at/below proposed height, require proposed height
    | to be strictly below its top; height_pixels is read as a signed byte.
    move.w (0x2a,%sp),%d1
    move.w (OBJ_HEIGHT,%a3),%d0
    cmp.w %d1,%d0
    bgt.s .L_volume_below_target
    move.b (OBJ_HEIGHT_PIXELS,%a3),%d0
    ext.w %d0
    add.w (OBJ_HEIGHT,%a3),%d0
    cmp.w %d0,%d1
    bge.s .L_volume_next

    | A volume hit returns target index plus one.
.L_volume_hit:
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    ext.l %d0
    addq.l #0x1,%d0
    bra.s .L_volume_return

    | Otherwise the querying entity's signed height_pixels extends upward
    | from proposed height; equality with the target base counts as contact.
.L_volume_below_target:
    move.w (OBJ_HEIGHT,%a3),%d1
    move.b (OBJ_HEIGHT_PIXELS,%a2),%d0
    ext.w %d0
    add.w (0x2a,%sp),%d0
    cmp.w %d0,%d1
    bgt.s .L_volume_next
    bra.s .L_volume_hit

    | Continue through OBJ_NEXT until no candidate remains.
.L_volume_next:
    movea.l (OBJ_NEXT,%a3),%a3
.L_volume_test:
    move.l %a3,%d0
    bne.w .L_volume_candidate
.L_volume_none:
    moveq #0,%d0
.L_volume_return:
    movem.l (%sp)+,%d2-%d5/%a2-%a3
    rts
.size EntityFindVolumeCollisionAtPosition, .-EntityFindVolumeCollisionAtPosition

| Arguments: projectile entity and owner index to exclude. Only the target half-width
| minus one defines X reach. Current Y distance may equal eight. Height contact uses
| unsigned target height_pixels and different strict bounds above and below the target.
.globl ProjectileFindCollisionExcludingOwner
.type ProjectileFindCollisionExcludingOwner, @function
ProjectileFindCollisionExcludingOwner:
    movem.l %d2-%d3/%a2-%a3,-(%sp)
    movea.l (0x14,%sp),%a2
    move.w (0x1a,%sp),%d3
    movea.l (gEntityDrawListHead).l,%a3
    bra.w .L_projectile_test

    | Cull by the projectile's current Y using the same broad-phase band.
.L_projectile_candidate:
    move.w (OBJ_Y,%a2),%d1
    move.w (OBJ_Y,%a3),%d2
    move.w %d2,%d0
    subi.w #BROAD_PHASE_Y_PADDING,%d0
    cmp.w %d0,%d1
    blt.w .L_projectile_next
    addi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    bgt.w .L_projectile_none

    | Require collidable, same level, not the supplied owner index, and active.
    | There is no additional self-pointer comparison in this query.
    btst #ENTITY_COLLIDABLE_BIT,(OBJ_BEHAVIOR_FLAGS,%a3)
    beq.w .L_projectile_next
    move.b (OBJ_LEVEL,%a3),%d0
    cmp.b (OBJ_LEVEL,%a2),%d0
    bne.w .L_projectile_next
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    cmp.w %d3,%d0
    beq.w .L_projectile_next
    btst #ENTITY_ACTIVE_BIT,(OBJ_DRAW_FLAGS,%a3)
    beq.w .L_projectile_next

    | Use only target half-width minus one for horizontal reach.
    moveq #0,%d2
    move.b (OBJ_WIDTH,%a3),%d2
    lsr.w #0x1,%d2
    subq.w #PROJECTILE_WIDTH_INSET,%d2
    move.w (OBJ_X,%a2),%d0
    move.w (OBJ_X,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_projectile_x_magnitude
    neg.w %d0
.L_projectile_x_magnitude:
    cmp.w %d0,%d2
    ble.w .L_projectile_next

    | Unlike the narrow queries, Y distance equal to eight is accepted.
    move.w (OBJ_Y,%a2),%d2
    move.w (OBJ_Y,%a3),%d0
    sub.w %d0,%d2
    bpl.s .L_projectile_y_magnitude
    neg.w %d2
.L_projectile_y_magnitude:
    cmpi.w #PROJECTILE_Y_LIMIT,%d2
    bgt.s .L_projectile_next

    | Choose different vertical bounds according to relative base heights.
    move.w (OBJ_HEIGHT,%a2),%d1
    cmp.w (OBJ_HEIGHT,%a3),%d1
    ble.s .L_projectile_at_or_below

    | Above target: zero-extend target height_pixels and add eight pixels;
    | the projectile height must be strictly below that upper bound.
    moveq #0,%d2
    move.b (OBJ_HEIGHT_PIXELS,%a3),%d2
    add.w (OBJ_HEIGHT,%a3),%d2
    addq.w #PROJECTILE_ABOVE_PADDING,%d2
    cmp.w %d2,%d1
    bge.w .L_projectile_next

    | Preserve the original word-sized index increment.
.L_projectile_hit:
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    ext.l %d0
    addq.w #0x1,%d0
    bra.s .L_projectile_return

    | At/below target: projectile height plus seven must exceed target height.
.L_projectile_at_or_below:
    addq.w #PROJECTILE_BELOW_PADDING,%d1
    cmp.w (OBJ_HEIGHT,%a3),%d1
    bgt.s .L_projectile_hit

    | Advance to the next candidate; no hit returns zero.
.L_projectile_next:
    movea.l (OBJ_NEXT,%a3),%a3
.L_projectile_test:
    move.l %a3,%d0
    bne.w .L_projectile_candidate
.L_projectile_none:
    moveq #0,%d0
.L_projectile_return:
    movem.l (%sp)+,%d2-%d3/%a2-%a3
    rts
.size ProjectileFindCollisionExcludingOwner, .-ProjectileFindCollisionExcludingOwner

| Arguments: entity, proposed X, unused Y, and first eligible entity index.
| Use the current-Y broad phase, both half-widths without the narrow inset, and
| current height distance <= 10. The original load of the unused Y argument stays.
.globl EntityFindHorizontalCollisionAtX
.type EntityFindHorizontalCollisionAtX, @function
EntityFindHorizontalCollisionAtX:
    movem.l %d2-%d3/%a2-%a3,-(%sp)
    movea.l (0x14,%sp),%a2
    move.w (0x1a,%sp),%d3

    | This historical parameter is loaded but not used in the collision tests.
    move.w (0x1e,%sp),%d0
    movea.l (gEntityDrawListHead).l,%a3
    bra.w .L_horizontal_test

    | Cull by current Y, without a separate proposed-Y distance test.
.L_horizontal_candidate:
    move.w (OBJ_Y,%a2),%d1
    move.w (OBJ_Y,%a3),%d2
    move.w %d2,%d0
    subi.w #BROAD_PHASE_Y_PADDING,%d0
    cmp.w %d0,%d1
    blt.w .L_horizontal_next
    addi.w #BROAD_PHASE_Y_PADDING,%d2
    cmp.w %d2,%d1
    bgt.w .L_horizontal_none

    | Reject self, noncollidable, other-level, inactive, or below-threshold targets.
    cmpa.l %a3,%a2
    beq.w .L_horizontal_next
    btst #ENTITY_COLLIDABLE_BIT,(OBJ_BEHAVIOR_FLAGS,%a3)
    beq.w .L_horizontal_next
    move.b (OBJ_LEVEL,%a3),%d0
    cmp.b (OBJ_LEVEL,%a2),%d0
    bne.w .L_horizontal_next
    btst #ENTITY_ACTIVE_BIT,(OBJ_DRAW_FLAGS,%a3)
    beq.s .L_horizontal_next
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    cmp.w (0x22,%sp),%d0
    blt.s .L_horizontal_next

    | Use both half-widths with no narrow-query inset.
    moveq #0,%d2
    move.b (OBJ_WIDTH,%a3),%d2
    lsr.w #0x1,%d2
    moveq #0,%d0
    move.b (OBJ_WIDTH,%a2),%d0
    lsr.w #0x1,%d0
    add.w %d0,%d2

    | Test proposed X with the original N-only word-magnitude sequence.
    move.w %d3,%d0
    move.w (OBJ_X,%a3),%d1
    sub.w %d1,%d0
    bpl.s .L_horizontal_x_magnitude
    neg.w %d0
.L_horizontal_x_magnitude:
    cmp.w %d0,%d2
    ble.s .L_horizontal_next

    | Test current elevation with the inclusive ten-pixel tolerance.
    move.w (OBJ_HEIGHT,%a2),%d2
    sub.w (OBJ_HEIGHT,%a3),%d2
    bpl.s .L_horizontal_height_magnitude
    neg.w %d2
.L_horizontal_height_magnitude:
    cmpi.w #HEIGHT_TOLERANCE,%d2
    bgt.s .L_horizontal_next

    | Return entity index plus one using the original word increment.
    move.b (OBJ_ENTITY_INDEX,%a3),%d0
    ext.w %d0
    ext.l %d0
    addq.w #0x1,%d0
    bra.s .L_horizontal_return

    | Advance through OBJ_NEXT and return zero at the end of the list.
.L_horizontal_next:
    movea.l (OBJ_NEXT,%a3),%a3
.L_horizontal_test:
    move.l %a3,%d0
    bne.w .L_horizontal_candidate
.L_horizontal_none:
    moveq #0,%d0
.L_horizontal_return:
    movem.l (%sp)+,%d2-%d3/%a2-%a3
    rts
.size EntityFindHorizontalCollisionAtX, .-EntityFindHorizontalCollisionAtX
