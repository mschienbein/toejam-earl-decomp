# Status (current entry first)

## 2026-10-22 — matching waves under way: 105 functions credited, data closed for REV 02

**Result.**
- **Credited C (REV 02):** 105 of 640 functions, 33,132 of 180,364 bytes (18.4%) through
  checkpoints c002–c008, every one also matching REV 00 from the same source; c009 (~54 more,
  including the whole inventory file) is in review. Each checkpoint: two fresh builds of both
  revisions, the suite, an independent review, a fresh-build finalize.
- **Authoring loop at scale:** wave authors take one original translation unit at a time
  (`config/rev02/tus.json`); a reviewer per checkpoint. 9 of ~240 functions tried so far resist
  (register allocation or scheduling); each is recorded with its closest attempt.
- **Compiler profile settled further:** `/STRINTEXT` (strings follow their function, as in the
  ROM); `PEA`/`LEA` of same-file labels are PC-relative like calls; `revision.h` expresses the few
  REV 00 / REV 02 source differences (e.g. DrawGameMap's REV 02-only reset).
- **Data:** REV 02 builds every data byte from assets extracted at build time (4,163 typed units,
  15 raw bytes; `docs/DATA.md`); the progress report counts typed data (99.998%).
- **Corrections found by matching:** catalogue r3 (switch exit jump), REV 00 inventory tables
  start eight entries earlier, ResetVDP is plain C (was classified hand-written).

## 2026-10-21 (night) — compiler identified: Microtec MCC68K 3.3K; matching pipeline built

**Result.**
- **Compiler:** Microtec MCC68K 3.3K (DOS; archive.org copy acquired under ruling R006), default
  options. 12 REV 02 functions of varied shapes (switches, helper calls, function pointers, framed
  functions with locals) reproduced byte for byte from independent C; the five 32-bit helpers are
  the stock 3.3K runtime modules (`abs`, `lmodt`/`ldivt`, `lmult`, `ulmodt`/`uldivt`), byte for
  byte. Assembler behaviour (ASM68K 6.6A) measured and reproduced: forward branches word,
  backward shortest; same-file calls PC-relative (forward + NOP); confirmed on all 521 PC-relative
  calls in the ROM. Language level: K&R (no prototypes, `signed`, `volatile`). `docs/COMPILER.md`.
- **Pipeline:** `tools/setup.py` provisions emu2 (pinned commit) and MCC68K/ASM68K from the
  pinned archive; `tools/mcc.py` compiles and adapts; `tools/match.py` checks one C file strictly;
  C units (`config/sources.json`) replace their catalogued runs in the full build; `tools/accept.py`
  checkpoints (two fresh builds of both revisions + suite, review, finalize into
  `config/matches.json`). `tools/tus.py` recovers 50 original translation-unit spans from
  PC-relative references so partial C files call the rest of their unit the original way.
- **Knowledge:** reviewed names (1,462 symbols per revision; Microtec runtime names primary) and
  classification (530 compiled_c, 19 hand-written assembly, 6 runtime, 2 Sega library, 94 unknown)
  integrated; generated assembly reads with names; denominator r2: 640 C functions, 180,304 bytes.
- **Reviews and fixes.** MatchInfraReview (12 findings) led to: a tokenizing adapter (registers
  `%d0`, C symbols separate; strings byte-exact; no PC-relative calls to data; branches out of a
  file refused), authoritative addresses in `match.py`, toolchain hashes checked before every use,
  and reworked checkpoints (input manifest, FAIL records, fresh-rebuild finalize, per-function
  credit, `config/matches.json` schema 2). Toolsmith's `packet.py`/`diff.py` integrated; its audit
  r3 is under review. DataSmith's data closure (4,164 typed units round-trip) needs an r2 (plan
  size, raw-byte accounting, codec edge semantics).
- **One source, two revisions.** `tools/revmap.py` pairs REV 02 and REV 00 functions one-to-one;
  REV 00 takes REV 02 identities and translates REV 02 address spellings; the 13 matched functions
  build both revisions.
- **Catalogue r3.** The analyzer follows MCC68K's switch exit jump; NewGameMainMenu owns its case
  bodies (denominator 640 C functions, 180,364 bytes).
- **Lanes:** C001Review (first checkpoint), MatchWave1A (first author wave, 18 functions),
  AuditReview3, DataSmith r2.

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
