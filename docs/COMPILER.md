# Compiler identification

Matching C needs the compiler (and version, options, runtime) that built the game. This file collects
the evidence; each claim is observed in the ROM or cited, else marked [INFERENCE].

## Identified: Microtec Research MCC68K (2026-10-21)

- **Byte-identical reproduction.** C written independently for two REV 02 functions, compiled by
  Microtec MCC68K **3.3K** (default options) under the free DOS emulator emu2, with the compiler's
  assembly output passed to GNU `as` after directive-only adaptation (sections, `XDEF`/`XREF`; no
  instruction changed) and linked at the original address, reproduces both exactly:
  `0x0011CA` (14 bytes: a `short` count-down loop) and `0x001262` (36 bytes: `(x - 1) % 6` on a
  `short`, including the native `divs` with a `bvc` fallback to the `lmodt` helper at `0x027154`
  and the redundant `move.w d0,d0` sign-extension sequence seen throughout the ROM). First found by
  the CompilerHunter lane, reproduced independently by Main with different source.
- **Historical fit.** Sega of America's Genesis Technical Bulletin #1 (1991-03-13) corrects the
  Microtec C examples shipped with the development kit (`TESTC68K.bat`, `asm68k`, `LNK68K`,
  option `STRINTEXT`); the Microtec 68000 reference manual of September 1991 documents the observed
  frame policy, preserved/volatile registers and argument cleanup.
- **Toolchain source.** archive.org `mri-68-k-c-cross-compiler-toolchain` (MCC68K 3.3K + ASM68K
  6.6A, 1989–90), acquired under ruling R006; private cache only.
- **Runtime library: the stock MCC68K 3.3K 68000 runtime.** Parsed properly as IEEE-695 objects
  (load records split instructions, so a raw byte search of the `.LIB` files finds nothing), the
  3.3K libraries `MCC68KAB.LIB`, `MCC68KA5.LIB` and `MCC68KPC.LIB` contain all five ROM helpers
  byte for byte: `abs` (`0x02714A`), `lmodt` (`0x027154`), `ldivt` (`0x0271B2`), `ulmult`/`lmult`
  (`0x027214`), and `ulmodt`/`uldivt` (`0x027240`/`0x027248`, one body). Found by the Archaeologist
  lane; the disks were manufactured 1990-03-05, the release notes are dated 1989-07-28.
- **Still open:** the options used for the whole game (3.3K with defaults reproduces both probes) and
  the assembler's branch sizing (the ROM has 235 forward word branches that would fit a byte
  displacement, as a one-pass assembler sizes forward references; ASM68K 6.6A is on the same disks).

## Writing C for MCC68K 3.3K

- **K&R C.** Observed 2026-10-21: function prototypes (`int f(short a);`), the `signed` keyword and
  `volatile` are rejected; old-style definitions (`short f(a) short a; { ... }`) and `void` work.
  Declare functions as `extern short f();`. Headers for game code must use only K&R constructs
  (typedefs, structs, enums, `unsigned`); host-side layout checks live in separate files.
- **Identifiers.** The compiler prefixes C names with a dot (`.name`); `tools/mcc.py` removes it,
  so C functions and globals are spelled with the project's symbol names.
- **Options.** Default options reproduce every function matched so far. Options are written with a
  leading slash (`/optimize=time`); a bare word becomes a listing file name.

## Evidence that the game is compiled C

- REV 00 → REV 02 changes register allocation in the function at REV 02 `0x27A10` the way a compiler
  does after a small source edit, not the way a programmer patches assembly
  ([tje-disasm compiler-hunting.md](https://github.com/bstreiff/tje-disasm/blob/master/documentation/compiler-hunting.md)).
- One `switch` idiom throughout (24 tables in REV 02, recognised by `tools/analyze.py`):

  ```
  cmpi.w  #N,d0            ; highest case
  bgt     default          ; (bhi when unsigned)
  add.w   d0,d0
  move.w  T-2k(pc,d0.w),d0 ; k = lowest case: 1-based switches index from 2 bytes before T
  jmp     T(pc,d0.w)
  T:      .short case_k-T, ..., case_N-T
  ```

- Function entries in REV 02 (651 catalogued functions, first instruction): `movem.l …,-(sp)` 345,
  `move` 146, `link a6,#-n` 91 (89 of them followed by `movem.l`), others 69. Frames are built only
  when a function has locals; otherwise arguments are read at `n(sp)` after the register save.
  [INFERENCE] This "frame pointer only when needed" policy is a compiler trait worth matching first.
- 32-bit arithmetic helpers (the 68000 has no 32-bit multiply/divide), REV 02 addresses from
  tje-disasm's labels: abs `0x02714A`, signed mod `0x027154`, signed div `0x0271B2`, multiply
  `0x027214`, unsigned mod `0x027240`. Their exact code is the strongest fingerprint of the
  compiler's runtime library.

## Candidates (1990–91, 68000 targets)

| Compiler | Host | Notes | Status |
|---|---|---|---|
| Sierra Systems 68000 C | DOS | Licensed to Sega for its official SDK ([Sega Retro](https://segaretro.org/Sierra_68000_C_Compiler)). bstreiff compared v3.1b (1996 Sega Channel disc): its `div32` differs. Earlier versions (1.x/2.x) not located | open: find a 1990–91 version |
| GCC 1.3x–1.40 | Unix | m68k back end mature; libgcc `__divsi3` etc.; sources archived | open: build 1.37/1.40 cross, compare helpers and prologues |
| Lattice / SAS C 5.x | Amiga | 1990 v5.10 documented | open |
| Aztec C (Manx) 5.x | Amiga/Mac/DOS-cross | | open |
| Microtec MCC68K | DOS/Unix | common embedded 68k cross-compiler | open |
| Think C / MPW C | Mac | [INFERENCE] plausible if developed on Macs | open |
| Alcyon, Pure/Turbo C, Megamax/Laser C | Atari ST | | open |
| Green Hills, Whitesmiths, Mark Williams | various | | open |

## Method

1. For each obtainable compiler: build or run it (DOS ones under an emulator), compile reference
   C for the helpers and a few small leaf functions, and compare prologue/epilogue, register
   allocation order, `switch` lowering and the runtime helper bytes with the ROM.
2. A candidate that reproduces a helper byte for byte is then tried on real game functions.
3. Record every candidate tried with its version, flags and result here; failures are evidence.

Until a compiler is identified, structural work continues without it: names, types, RAM map, data
formats and splitting the assembly into readable modules.
