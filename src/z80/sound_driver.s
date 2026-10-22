; ToeJam & Earl Z80 sound driver. Assemble/link at Z80 address 0.
; Same upload image in REV 02 and REV 00; independently decoded under R007.
; The 68000 runs music/FM sequencing. This program supplies interruptible PSG
; effects and one unsigned-PCM stream through the YM2612 channel-six DAC.
;
; 68000 interface (Z80 RAM is mapped at 0xa00000):
;   AudioSubmitPendingIndexedCommand: command 1, argument = gMusicUnknownE47E (PlaySound's ID).
;   AudioSubmitPendingPcmSample / AudioTrySubmitSequencerPcmSample: command 2, LE24 payload address and LE16 count
;     after consuming a sample's big-endian length header on the 68000 side.
;   Both take the Z80 bus and inspect Z80MailboxBusy; while it is nonzero they
;     release the bus, delay and retry. Once it is clear they write the fields,
;     publish Z80Command last and release the bus. Busy protects the short
;     mailbox snapshot, not the duration of playback.
; Internal routine names below describe observed operations, not original names.

.equ Z80_RAM_END,             0x2000
.equ YM2612_ADDRESS,          0x4000
.equ YM2612_DATA,             0x4001
.equ Z80_ROM_BANK,            0x6000
.equ PSG_PORT,                0x7f11
.equ ROM_WINDOW,              0x8000
.equ ROM_BANK_BITS_AFTER_A15, 8
.equ YM_DAC_DATA,             0x2a
.equ YM_DAC_ENABLE,           0x2b
.equ DAC_ENABLED,             0x80
.equ DAC_MIDPOINT,            0x80
.equ DAC_STARTUP_LEVEL1,      0x20
.equ DAC_STARTUP_LEVEL2,      0x40
.equ STARTUP_DELAY_UNITS,     5
.equ COMMAND_PSG,             1
.equ COMMAND_PCM,             2
.equ PSG_EFFECT_LIMIT,        30
.equ PSG_ACTIVE_VALUE,        0x0f
.equ PCM_DELAY_RELOAD,        4
.equ DELAY_SPIN_COUNT,        0x54

; SN76489 latch/register selectors; following low nibble is period/attenuation.
; Tone periods take a low-nibble latch byte followed by a six-bit high byte.
.equ PSG_TONE0,               0x80
.equ PSG_VOLUME0,             0x90
.equ PSG_TONE1,               0xa0
.equ PSG_VOLUME1,             0xb0
.equ PSG_TONE2,               0xc0
.equ PSG_VOLUME2,             0xd0
.equ PSG_NOISE,               0xe0
.equ PSG_VOLUME3,             0xf0
.equ PSG_MUTE,                0x0f
.equ PSG_WHITE_NOISE,         4
.equ PSG_NOISE_RATE_TONE2,    3

.text
.globl SoundDriverProgram
SoundDriverProgram:
    im 1
    di
    ld sp,Z80_RAM_END
    jp DriverInitialize

    ; Unused restart-vector space. Interrupts remain disabled throughout.
    .space 0x38-(.-SoundDriverProgram),0
DriverIRQ:
    ret
    .space 0x40-(.-SoundDriverProgram),0

; Zero-initialized mailbox and live state, at their 68000-visible offsets.
Z80Command:                       ; 0x40: 0 idle, 1 PSG, 2 PCM; publish last.
    .db 0
Z80MailboxBusy:                   ; 0x41: nonzero only while copying PCM args.
    .db 0
Z80CommandArgument:               ; 0x42: PSG effect ID; PCM sender also writes it.
    .db 0
    .db 0                        ; 0x43: unused mailbox byte.
Z80SampleAddress:                 ; 0x44..45: low word of 68000 payload address.
    .dw 0
Z80SampleAddressHi:               ; 0x46: high byte of 68000 payload address.
    .db 0
    .db 0                        ; 0x47: unused mailbox byte.
Z80SampleCount:                   ; 0x48..49: number of PCM bytes, little-endian.
    .dw 0
Z80SampleDelay:                   ; 0x4a: decremented sample pacing counter.
    .db 0
Z80PSGActive:                     ; 0x4b: 0x0f at dispatch, 0 on silence/return.
    .db 0
    ; Some effects call SilencePSG mid-sequence, so zero is not a guarantee
    ; that an effect's final note has finished.
    .space 4,0                   ; 0x4c..4f: unused.
Z80SampleBankBase:                ; 0x50..51: copied address, +0x8000 on wrap.
    .dw 0
Z80SampleBankBaseHi:              ; 0x52: copied address high byte, with carry.
    .db 0
    .db 0                        ; 0x53: unused.
Z80RestoreNoise:                  ; 0x54: restore background noise after PSG FX.
    .db 0
    .space 0x66-(.-SoundDriverProgram),0
DriverNMI:
    retn

DriverInitialize:
    xor a
    ld (Z80MailboxBusy),a
    ld (Z80Command),a
    ld (Z80RestoreNoise),a
    call SilencePSG

    ; Startup DAC ramp: enable, output 0x20 then 0x40, center, disable.
    ld hl,YM2612_ADDRESS
    ld (hl),YM_DAC_ENABLE
    inc hl
    ld (hl),DAC_ENABLED
    dec hl
    ld (hl),YM_DAC_DATA
    inc hl
    ld (hl),DAC_STARTUP_LEVEL1
    ld c,STARTUP_DELAY_UNITS
    call WaitForCommand
    ld (hl),DAC_STARTUP_LEVEL2
    ld c,STARTUP_DELAY_UNITS
    call WaitForCommand
    ld (hl),DAC_MIDPOINT
    dec hl
    ld (hl),YM_DAC_ENABLE
    inc hl
    ld (hl),0

DriverPollCommand:
    ld a,(Z80Command)
    cp COMMAND_PSG
    jp z,DispatchPSGEffect
    cp COMMAND_PCM
    jp z,StartPCM
    jp DriverPollCommand

DispatchPSGEffect:
    xor a
    ld (Z80Command),a
    call SilencePSG
    ld a,PSG_ACTIVE_VALUE
    ld (Z80PSGActive),a
    ld a,(Z80CommandArgument)
    cp PSG_EFFECT_LIMIT
    ; ORIGINAL BUG: an out-of-range ID returns to polling without clearing
    ; Z80PSGActive, which was set above. Preserve the game's exact behavior.
    jp nc,DriverPollCommand
    ld hl,PSGEffectTable
    ccf                          ; CP proved carry set; double ID with no carry.
    rla
    ld c,a
    ld b,0
    add hl,bc
    ld d,(hl)                    ; Table stores big-endian addresses, unlike .dw.
    inc hl
    ld e,(hl)
    ld hl,PSGEffectFinished
    push hl                      ; Synthetic call: effect RETs to the epilogue.
    push de
    ld hl,PSG_PORT
    ret

PSGEffectFinished:
    xor a
    ld (Z80PSGActive),a
    ld a,(Z80RestoreNoise)
    or a
    jr z,DriverPollCommand
    ; Effect 5 leaves background tone-2-clocked white noise enabled; restore
    ; this exact tone/noise state after any later PSG effect until effect 6.
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld (hl),PSG_TONE2+PSG_MUTE
    ld (hl),7
    ld (hl),PSG_VOLUME3+4
    jp DriverPollCommand

StartPCM:
    ld a,1
    ld (Z80MailboxBusy),a
    ld hl,(Z80SampleCount)
    ex de,hl                     ; DE is the remaining unsigned byte count.
    ; Count is tested after the first write; a zero count streams 65536 bytes.
    ld a,(Z80SampleAddressHi)
    ld (Z80SampleBankBaseHi),a
    ld hl,(Z80SampleAddress)
    ld (Z80SampleBankBase),hl
    ld c,a
    xor a
    ld (Z80Command),a
    ld (Z80MailboxBusy),a
    call SelectROMBank
    ld a,h
    or ROM_WINDOW >> 8
    ld h,a                       ; Map original address low 15 bits to 0x8000+.
    ld bc,YM2612_ADDRESS
    ld a,YM_DAC_ENABLE
    ld (bc),a
    ld a,DAC_ENABLED
    ld (YM2612_DATA),a
    exx
    ld e,PCM_DELAY_RELOAD
    ld hl,Z80SampleDelay
    ld (hl),e
    exx

PCMWriteSample:
    ld a,YM_DAC_DATA
    ld (bc),a
    ld a,(hl)
    ld (YM2612_DATA),a
    inc hl
    ld a,h
    or l
    jp z,PCMNextROMBank
    exx                          ; Alternate HL/E retain pacing state.
PCMWaitSample:
    ld a,(Z80Command)
    or a
    jp nz,StopPCM                ; A new command preempts PCM at this poll.
    nop
    dec (hl)
    ld a,(hl)
    or a
    jp p,PCMWaitSample            ; 4 -> 3,2,1,0,0xff: five delay iterations.
    ld (hl),e
    exx
PCMCountSample:
    dec de
    ld a,d
    or e
    jp nz,PCMWriteSample

StopPCM:
    ld bc,YM2612_ADDRESS
    ld a,YM_DAC_DATA
    ld (bc),a
    ld a,DAC_MIDPOINT
    ld (YM2612_DATA),a
    ld a,YM_DAC_ENABLE
    ld (bc),a
    ld a,0
    ld (YM2612_DATA),a
    jp DriverPollCommand

PCMNextROMBank:
    ; HL wrapped from 0xffff to 0. Advance the saved 24-bit address by 32 KiB.
    ; Bank programming replaces the usual PCMWaitSample path for this sample.
    call AdvanceROMBank
    ld hl,ROM_WINDOW
    ld bc,YM2612_ADDRESS
    jp PCMCountSample

AdvanceROMBank:
    ld a,(Z80SampleBankBase+1)
    add a,ROM_WINDOW >> 8
    ld (Z80SampleBankBase+1),a
    ld h,a
    ld a,(Z80SampleBankBaseHi)
    adc a,0
    ld c,a
    ld (Z80SampleBankBaseHi),a
SelectROMBank:
    ; Genesis bank latch takes address bits A15..A23, least-significant first.
    ; H supplies A15; C supplies A16..A23. DE (sample count) is preserved.
    ld a,h
    rlca
    and 1
    ld (Z80_ROM_BANK),a
    ld a,c
    ld b,ROM_BANK_BITS_AFTER_A15
SelectROMBankBit:
    ld c,a
    and 1
    ld (Z80_ROM_BANK),a
    ld a,c
    rra
    djnz SelectROMBankBit
    ret

WaitForCommand:
    ; Delay C units, each DELAY_SPIN_COUNT polls; preserves HL and DE.
    ; Effects must use EXX around this call when their B/C values are live.
    ld b,DELAY_SPIN_COUNT
WaitForCommandSpin:
    ld a,(Z80Command)
    or a
    jp nz,AbortPSGEffect
    djnz WaitForCommandSpin
    dec c
    jp nz,WaitForCommand
    ret

AbortPSGEffect:
    ; Abandon nested effect returns and service the new command immediately.
    ld sp,Z80_RAM_END
    call SilencePSG
    jp DriverPollCommand

SilencePSG:
    ld hl,PSG_PORT
    ld (hl),PSG_VOLUME0+PSG_MUTE
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ld (hl),PSG_VOLUME2+PSG_MUTE
    ld (hl),PSG_VOLUME3+PSG_MUTE
    xor a
    ld (Z80PSGActive),a
    ret

; ID 0x0f: tone 0 period 0x0a4; tone 1 period 0x0a0 at attenuation 11.
; Fade tone 0 from attenuation 0 to 14, 0x72 delay units per step.
PSGEffect0F_DualToneFade:
    ld (hl),PSG_TONE0+4
    ld (hl),0x0a
    ld d,PSG_VOLUME0
PSGEffect0F_SetVolume:
    ld (hl),d
    ld (hl),PSG_TONE1
    ld (hl),0x0a
    ld (hl),PSG_VOLUME1+11
PSGEffect0F_Fade:
    ld c,0x72
    call WaitForCommand
    inc d
    ld (hl),d
    ld a,PSG_VOLUME0+PSG_MUTE
    cp d
    jp nz,PSGEffect0F_Fade
    ld (hl),PSG_VOLUME0+PSG_MUTE
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ret

; ID 8: white noise clocked by tone 2. Sweep period 0x020..0x05e,
; and raise noise attenuation each second high-period step.
PSGEffect08_NoiseRise:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld b,PSG_VOLUME3
    ld (hl),b
    ld de,(PSG_TONE2 << 8)+2
PSGEffect08_Period:
    ld (hl),d
    ld (hl),e
    inc d
    ld a,PSG_TONE2+PSG_MUTE
    cp d
    jp nz,PSGEffect08_Wait
    ld d,PSG_TONE2
    inc e
    ld a,e
    and 1
    jp nz,PSGEffect08_Wait
    inc b
    ld (hl),b
PSGEffect08_Wait:
    exx
    ld c,0x12
    call WaitForCommand
    exx
    ld a,e
    cp 6
    jp nz,PSGEffect08_Period
    ld (hl),PSG_VOLUME3+PSG_MUTE
    ret

; ID 9: audible tone-0 low-period sweep at full volume, fixed high bits 1.
PSGEffect09_ToneSweep:
    ld (hl),PSG_VOLUME0
    ld de,(PSG_TONE0 << 8)+1
PSGEffect09_Period:
    ld (hl),d
    ld (hl),e
    ld c,0x18
    call WaitForCommand
    inc d
    ld a,d
    cp PSG_TONE0+PSG_MUTE
    jp nz,PSGEffect09_Period
    ld (hl),PSG_VOLUME0+PSG_MUTE
    ret

; ID 0x0a: fixed-rate white noise, attenuation 0..14, increasing delays.
PSGEffect0A_NoiseFade:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld d,PSG_VOLUME3
    ld e,0x0f
PSGEffect0A_Volume:
    ld (hl),d
    ld c,e
    call WaitForCommand
    ld a,0x0f
    add a,e
    ld e,a
    inc d
    ld a,PSG_VOLUME3+PSG_MUTE
    cp d
    jp nz,PSGEffect0A_Volume
    ld (hl),d
    ret

; ID 0x0b: fixed-rate white noise at rate 2, progressively slower fade.
PSGEffect0B_NoiseFade:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+2
    ld d,PSG_VOLUME3
    ld e,8
PSGEffect0B_Volume:
    ld (hl),d
    ld c,e
    call WaitForCommand
    ld a,0x0c
    add a,e
    ld e,a
    inc d
    ld a,PSG_VOLUME3+PSG_MUTE
    cp d
    jp nz,PSGEffect0B_Volume
    ld (hl),d
    ret

; ID 0x0c: longer tone-2-clocked noise sweep, period high bits 0..11.
PSGEffect0C_NoiseRise:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld b,PSG_VOLUME3
    ld (hl),b
    ld de,PSG_TONE2 << 8
PSGEffect0C_Period:
    ld (hl),d
    ld (hl),e
    inc d
    ld a,PSG_TONE2+PSG_MUTE
    cp d
    jp nz,PSGEffect0C_Wait
    ld d,PSG_TONE2
    inc e
    ld a,e
    and 1
    jp nz,PSGEffect0C_Wait
    inc b
    ld (hl),b
PSGEffect0C_Wait:
    exx
    ld c,0x0a
    call WaitForCommand
    exx
    ld a,e
    cp 0x0c
    jp nz,PSGEffect0C_Period
    ld (hl),PSG_VOLUME3+PSG_MUTE
    ret

; ID 0x0d: fixed-rate white noise, attenuation rises, delay grows by 5.
PSGEffect0D_NoiseFade:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld d,PSG_VOLUME3
    ld e,0x12
PSGEffect0D_Volume:
    ld (hl),d
    ld c,e
    call WaitForCommand
    ld a,5
    add a,e
    ld e,a
    inc d
    ld a,PSG_VOLUME3+PSG_MUTE
    cp d
    jp nz,PSGEffect0D_Volume
    ld (hl),d
    ret

; ID 3: tone-2-clocked noise; sweep the period while fading to attenuation 12.
PSGEffect03_NoiseSweep:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld b,PSG_VOLUME3
    ld (hl),b
    ld de,PSG_TONE2 << 8
PSGEffect03_Period:
    ld (hl),d
    ld (hl),e
    ld a,PSG_TONE2+PSG_MUTE
    cp d
    jp nz,PSGEffect03_Wait
    ld d,PSG_TONE2-1
    inc e
    ld a,e
    and 1
    jp nz,PSGEffect03_Wait
    inc b
    ld (hl),b
PSGEffect03_Wait:
    inc d
    exx
    ld c,0x0a
    call WaitForCommand
    exx
    ld a,b
    cp PSG_VOLUME3+12
    jp nz,PSGEffect03_Period
    ret

; ID 4: reverse the tone-2 period sweep, high bits 6 down through 1.
PSGEffect04_NoiseFall:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld (hl),PSG_VOLUME3+3
    ld de,((PSG_TONE2+PSG_MUTE) << 8)+6
PSGEffect04_Period:
    ld (hl),d
    ld (hl),e
    ld c,2
    call WaitForCommand
    dec d
    ld a,PSG_TONE2-1
    cp d
    jp nz,PSGEffect04_Period
    ld d,PSG_TONE2+PSG_MUTE
    dec e
    jp nz,PSGEffect04_Period
    jp SilencePSG

; ID 2: two tone-0 sweeps with period high bits 1 then 2, attenuation 4.
PSGEffect02_DoubleToneSweep:
    ld (hl),PSG_VOLUME0+4
    ld de,(PSG_TONE0 << 8)+1
PSGEffect02_FirstPeriod:
    ld (hl),d
    ld (hl),e
    ld c,8
    call WaitForCommand
    inc d
    ld a,d
    cp PSG_TONE0+PSG_MUTE
    jp nz,PSGEffect02_FirstPeriod
    ld de,(PSG_TONE0 << 8)+2
PSGEffect02_SecondPeriod:
    ld (hl),d
    ld (hl),e
    ld c,8
    call WaitForCommand
    inc d
    ld a,d
    cp PSG_TONE0+PSG_MUTE
    jp nz,PSGEffect02_SecondPeriod
    ld (hl),PSG_VOLUME0+PSG_MUTE
    ret

; ID 5: shorter version of effect 3; ask the epilogue to retain background
; white noise at tone-2 period 0x07f and attenuation 4 between PSG commands.
PSGEffect05_StartBackgroundNoise:
    ld a,1
    ld (Z80RestoreNoise),a
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+PSG_NOISE_RATE_TONE2
    ld b,PSG_VOLUME3
    ld (hl),b
    ld de,PSG_TONE2 << 8
PSGEffect05_Period:
    ld (hl),d
    ld (hl),e
    ld a,PSG_TONE2+PSG_MUTE
    cp d
    jp nz,PSGEffect05_Wait
    ld d,PSG_TONE2-1
    inc e
    ld a,e
    and 1
    jp nz,PSGEffect05_Wait
    inc b
    ld (hl),b
PSGEffect05_Wait:
    inc d
    exx
    ld c,0x0a
    call WaitForCommand
    exx
    ld a,b
    cp PSG_VOLUME3+4
    jp nz,PSGEffect05_Period
    ret

; ID 6: clear effect 5's persistent-noise request and mute all PSG channels.
PSGEffect06_StopBackgroundNoise:
    xor a
    ld (Z80RestoreNoise),a
    jp SilencePSG

; ID 7: short tone-1 pulse, period 0x08c, attenuation 2.
PSGEffect07_TonePulse:
    ld (hl),PSG_TONE1+12
    ld (hl),8
    ld (hl),PSG_VOLUME1+2
    ld c,0x0a
    call WaitForCommand
    jp SilencePSG

; ID 1: two tone-1 pulses separated by silence.
PSGEffect01_DoubleTonePulse:
    ld (hl),PSG_TONE1
    ld (hl),0x0a
    ld (hl),PSG_VOLUME1+1
    ld c,0x0f
    call WaitForCommand
    call SilencePSG
    ld c,0x0a
    call WaitForCommand
    ld (hl),PSG_TONE1+12
    ld (hl),8
    ld (hl),PSG_VOLUME1+1
    ld c,0x1e
    call WaitForCommand
    jp SilencePSG

; ID 0x0e: tone-1 period 0x0f0, gap, then period 0x0ac, full volume.
PSGEffect0E_TwoNotes:
    ld (hl),PSG_TONE1
    ld (hl),0x0f
    ld (hl),PSG_VOLUME1
    ld c,0x0a
    call WaitForCommand
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ld c,0x0a
    call WaitForCommand
    ld (hl),PSG_TONE1+12
    ld (hl),0x0a
    ld (hl),PSG_VOLUME1
    ld c,0x14
    call WaitForCommand
    jp SilencePSG

; ID 0x10: white-noise rate 2, fade from attenuation 0 to mute.
PSGEffect10_NoiseFade:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE+2
    ld d,PSG_VOLUME3
PSGEffect10_Volume:
    ld (hl),d
    ld c,0x1b
    call WaitForCommand
    inc d
    ld a,PSG_VOLUME3+PSG_MUTE
    cp d
    jp nz,PSGEffect10_Volume
    ld (hl),d
    ret

; ID 0x11: descending tone-1 period-high steps, gated by long silent gaps.
PSGEffect11_GatedToneSweep:
    ld de,((PSG_TONE1+12) << 8)+0x18
PSGEffect11_Period:
    ld (hl),PSG_VOLUME1
    ld (hl),d
    ld (hl),e
    ld c,0x0f
    call WaitForCommand
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ld c,0x1e
    call WaitForCommand
    dec e
    jp nz,PSGEffect11_Period
    jp SilencePSG

; ID 0x12: nine short bursts of tone 1 (period 0x380) and white noise.
PSGEffect12_ToneNoiseBursts:
    ld e,9
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld (hl),PSG_TONE1
    ld (hl),0x38
PSGEffect12_Burst:
    ld (hl),PSG_VOLUME1+3
    ld (hl),PSG_VOLUME3
    ld c,6
    call WaitForCommand
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ld (hl),PSG_VOLUME3+PSG_MUTE
    ld c,6
    call WaitForCommand
    dec e
    jp nz,PSGEffect12_Burst
    ret

; ID 0x13: tone-1 period 0x380 under a rising-volume white-noise envelope.
PSGEffect13_NoiseCrescendo:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld (hl),PSG_TONE1
    ld (hl),0x38
    ld (hl),PSG_VOLUME1+2
    ld d,PSG_VOLUME3+PSG_MUTE
PSGEffect13_Volume:
    ld (hl),d
    ld c,0x1b
    call WaitForCommand
    dec d
    ld a,d
    cp PSG_VOLUME3
    jp nz,PSGEffect13_Volume
    ld (hl),PSG_VOLUME1+PSG_MUTE
    ld c,0x23
    call WaitForCommand
    jp SilencePSG

; ID 0x14: four tone-1 period steps, then a five-step attenuation fade.
PSGEffect14_TonePhrase:
    ld (hl),PSG_TONE1
    ld (hl),8
    ld (hl),PSG_VOLUME1+2
    ld c,0x1b
    call WaitForCommand
    ld (hl),PSG_TONE1
    ld (hl),6
    ld (hl),PSG_VOLUME1+1
    ld c,0x1b
    call WaitForCommand
    ld (hl),PSG_TONE1
    ld (hl),4
    ld (hl),PSG_VOLUME1
    ld c,0x1b
    call WaitForCommand
    ld (hl),PSG_TONE1
    ld (hl),6
    ld (hl),PSG_VOLUME1+1
    ld c,0x1b
    call WaitForCommand
    ld d,PSG_VOLUME1+2
PSGEffect14_Fade:
    ld (hl),PSG_TONE1
    ld (hl),8
    ld (hl),d
    ld c,0x1b
    call WaitForCommand
    inc d
    ld a,d
    cp PSG_VOLUME1+7
    jp nz,PSGEffect14_Fade
    jp SilencePSG

; ID 0x15: tone-1 period-high sweep from 0x18 down to 7 at full volume.
PSGEffect15_ToneRise:
    ld de,((PSG_TONE1+12) << 8)+0x18
    ld (hl),PSG_VOLUME1
PSGEffect15_Period:
    ld (hl),d
    ld (hl),e
    ld c,0x0c
    call WaitForCommand
    dec e
    ld a,e
    cp 6
    jp nz,PSGEffect15_Period
    jp SilencePSG

; ID 0x16: longer tone-1 period-high sweep, attenuation 2, high 0x28..7.
PSGEffect16_ToneRise:
    ld de,((PSG_TONE1+12) << 8)+0x28
    ld (hl),PSG_VOLUME1+2
PSGEffect16_Period:
    ld (hl),d
    ld (hl),e
    ld c,0x0c
    call WaitForCommand
    dec e
    ld a,e
    cp 6
    jp nz,PSGEffect16_Period
    jp SilencePSG

; ID 0x17: white-noise crescendo from mute down to attenuation 2, then hold.
PSGEffect17_NoiseCrescendo:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld d,PSG_VOLUME3+PSG_MUTE
PSGEffect17_Volume:
    ld (hl),d
    ld c,8
    call WaitForCommand
    dec d
    ld a,d
    cp PSG_VOLUME3+1
    jp nz,PSGEffect17_Volume
    ld c,0x0f
    call WaitForCommand
    jp SilencePSG

; ID 0x18: similar crescendo, followed by attenuation 7 then 0 accents.
PSGEffect18_NoiseAccent:
    ld (hl),PSG_NOISE+PSG_WHITE_NOISE
    ld d,PSG_VOLUME3+PSG_MUTE
PSGEffect18_Volume:
    ld (hl),d
    ld c,0x0c
    call WaitForCommand
    dec d
    ld a,d
    cp PSG_VOLUME3+2
    jp nz,PSGEffect18_Volume
    ld c,0x10
    call WaitForCommand
    ld (hl),PSG_VOLUME3+7
    ld c,7
    call WaitForCommand
    ld (hl),PSG_VOLUME3
    ld c,5
    call WaitForCommand
    jp SilencePSG

; Not selected by the present dispatch table: long periodic-noise rate 0.
PSGPeriodicNoiseRate0:
    ld (hl),PSG_NOISE
    ld (hl),PSG_VOLUME3
    ld c,0xc8
    call WaitForCommand
    jp SilencePSG

; ID 0x19: long periodic noise at fixed rate 2.
PSGEffect19_PeriodicNoise:
    ld (hl),PSG_NOISE+2
    ld (hl),PSG_VOLUME3
    ld c,0xc8
    call WaitForCommand
    jp SilencePSG

; Not selected by the present dispatch table: long periodic-noise rate 1.
PSGPeriodicNoiseRate1:
    ld (hl),PSG_NOISE+1
    ld (hl),PSG_VOLUME3
    ld c,0xc8
    call WaitForCommand
    jp SilencePSG

; ID 0x1a: long periodic noise clocked by tone 2, whose period is 0x050.
PSGEffect1A_PeriodicTone2Noise:
    ld (hl),PSG_NOISE+PSG_NOISE_RATE_TONE2
    ld (hl),PSG_VOLUME3
    ld (hl),PSG_TONE2
    ld (hl),5
    ld c,0xc8
    call WaitForCommand
    jp SilencePSG

; Big-endian routine addresses, consumed as D=high, E=low by the dispatcher.
; Z80 .dw is little-endian, so emit each symbolic address explicitly as bytes.
; Entries 0x1e..0x21 are present in the image but rejected by the ID<30 check.
.macro effect_address routine
    .db ((\routine-SoundDriverProgram) >> 8),((\routine-SoundDriverProgram) & 0xff)
.endm
PSGEffectTable:
    effect_address SilencePSG                         ; 0x00
    effect_address PSGEffect01_DoubleTonePulse         ; 0x01
    effect_address PSGEffect02_DoubleToneSweep         ; 0x02
    effect_address PSGEffect03_NoiseSweep              ; 0x03
    effect_address PSGEffect04_NoiseFall               ; 0x04
    effect_address PSGEffect05_StartBackgroundNoise    ; 0x05
    effect_address PSGEffect06_StopBackgroundNoise     ; 0x06
    effect_address PSGEffect07_TonePulse               ; 0x07
    effect_address PSGEffect08_NoiseRise               ; 0x08
    effect_address PSGEffect09_ToneSweep               ; 0x09
    effect_address PSGEffect0A_NoiseFade                ; 0x0a
    effect_address PSGEffect0B_NoiseFade                ; 0x0b
    effect_address PSGEffect0C_NoiseRise                ; 0x0c
    effect_address PSGEffect0D_NoiseFade                ; 0x0d
    effect_address PSGEffect0E_TwoNotes                 ; 0x0e
    effect_address PSGEffect0F_DualToneFade             ; 0x0f
    effect_address PSGEffect10_NoiseFade                ; 0x10
    effect_address PSGEffect11_GatedToneSweep           ; 0x11
    effect_address PSGEffect12_ToneNoiseBursts          ; 0x12
    effect_address PSGEffect13_NoiseCrescendo           ; 0x13
    effect_address PSGEffect14_TonePhrase               ; 0x14
    effect_address PSGEffect15_ToneRise                 ; 0x15
    effect_address PSGEffect16_ToneRise                 ; 0x16
    effect_address PSGEffect17_NoiseCrescendo           ; 0x17
    effect_address PSGEffect18_NoiseAccent              ; 0x18
    effect_address PSGEffect19_PeriodicNoise            ; 0x19
    effect_address PSGEffect1A_PeriodicTone2Noise        ; 0x1a
    effect_address SilencePSG                         ; 0x1b
    effect_address SilencePSG                         ; 0x1c
    effect_address SilencePSG                         ; 0x1d
    effect_address SilencePSG                         ; 0x1e (unreachable ID)
    effect_address SilencePSG                         ; 0x1f (unreachable ID)
    effect_address SilencePSG                         ; 0x20 (unreachable ID)
    effect_address SilencePSG                         ; 0x21 (unreachable ID)
SoundDriverEnd:
