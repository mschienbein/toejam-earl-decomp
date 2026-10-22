; Boot-time Z80 image, uploaded by InitialiseTheMegadrive to Z80 RAM at 0.
; Independently decoded from the game's bytes under ruling R007. Both ROM
; revisions contain this same 38-byte program. This is not the later six-byte
; idle program written by InitialiseTheZ80 before LoadSoundDriverToZ80.

.equ Z80_RAM_END, 0x2000
.equ JP_HL_OPCODE, 0xe9

.text
; Clear unused Z80 RAM, reset both register banks, then enter the cartridge idle loop.
.globl InitialiseTheMegadrive_Z80SetupValues
InitialiseTheMegadrive_Z80SetupValues:
    xor a
    ld bc,Z80_RAM_END - (BootstrapEnd - InitialiseTheMegadrive_Z80SetupValues + 1)
    ld de,BootstrapEnd+1
    ld hl,BootstrapEnd
    ld sp,hl
    ld (hl),a
    ldir                        ; Seed + overlapping copy clears 0x26..0x1fff.

    ; The cleared RAM is also a stack of zero values for both register banks.
    pop ix
    pop iy
    ld i,a
    ld r,a
    pop de
    pop hl
    pop af
    ex af,af'
    exx
    pop bc
    pop de
    pop hl
    pop af
    ld sp,hl                    ; HL and SP are now zero.
    di
    im 1
    ld (hl),JP_HL_OPCODE         ; Replace entry byte with "jp (hl)".
    jp (hl)                     ; Idle at address 0 until the 68000 replaces it.
BootstrapEnd:
