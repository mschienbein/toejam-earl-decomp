| FM patch/frequency writes, sound-channel templates, and the music bytecode interpreter.
| Independently restated from decoded ROM behavior under ruling R007.
.text
.include "sound.inc"

| Remove the three auxiliary sound streams, clear their channel records, and copy a requested template.
.globl AudioLoadPendingChannelTemplateEffect
.type AudioLoadPendingChannelTemplateEffect, @function
AudioLoadPendingChannelTemplateEffect:
    tst.l (gFmEffectChannel1CurrentStream).l
    beq.s .L_reset_channel3
    move.w #0x2,%d1
    jsr (AudioWriteFmChannelReleaseMask,%pc)
    nop
    clr.l (gMusicFmChannel2Owner).l

    | Clear the next auxiliary stream if it was active.
.L_reset_channel3:
    tst.l (gFmEffectChannel2CurrentStream).l
    beq.s .L_reset_channel1
    move.w #0x3,%d1
    jsr (AudioWriteFmChannelReleaseMask,%pc)
    nop
    clr.l (gMusicFmChannel3Owner).l

    | Clear the remaining auxiliary stream if it was active.
.L_reset_channel1:
    tst.l (gFmEffectChannel3CurrentStream).l
    beq.s .L_clear_sfx_channels
    move.w #0x1,%d1
    jsr (AudioWriteFmChannelReleaseMask,%pc)
    nop
    clr.l (gMusicFmChannel1Owner).l

    | Zero all three 38-byte auxiliary channel records.
.L_clear_sfx_channels:
    move.w #SFX_CLEAR_WORDS_MINUS1,%d0
    lea (gFmEffectChannels).l,%a0
.L_clear_sfx_words:
    clr.w (%a0)+
    dbra %d0,.L_clear_sfx_words
    move.w (gPendingFmEffect).l,%d0
    move.w %d0,(gFmEffectTemplateStatus).l
    beq.s .L_reset_sfx_request

    | Select a template pointer/count by request index and copy complete 38-byte channel records.
    lea (SfxChannelTemplateCounts,%pc),%a0
    move.b (%a0,%d0.w),%d1
    ext.w %d1
    asl.w #0x2,%d0
    lea (SfxChannelTemplatePointers,%pc),%a0
    movea.l (%a0,%d0.w),%a1
    mulu.w #SFX_CHANNEL_WORDS,%d1
    subq.w #0x1,%d1
    lea (gFmEffectChannels).l,%a0
.L_copy_sfx_template:
    move.w (%a1)+,(%a0)+
    dbra %d1,.L_copy_sfx_template

    | Consume the template request.
.L_reset_sfx_request:
    move.w #SOUND_REQUEST_NONE,(gPendingFmEffect).l
    rts
.size AudioLoadPendingChannelTemplateEffect, .-AudioLoadPendingChannelTemplateEffect

| Inputs: d1 FM channel, d3 patch index, d4/d5 frequency high/low bytes.
| Select one of the six YM channels, upload its patch, write frequency, and key the operators.
| The original nine-NOP gaps are retained as instructions (not data) between patch writes.
.globl SetupYM2612
.type SetupYM2612, @function
SetupYM2612:
    cmp.b (gMusicAnimationChannel).l,%d1
    bne.s .L_patch_validate_channel
    move.b #0x1,(gMusicAnimationEvent).l

    | Reject channel indices >= 6 and locate the 38-byte FM patch record.
.L_patch_validate_channel:
    ext.w %d1
    cmpi.w #YM_CHANNEL_COUNT,%d1
    bge.w SetupYM2612_Return
    ext.w %d3
    mulu.w #FM_PATCH_SIZE,%d3
    lea (FMPatches,%pc),%a4
    adda.l %d3,%a4
    ext.w %d1
    lea (gFmChannelKeyMasks).l,%a5
    move.b (FM_PATCH_KEY_MASK,%a4),(%a5,%d1.w)

    | Port 0 handles channels 0..2; port 1 handles channels 3..5 with key-on codes 4..6.
    moveq #0,%d3
    cmpi.w #YM_BANK_CHANNELS,%d1
    bge.s .L_patch_bank1
    cmpi.w #0x2,%d1
    bne.w .L_patch_bank0
    move.b (FM_PATCH_MODE,%a4),%d3
.L_patch_bank0:
    lea (YM2612_ADDRESS_0).l,%a5
    move.w %d1,%d2
    bra.s .L_patch_request_bus
.L_patch_bank1:
    lea (YM2612_ADDRESS_1).l,%a5
    move.w %d1,%d2
    addq.w #0x1,%d2
    subq.w #YM_BANK_CHANNELS,%d1

    | Acquire the Z80 bus; account for the first DAC-delay step.
.L_patch_request_bus:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    nop
    subq.b #0x1,(Z80_DAC_DELAY).l

    | Wait for YM ready, then write YM_REG_LFO and its patch/frequency value.
WaitForYM2612Ready_01:
    btst #YM_BUSY_BIT,(YM2612_ADDRESS_0).l
    bne.s WaitForYM2612Ready_01
    move.b #YM_REG_LFO,(YM2612_ADDRESS_0).l
    nop
    move.b (%a4)+,(YM2612_DATA_0).l
    addq.l #0x1,%a4

    | Wait for YM ready, then write YM_REG_MODE and its patch/frequency value.
WaitForYM2612Ready_02:
    btst #YM_BUSY_BIT,(YM2612_ADDRESS_0).l
    bne.s WaitForYM2612Ready_02
    move.b #YM_REG_MODE,(YM2612_ADDRESS_0).l
    nop
    move.b %d3,(YM2612_DATA_0).l

    | Wait for YM ready, then write YM_REG_FEEDBACK_ALGORITHM and its patch/frequency value.
WaitForYM2612Ready_03:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_03
    move.b #YM_REG_FEEDBACK_ALGORITHM,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_PAN_AMS_FMS and its patch/frequency value.
WaitForYM2612Ready_04:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_04
    move.b #YM_REG_PAN_AMS_FMS,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DTMUL and its patch/frequency value.
WaitForYM2612Ready_05:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_05
    move.b #YM_REG_DTMUL,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_TOTAL_LEVEL and its patch/frequency value.
WaitForYM2612Ready_06:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_06
    move.b #YM_REG_TOTAL_LEVEL,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_ATTACK and its patch/frequency value.
WaitForYM2612Ready_07:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_07
    move.b #YM_REG_ATTACK,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DECAY and its patch/frequency value.
WaitForYM2612Ready_08:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_08
    move.b #YM_REG_DECAY,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RATE and its patch/frequency value.
WaitForYM2612Ready_09:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_09
    move.b #YM_REG_SUSTAIN_RATE,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RELEASE and its patch/frequency value.
WaitForYM2612Ready_10:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_10
    move.b #YM_REG_SUSTAIN_RELEASE,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SSG_ENVELOPE and its patch/frequency value.
WaitForYM2612Ready_11:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_11
    move.b #YM_REG_SSG_ENVELOPE,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b #0x0,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DTMUL+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_12:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_12
    move.b #YM_REG_DTMUL+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_TOTAL_LEVEL+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_13:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_13
    move.b #YM_REG_TOTAL_LEVEL+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_ATTACK+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_14:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_14
    move.b #YM_REG_ATTACK+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DECAY+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_15:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_15
    move.b #YM_REG_DECAY+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RATE+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_16:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_16
    move.b #YM_REG_SUSTAIN_RATE+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RELEASE+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_17:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_17
    move.b #YM_REG_SUSTAIN_RELEASE+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SSG_ENVELOPE+YM_SLOT_2_OFFSET and its patch/frequency value.
WaitForYM2612Ready_18:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_18
    move.b #YM_REG_SSG_ENVELOPE+YM_SLOT_2_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b #0x0,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DTMUL+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_19:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_19
    move.b #YM_REG_DTMUL+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_TOTAL_LEVEL+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_20:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_20
    move.b #YM_REG_TOTAL_LEVEL+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_ATTACK+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_21:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_21
    move.b #YM_REG_ATTACK+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DECAY+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_22:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_22
    move.b #YM_REG_DECAY+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RATE+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_23:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_23
    move.b #YM_REG_SUSTAIN_RATE+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RELEASE+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_24:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_24
    move.b #YM_REG_SUSTAIN_RELEASE+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SSG_ENVELOPE+YM_SLOT_3_OFFSET and its patch/frequency value.
WaitForYM2612Ready_25:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_25
    move.b #YM_REG_SSG_ENVELOPE+YM_SLOT_3_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b #0x0,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DTMUL+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_26:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_26
    move.b #YM_REG_DTMUL+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_TOTAL_LEVEL+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_27:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_27
    move.b #YM_REG_TOTAL_LEVEL+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_ATTACK+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_28:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_28
    move.b #YM_REG_ATTACK+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_DECAY+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_29:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_29
    move.b #YM_REG_DECAY+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RATE+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_30:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_30
    move.b #YM_REG_SUSTAIN_RATE+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SUSTAIN_RELEASE+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_31:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_31
    move.b #YM_REG_SUSTAIN_RELEASE+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b (%a4)+,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_SSG_ENVELOPE+YM_SLOT_4_OFFSET and its patch/frequency value.
WaitForYM2612Ready_32:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_32
    move.b #YM_REG_SSG_ENVELOPE+YM_SLOT_4_OFFSET,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b #0x0,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Normal channels take d4/d5; channel 3 special mode takes four frequency pairs from the patch tail.
    tst.b %d3
    bne.s WaitForYM2612Ready_35

    | Wait for YM ready, then write YM_REG_FREQUENCY_HI and its patch/frequency value.
WaitForYM2612Ready_33:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_33
    move.b #YM_REG_FREQUENCY_HI,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b %d4,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_FREQUENCY_LO and its patch/frequency value.
WaitForYM2612Ready_34:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_34
    move.b #YM_REG_FREQUENCY_LO,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b %d5,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr
    bra.w .L_patch_keyon

    | Wait for YM ready, then write YM_REG_FREQUENCY_HI+2 and its patch/frequency value.
