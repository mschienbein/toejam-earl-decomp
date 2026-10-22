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
- **String literals.** The ROM keeps each function's strings right after it in the code section
  (e.g. REV 02 `0x009D1A`), which is MCC68K's `/STRINTEXT` (Sega's 1991 bulletin shows it in
  Microtec's examples). `tools/mcc.py` always passes it; it does not change functions without strings.
- **Compiler profile.** Default options reproduce all 13 functions of the first checkpoint
  (switches with two tables, helper calls, function pointers, framed functions with locals, the
  Park-Miller random generator). Observed by the CompilerHunter: `/optimize=time` or `=all` inlines
  the multiply partial products and reshapes loops; disabling register allocation, peephole or
  combined stack pops, or forcing debug frames, breaks the matches. These constrain the profile; the
  exact original command line and patch release are not uniquely proved (nearby releases may emit
  identical code).
- **Library memory model.** The helper modules are identical in the absolute, A5-relative and
  PC-relative 3.3K libraries, so they do not tell which one was linked.
- **Rejected alternatives.** Locally built GCC 1.37.1 and 1.40 and Sozobon hcc/TOP fail the switch
  and arithmetic-call conventions; released GCC 1.41+/2.x postdate the ROMs (CompilerHunter).

## Assembler: ASM68K 6.6A, reproduced with GNU as

Measured by running ASM68K 6.6A under emu2 (CompilerHunter receipts) and confirmed on the ROM;
`tools/mcc.py` applies these rules when adapting the compiler's assembly, never changing an
instruction:

- An unsized `Bcc`/`BRA`/`BSR` to a label not yet defined (forward) reserves a word displacement
  even when a byte would fit; to a label already defined (backward) it takes a byte when it fits
  (−128 included), else a word. Explicit `.S`/`.W` are kept.
- A bare `JSR`/`JMP`/`PEA`/`LEA` of a label in the same section of the same file becomes PC-relative: 4 bytes
  backward; forward, the 6 bytes reserved for an absolute call stay used as PC-relative + `NOP`.
  Targets in other files, RAM or the runtime stay absolute long. All 521 PC-relative `JSR`s in REV 02
  follow this: the 94 forward ones are each followed by a `NOP`, no backward one is. ASM68K's
  `-f OPNOP` option removes the padding, so the game was assembled without it.
- Translation units: because only same-file calls become PC-relative, the PC-relative references
  between functions recover the original C files' extents (`tools/tus.py`, `config/<v>/tus.json`:
  50 spans, 500 functions). A C file holding part of such a span calls the rest of it PC-relative,
  exactly as the original assembler did.

## One source, two revisions

REV 02 is primary. Every function keeps its REV 02 identity in REV 00 (`tools/revmap.py`: the 653
functions pair one-to-one, 435 identical after masking addresses); an unnamed function is
`func_<REV 02 address>` in both. Address-spelled names in C (`dat_`, `rom_`, `loc_`) always mean
REV 02 addresses; the REV 00 build moves them to the same offset inside the corresponding function
or data range. The first 13 matched functions compile to both revisions byte for byte.

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

## Candidates considered (1990–91, 68000 targets)

Closed 2026-10-21 by the Microtec identification; kept as the record of what was tried.

| Compiler | Host | Outcome |
|---|---|---|
| **Microtec MCC68K 3.3K** | DOS | **identified**: functions and runtime reproduced byte for byte |
| Sierra Systems 68000 C | DOS | not needed; bstreiff's v3.1b (1996) comparison: its `div32` differs |
| GCC 1.37.1, 1.40 | Unix | built locally; switch lowering and arithmetic-call convention differ; 1.41+/2.x postdate the ROMs |
| Sozobon hcc 2.0 / TOP 2.00 | Atari ST/Unix | built locally; differs |
| Lattice/SAS, Aztec, Think/MPW, Alcyon, Pure, Megamax, Green Hills, Whitesmiths, Mark Williams | various | not pursued after the identification |

Full matrix, receipts and commands: the CompilerHunter lane's report (private scratch).
