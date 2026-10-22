# R006 — Acquiring proprietary and abandonware compilers (2026-10-21)

Context: the Archaeologist lane found that Sega of America's developer bulletin #1 (1991-03-13)
supports Microtec C (MCC68K) with ASM68K, and that the September 1991 MCC68K manual documents the
conventions observed in the ROM (LINK frames only when needed, A7-relative arguments, D0/D1/A0/A1
volatile). An archive.org item, `mri-68-k-c-cross-compiler-toolchain` (Microtec MCC68K 3.3A +
ASM/LNK 6.6A, 1990), holds a matching-era copy.

User decision ("Yes, and other leads too"):

- Download the archive.org MCC68K set into the private, git-ignored cache and run it under a DOS
  emulator to test it against the ROM.
- Standing approval to fetch other proprietary or abandonware 68000 compilers from public archives
  (archive.org, hiddenpalace, winworld and similar) when the evidence points at them.
- Every acquisition is recorded with its source URL, sizes and sha256 (`toolchain.lock.json` once
  pinned, `docs/COMPILER.md` while being tested). These tools are never committed or redistributed.