WaitForYM2612Ready_35:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_35
    move.b #YM_REG_FREQUENCY_HI+2,(%a5)
    nop
    move.b (0x1,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_FREQUENCY_LO+2 and its patch/frequency value.
WaitForYM2612Ready_36:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_36
    move.b #YM_REG_FREQUENCY_LO+2,(%a5)
    nop
    move.b (%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_HI_1 and its patch/frequency value.
WaitForYM2612Ready_37:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_37
    move.b #YM_REG_CH3_FREQUENCY_HI_1,(%a5)
    nop
    move.b (0x3,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_LO_1 and its patch/frequency value.
WaitForYM2612Ready_38:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_38
    move.b #YM_REG_CH3_FREQUENCY_LO_1,(%a5)
    nop
    move.b (0x2,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_HI_2 and its patch/frequency value.
WaitForYM2612Ready_39:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_39
    move.b #YM_REG_CH3_FREQUENCY_HI_2,(%a5)
    nop
    move.b (0x5,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_LO_2 and its patch/frequency value.
WaitForYM2612Ready_40:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_40
    move.b #YM_REG_CH3_FREQUENCY_LO_2,(%a5)
    nop
    move.b (0x4,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_HI_3 and its patch/frequency value.
WaitForYM2612Ready_41:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_41
    move.b #YM_REG_CH3_FREQUENCY_HI_3,(%a5)
    nop
    move.b (0x7,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Wait for YM ready, then write YM_REG_CH3_FREQUENCY_LO_3 and its patch/frequency value.
WaitForYM2612Ready_42:
    btst #YM_BUSY_BIT,(%a5)
    bne.s WaitForYM2612Ready_42
    move.b #YM_REG_CH3_FREQUENCY_LO_3,(%a5)
    nop
    move.b (0x6,%a4),(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Pack the patch's key mask and channel code.
.L_patch_keyon:
    andi.b #YM_KEY_CHANNEL_MASK,%d2
    move.b (FM_PATCH_TAIL_KEY_MASK,%a4),%d3
    lsl.b #YM_OPERATOR_MASK_SHIFT,%d3
    or.b %d3,%d2

    | Wait for YM ready, then write YM_REG_KEY_ON and its patch/frequency value.
WaitForYM2612Ready_43:
    btst #YM_BUSY_BIT,(YM2612_ADDRESS_0).l
    bne.s WaitForYM2612Ready_43
    move.b #YM_REG_KEY_ON,(YM2612_ADDRESS_0).l
    nop
    move.b %d2,(YM2612_DATA_0).l

    | Account for five DAC-delay steps, restore the DAC latch, and release the bus.
    subq.b #0x5,(Z80_DAC_DELAY).l
    move.b #YM_REG_DAC_DATA,(YM2612_ADDRESS_0).l
    nop
    nop
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    nop
    nop
    rts
SetupYM2612_Return:
    rts
.size SetupYM2612, .-SetupYM2612

| For channel d1, combine its stored operator mask with the YM channel number and write key-on.
| Restore the DAC address latch before releasing the Z80 bus.
.globl AudioWriteFmChannelReleaseMask
.type AudioWriteFmChannelReleaseMask, @function
AudioWriteFmChannelReleaseMask:
    cmpi.w #YM_CHANNEL_COUNT,%d1
    bge.s .L_key_mask_return
    lea (gFmChannelKeyMasks).l,%a5
    move.b (%a5,%d1.w),%d3
    cmpi.w #YM_BANK_CHANNELS,%d1
    blt.s .L_key_mask_pack
    addq.b #0x1,%d1

    | Pack operator-enable bits above the channel number.
.L_key_mask_pack:
    lsl.b #YM_OPERATOR_MASK_SHIFT,%d3
    andi.b #YM_KEY_CHANNEL_MASK,%d1
    or.b %d3,%d1

    | Take the Z80 bus, wait until YM2612 is ready, and emit the key-mask command.
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    nop
    subq.b #0x1,(Z80_DAC_DELAY).l
.L_key_mask_wait:
    btst #YM_BUSY_BIT,(YM2612_ADDRESS_0).l
    bne.s .L_key_mask_wait
    move.b #YM_REG_KEY_ON,(YM2612_ADDRESS_0).l
    nop
    move.b %d1,(YM2612_DATA_0).l
    move.b #YM_REG_DAC_DATA,(YM2612_ADDRESS_0).l
    nop
    nop
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    nop
    nop
    rts
.L_key_mask_return:
    rts
.size AudioWriteFmChannelReleaseMask, .-AudioWriteFmChannelReleaseMask

| Update channel d1 frequency from d4/d5, choosing the correct YM port pair.
.globl YM2612UpdateFrequency
.type YM2612UpdateFrequency, @function
YM2612UpdateFrequency:
    cmpi.w #YM_CHANNEL_COUNT,%d1
    bge.w YM2612UpdateFrequency_Return
    lea (YM2612_ADDRESS_0).l,%a5
    cmpi.w #YM_BANK_CHANNELS,%d1
    blt.s .L_frequency_request_bus
    lea (YM2612_ADDRESS_1).l,%a5
    subq.w #YM_BANK_CHANNELS,%d1

    | Acquire the Z80 bus and account for the DAC-delay step.
.L_frequency_request_bus:
    move.w #Z80_BUS_TAKE,(Z80_BUS_REQUEST).l
    nop
    subq.b #0x1,(Z80_DAC_DELAY).l
YM2612UpdateFrequency_WaitForYM2612Ready_01:
    btst #YM_BUSY_BIT,(%a5)
    bne.s YM2612UpdateFrequency_WaitForYM2612Ready_01
    move.b #YM_REG_FREQUENCY_HI,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b %d4,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr
YM2612UpdateFrequency_WaitForYM2612Ready_02:
    btst #YM_BUSY_BIT,(%a5)
    bne.s YM2612UpdateFrequency_WaitForYM2612Ready_02
    move.b #YM_REG_FREQUENCY_LO,%d7
    or.b %d1,%d7
    move.b %d7,(%a5)
    nop
    move.b %d5,(YM_DATA_OFFSET,%a5)
    .rept YM_PATCH_GAP_NOPS
    nop
    .endr

    | Restore the DAC address latch before allowing Z80 PCM writes to resume.
    move.b #YM_REG_DAC_DATA,(YM2612_ADDRESS_0).l
    nop
    nop
    move.w #Z80_BUS_RELEASE,(Z80_BUS_REQUEST).l
    nop
    nop
    rts
YM2612UpdateFrequency_Return:
    rts
.size YM2612UpdateFrequency, .-YM2612UpdateFrequency

| Per-tick music interpreter: service requests/timers, choose channel owners, then process each stream.
| The signed byte at MUSIC_PRIORITY decides ownership; higher-or-equal values replace the prior owner.
.globl MusicUpdateSequencer
.type MusicUpdateSequencer, @function
MusicUpdateSequencer:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    tst.w (gFmChannelReleaseRequested).l
    beq.s .L_update_clear_request
    jsr (AudioReleaseAllFmChannels,%pc)

    | Service channel-clear, template, PCM, and PSG requests in their original priority order.
.L_update_clear_request:
    tst.w (gMusicTrackResetRequested).l
    beq.s .L_update_sfx_request
    jsr (MusicClearTrackPointers,%pc)
.L_update_sfx_request:
    tst.w (gPendingFmEffect).l
    blt.s .L_update_pcm_request
    jsr (AudioLoadPendingChannelTemplateEffect,%pc)
.L_update_pcm_request:
    tst.l (gPendingPcmSample).l
    beq.s .L_update_psg_request
    jsr (AudioSubmitPendingPcmSample).l
    bra.s .L_update_psg_timer
.L_update_psg_request:
    tst.w (gPendingPsgEffect).l
    blt.s .L_update_psg_timer
    jsr (AudioSubmitPendingIndexedCommand).l

    | Tick the PSG status countdown.
.L_update_psg_timer:
    tst.w (gPsgEffectStatus).l
    beq.s .L_update_speech_timer
    subq.w #0x1,(gPsgEffectStatus).l

    | Tick speech; an active speech-block flag skips channel processing until the final tick.
.L_update_speech_timer:
    tst.w (gPcmEffectCooldownTicks).l
    beq.s .L_choose_channel_owners
    subq.w #0x1,(gPcmEffectCooldownTicks).l
    tst.w (gPcmBlocksSequencer).l
    beq.s .L_choose_channel_owners
    tst.w (gPcmEffectCooldownTicks).l
    bne.w .L_update_return
    move.w #0x0,(gPcmBlocksSequencer).l

    | Choose one owner for each hardware FM channel by signed priority comparison.
.L_choose_channel_owners:
    lea (gMusicFmChannelOwners).l,%a3
    lea (gMusicChannels).l,%a0
    move.w (gMusicChannelScanCountMinusOne).l,%d0
.L_owner_candidate:
    tst.l (MUSIC_STREAM,%a0)
    beq.s .L_owner_next
    move.b (MUSIC_FM_CHANNEL,%a0),%d1
    ext.w %d1
    lsl.w #0x2,%d1
    move.b (MUSIC_PRIORITY,%a0),%d2
    move.l (%a3,%d1.w),%d3
    beq.s .L_owner_set
    movea.l %d3,%a4
    cmp.b (MUSIC_PRIORITY,%a4),%d2
    bge.s .L_owner_replace
    clr.b (MUSIC_SELECTED,%a0)
    move.b #0x1,(MUSIC_SELECTED,%a4)
    bra.s .L_owner_next
.L_owner_replace:
    clr.b (MUSIC_SELECTED,%a4)
.L_owner_set:
    move.b #0x1,(MUSIC_SELECTED,%a0)
    move.l %a0,(%a3,%d1.w)
.L_owner_next:
    lea (MUSIC_CHANNEL_SIZE,%a0),%a0
    dbra %d0,.L_owner_candidate

    | Process every live stream record after ownership has been resolved.
    lea (gMusicChannels).l,%a0
    move.w (gMusicChannelScanCountMinusOne).l,%d0
.L_vm_channel:
    move.l (MUSIC_STREAM,%a0),%d1
    beq.w .L_vm_next_channel
    movea.l %d1,%a1
    tst.b (MUSIC_WAIT,%a0)
    bgt.w .L_vm_tick

    | A negative byte encodes wait ticks; nonnegative bytes select the following command chain.
.L_vm_command:
    move.b (%a1)+,%d1
    blt.w .L_vm_wait
    bne.w .L_vm_try_note

    | VM 0: remove this stream and relinquish its hardware-channel ownership.
    tst.b (MUSIC_SELECTED,%a0)
    beq.s .L_vm_stop_channel
    move.b (MUSIC_FM_CHANNEL,%a0),%d1
    ext.w %d1
    move.w %d1,%d6
    lsl.w #0x2,%d6
    clr.l (%a3,%d6.w)
    jsr (AudioWriteFmChannelReleaseMask,%pc)
.L_vm_stop_channel:
    clr.l (MUSIC_STREAM,%a0)
    clr.l (MUSIC_CALLBACK,%a0)
    clr.b (MUSIC_NOTE_ACTIVE,%a0)
    clr.b (MUSIC_SELECTED,%a0)
    clr.w (gFmEffectTemplateStatus).l
    bra.w .L_vm_next_channel
.L_vm_try_note:
    subq.b #0x1,%d1
    bne.w .L_vm_try_key_mask

    | VM 1: read frequency bytes, mark the note active, and load the selected patch if this stream owns the channel.
    move.b (MUSIC_FM_CHANNEL,%a0),%d1
    move.b (MUSIC_PATCH,%a0),%d3
    move.b (%a1)+,%d4
    move.b (%a1)+,%d5
    move.b %d4,(MUSIC_FREQUENCY_HI,%a0)
    move.b %d5,(MUSIC_FREQUENCY_LO,%a0)
    move.b #0x1,(MUSIC_NOTE_ACTIVE,%a0)
    ext.w %d1
    tst.b (MUSIC_SELECTED,%a0)
    beq.w .L_vm_save_pc
    jsr (SetupYM2612,%pc)
    bra.w .L_vm_save_pc
.L_vm_try_key_mask:
    subq.b #0x1,%d1
    bne.w .L_vm_try_frequency

    | VM 2: emit the stored key mask for an owned channel and clear note-active state.
    tst.b (MUSIC_SELECTED,%a0)
    beq.s .L_vm_clear_gate
    move.b (MUSIC_FM_CHANNEL,%a0),%d1
    ext.w %d1
    jsr (AudioWriteFmChannelReleaseMask,%pc)
.L_vm_clear_gate:
    clr.b (MUSIC_NOTE_ACTIVE,%a0)
    bra.w .L_vm_save_pc
.L_vm_try_frequency:
    subq.b #0x1,%d1
    bne.w .L_vm_try_patch

    | VM 3: update the stored frequency and the owned hardware channel.
    move.b (%a1)+,%d4
    move.b (%a1)+,%d5
    move.b %d4,(MUSIC_FREQUENCY_HI,%a0)
    move.b %d5,(MUSIC_FREQUENCY_LO,%a0)
    tst.b (MUSIC_SELECTED,%a0)
    beq.w .L_vm_save_pc
    move.b (MUSIC_FM_CHANNEL,%a0),%d1
    ext.w %d1
    jsr (YM2612UpdateFrequency,%pc)
    bra.w .L_vm_save_pc
.L_vm_try_patch:
    subq.b #0x1,%d1
    bne.w .L_vm_try_loop_push

    | VM 4: translate a patch-selection byte through the current sixteen-entry map.
    move.b (%a1)+,%d1
    ext.w %d1
    move.b (gCurrentMusicSelection).l,%d2
    ext.w %d2
    asl.w #SFX_PATCH_MAP_STRIDE_SHIFT,%d2
    add.w %d1,%d2
    lea (InstrumentSelectionMaps,%pc),%a5
    move.b (%a5,%d2.w),%d1
    move.b %d1,(MUSIC_PATCH,%a0)
    bra.w .L_vm_save_pc
.L_vm_try_loop_push:
    subq.b #0x1,%d1
    bne.w .L_vm_try_loop_branch

    | VM 5: push a two-byte loop count into the channel's indexed loop storage.
    move.b (MUSIC_LOOP_DEPTH,%a0),%d1
    ext.w %d1
    move.w %d1,%d2
    add.w %d2,%d2
    move.b (%a1)+,(MUSIC_LOOP_PUSH_HI_BASE,%a0,%d2.w)
    move.b (%a1)+,(MUSIC_LOOP_PUSH_LO_BASE,%a0,%d2.w)
    addq.w #0x1,%d1
    move.b %d1,(MUSIC_LOOP_DEPTH,%a0)
    bra.w .L_vm_save_pc
.L_vm_try_loop_branch:
    subq.b #0x1,%d1
    bne.w .L_vm_try_callback

    | VM 6: decrement the indexed loop counter and take the signed bytecode-relative branch when positive.
    move.b (%a1)+,%d1
    lsl.w #0x8,%d1
    move.b (%a1)+,%d1
    move.b (MUSIC_LOOP_DEPTH,%a0),%d2
    ext.w %d2
    add.w %d2,%d2
    move.b (MUSIC_LOOP_DEPTH,%a0,%d2.w),%d3
    lsl.w #0x8,%d3
    move.b (MUSIC_LOOP_LO_BASE,%a0,%d2.w),%d3
    subq.w #0x1,%d3
    ble.w .L_vm_save_pc
    move.b %d3,(MUSIC_LOOP_LO_BASE,%a0,%d2.w)
    lsr.w #0x8,%d3
    move.b %d3,(MUSIC_LOOP_DEPTH,%a0,%d2.w)
    lea (0x3,%a1,%d1.w),%a1
    bra.w .L_vm_save_pc
.L_vm_try_callback:
    subq.b #0x1,%d1
    bne.w .L_vm_try_branch

    | VM 7: align the stream pointer and load a four-byte callback address.
    move.l %a1,%d1
    btst #0x0,%d1
    beq.s .L_vm_copy_callback
    lea (0x1,%a1),%a1
.L_vm_copy_callback:
    move.b (%a1)+,(MUSIC_CALLBACK,%a0)
    move.b (%a1)+,(MUSIC_CALLBACK_BYTE1,%a0)
    move.b (%a1)+,(MUSIC_CALLBACK_BYTE2,%a0)
    move.b (%a1)+,(MUSIC_CALLBACK_BYTE3,%a0)
    bra.w .L_vm_save_pc
.L_vm_try_branch:
    subq.b #0x1,%d1
    bne.w .L_vm_try_scratch

    | VM 8: take a signed sixteen-bit relative stream branch.
    move.b (%a1)+,%d1
    lsl.w #0x8,%d1
    move.b (%a1)+,%d1
    lea (%a1,%d1.w),%a1
    bra.w .L_vm_save_pc
.L_vm_try_scratch:
    subq.b #0x1,%d1
    bne.w .L_vm_try_clap

    | VM 9: request the record-scratch PCM effect.
    pea (Sfx_JamRecordScratch).l
    jsr (AudioTrySubmitSequencerPcmSample).l
    addq.l #0x4,%sp
    bra.w .L_vm_save_pc
.L_vm_try_clap:
    subq.b #0x1,%d1
    bne.w .L_vm_try_bass

    | VM 10: request the clap PCM effect.
    pea (Sfx_JamClap).l
    jsr (AudioTrySubmitSequencerPcmSample).l
    addq.l #0x4,%sp
    bra.w .L_vm_save_pc
.L_vm_try_bass:
    subq.b #0x1,%d1
    bne.w .L_vm_try_snare

    | VM 11: request the bass-drum PCM effect and set the music event flag.
    pea (Sfx_JamBassDrum).l
    jsr (AudioTrySubmitSequencerPcmSample).l
    addq.l #0x4,%sp
    move.b #0x1,(gMusicAnimationEvent).l
    bra.w .L_vm_save_pc
.L_vm_try_snare:
    subq.b #0x1,%d1
    bne.w .L_vm_save_pc

    | VM 12: request the snare-drum PCM effect.
    pea (Sfx_JamSnareDrum).l
    jsr (AudioTrySubmitSequencerPcmSample).l
    addq.l #0x4,%sp
    bra.w .L_vm_save_pc

    | Save the updated program counter and continue decoding commands in the same tick.
.L_vm_save_pc:
    move.l %a1,(MUSIC_STREAM,%a0)
    bra.w .L_vm_command

    | Wait token: keep the low seven bits and save the program counter.
.L_vm_wait:
    andi.b #0x7f,%d1
    move.b %d1,(MUSIC_WAIT,%a0)
    move.l %a1,(MUSIC_STREAM,%a0)

    | Consume one wait tick and invoke an installed callback with this channel record.
.L_vm_tick:
    subq.b #0x1,(MUSIC_WAIT,%a0)
    move.l (MUSIC_CALLBACK,%a0),%d1
    beq.w .L_vm_next_channel
    movea.l %d1,%a1
    move.l %a0,-(%sp)
    move.l %d0,-(%sp)
    jsr (%a1)
    move.l (%sp)+,%d0
    movea.l (%sp)+,%a0

    | Advance to the next fixed-size channel record.
.L_vm_next_channel:
    lea (MUSIC_CHANNEL_SIZE,%a0),%a0
    dbra %d0,.L_vm_channel
.L_update_return:
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    rts
.size MusicUpdateSequencer, .-MusicUpdateSequencer
