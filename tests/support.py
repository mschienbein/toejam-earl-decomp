"""Shared test helpers: a synthetic Genesis ROM with known code, a switch table and data."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

SIZE = 0x1000


def synthetic_rom() -> bytes:
    """Vectors -> 0x200; a caller, a switch function with a 3-entry table, and unreached data.

    0x200 jsr $300.l / bsr.w $300 / bra.s self          (reset handler)
    0x300 cmpi.w #2,d0 / bgt.s $322 / add.w d0,d0 / move.w $310(pc,d0.w),d0 / jmp $310(pc,d0.w)
    0x310 .short 6, 10, 14                              (cases at 0x316, 0x31a, 0x31e)
    0x316 moveq #1,d0 / rts ; 0x31a moveq #2,d0 / rts ; 0x31e moveq #-1,d0 / rts
    0x322 moveq #0,d0 / rts                             (default)
    0x326 "DATA..." (never reached: stays data)
    """
    rom = bytearray(SIZE)
    struct.pack_into(">II", rom, 0, 0x00FF8000, 0x200)
    rom[0x100:0x110] = b"SEGA GENESIS    "
    rom[0x180:0x18E] = b"GM 00000000-00"
    rom[0x200:0x20C] = bytes.fromhex("4eb900000300" "610000f8" "60fe")
    switch_words = (
        0x0C40, 0x0002, 0x6E1C, 0xD040, 0x303B, 0x0006, 0x4EFB,
        0x0002, 0x0006, 0x000A, 0x000E, 0x7001, 0x4E75, 0x7002,
        0x4E75, 0x70FF, 0x4E75, 0x7000, 0x4E75,
    )
    rom[0x300:0x326] = b"".join(word.to_bytes(2, "big") for word in switch_words)   # synthetic switch
    rom[0x326:0x336] = b"DATA" * 4
    body = rom[0x200:]
    struct.pack_into(">H", rom, 0x18E, sum(struct.unpack(f">{len(body) // 2}H", body)) & 0xFFFF)
    return bytes(rom)
