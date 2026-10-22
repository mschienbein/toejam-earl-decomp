# Status (current entry first)

## 2026-10-21 (evening) — goal run started: ByteMatch live, four lanes, names and denominator plumbing

**Result.**
- ByteMatch registered and fully configured (`docs/BYTEMATCH.md`): platform id `md`, IGDB game
  8389, art, GitHub App on the repository; first ingested commit `4debbc9` (0 / 651 functions).
- Rulings R003 (import the existing disassemblies' names, layouts and formats with attribution),
  R004 (sub-agents: `openai-codex/gpt-6.1-sol` max), R005 (up to four at once).
- Four sub-agent lanes running (log: genesis-experiments `records/tje/subagent-log.jsonl`):
  CompilerHunter (fingerprints + free candidates), Cartographer (names, RAM/struct headers, typed
  data units, classification), Toolsmith (`audit_public.py`, `packet.py`, `diff.py`), Archaeologist
  (runtime-helper survey across the Genesis library, documentary evidence, DOS emulator).
- Main: `tools/names.py` + build/report support for imported names (labels, RAM and ROM-data
  symbols); `tools/denominator.py` hash-chained denominator history, r1 recorded (catalogues only)
  and verified in CI. 23 tests pass; both revisions still PASS. Pending an independent review before
  push.

**Observed (Main).** The 32-bit helpers take their operands in `d0`/`d1`, return in `d0`, use
`a0`/`a1` as scratch and preserve `d2`: a register calling convention, unlike GCC's stack-based
`__mulsi3`/`__divsi3`; unsigned div/mod share one body selected by a flag in `a0`
(`0x027240`/`0x027248`). [INFERENCE] GCC 1.x/2.x with its stock runtime is unlikely.

## 2026-10-21 — harness milestone 1: both revisions rebuild byte-identically

**Result.** `tools/build.py` rebuilds REV 02 and REV 00 byte-identically from generated GNU assembly
in about 3.4 s each; every catalogued instruction is readable assembly (0 raw words), all 24
switch tables are symbolic. Matched C: 0 functions (no compiler yet).

| | REV 02 | REV 00 |
|---|---:|---:|
| functions (+ fragments) | 651 (+2) | 651 (+2) |
| instructions | 56,970 | 56,943 |
| code bytes (share of 1 MiB) | 180,798 (17.24%) | 180,686 (17.23%) |
| switch tables | 24 | 24 |
| seeds | 67 | 67 (carried over by signature) |
| image sha256 | `65ea2386…` = pinned | `2bdfb89b…` = pinned |

**How the coverage was reached.** Recursive descent from the vectors alone found 46,402
instructions; recognising the `switch` idiom raised that to 49,576 with no conflicts or invalid
decodes. Rebuilding [bstreiff/tje-disasm](https://github.com/bstreiff/tje-disasm) with our binutils
(byte-identical REV 02) gave its code map: 99.3% of our instructions were code there too, and it
spelled 7,394 more instructions as code (functions reached only through pointers, e.g. the
per-character AI handlers). Seeding the analyzer with the starts of those runs (iterated to a
fixpoint, 67 addresses) closes the gap completely: every instruction tje-disasm identifies is now in
our catalogue, plus 342 it builds through macros.

**Verification.** `python -m unittest discover -s tests`: 17 tests, including synthetic-ROM
round-trips of awkward encodings and of a deliberately wrong spelling (falls back to raw words, still
PASS), and both real-ROM end-to-end builds with a committed-catalogue freshness check.

**Open.**
- Compiler unidentified (`docs/COMPILER.md`) — gates all C work.
- The Z80 sound driver (`0x03F06A`, 1,294 bytes) and all data are `.incbin` units; no data formats
  are modelled yet.
- Function boundaries come from call targets and seeds; tail-call or shared-epilogue cases may
  still split or merge functions (two fragments remain).
- Names: none committed. tje-disasm's 237 descriptive labels are not imported (no licence; needs a
  ruling).

## Next

1. Compiler hunt: obtain candidate compilers (Sierra 68000 C early versions, GCC 1.3x/1.40, Lattice
   5.x, Aztec, Microtec) and compare the runtime helpers at `0x027154`–`0x027240` and the
   prologue/switch conventions.
2. RAM map and structures (tje-disasm `include/*.h` documents player state, presents, earthlings —
   facts to re-derive).
3. `tools/packet.py`-style function packets and an objdiff/asm-differ setup for authors once C
   compiles.
