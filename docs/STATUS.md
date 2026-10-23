# Status (current entry first)

## Final — checkpoint c038-final accepted

**Complete.** The final checkpoint passed both independent final reviews
([FinalReviewA](../accept/c038-final/review-FinalReviewA-delta3-review.json): functions, bytes,
REV 00, acceptance; [FinalReviewB](../accept/c038-final/review-FinalReviewB-delta3-review.json):
readability, qualifications, documentation), each PASS with disclosed qualifications and no
blocking finding. [Preparation](../accept/c038-final/prepare.json) bound two fresh
byte-identical builds of each revision and the full suite (191 tests);
[finalization](../accept/c038-final/finalize.json) rebound every credited function to the final
sources.

- **Code:** 598 of 598 C functions match in both revisions (REV 02 174,344 bytes, REV 00
  174,232 bytes); 36 hand-written routines, six runtime helpers, the five R011 collision
  queries and the Z80 driver are readable assembly source; zero generated assembly.
- **Data:** every byte comes from a typed, named unit extracted from the owner's ROM, from the
  Z80 source, or from the matched C's own string literals; six bytes are typed `unreferenced`
  ([R012](rulings/R012-closure-interpretations.md)).
- **Readability:** the final review rounds (c034 to c037, recorded as failed or superseded)
  replaced the remaining record offsets with struct fields, gave every established domain
  identity a named constant (including the new `include/dialogue_ids.h`), added the missing
  `MATCHING QUALIFICATION:` markers, and split `src/presents/present_dispatch.c` from
  `present_effects.c`, which sat at MCC68K's DOS heap limit. The
  [qualification ledger](QUALIFICATIONS.md) covers all 98 markers.
- **Revisions:** REV 00 builds from the same source; its eight logic differences are the
  `#if REVISION` sites listed in [REVISIONS](REVISIONS.md).

## Completion — checkpoint c033: 598 of 598 C functions matched

**Achieved.** Both retail revisions rebuild byte-identically from the repository's
readable source and assets extracted from the owner's ROM. There is no generated
assembly left in either build.

- **C complete:** 598 of 598 functions in each revision; REV 02 is 174,344 of
  174,344 C bytes and REV 00 is 174,232 of 174,232 C bytes (**100% each**).
  `PlayerProcessTerrainMotion`, matched in c032, completes the seven-function
  `src/player/entity_motion_terrain.c` translation unit.
- **Every other routine is source:** 36 hand-written functions, six runtime helpers,
  five collision queries, and three fragments are readable 68000 assembly; the Z80
  driver and bootstrap are readable Z80 assembly. The collision queries are in
  `src/asm/collision.s` under [R011](rulings/R011-unreproducible-functions.md), not C credit.
- **Denominator r12:** 645 functions = 598 C + 36 hand-written + six runtime helpers
  + five classified `not_reproducible_c`. R011 records the reviewed reproduction
  evidence without claiming an established historical compiler/source mechanism.
- **All data typed:** no raw data bytes remain; six bytes without a known reader are
  explicitly typed `unreferenced` ([data accounting](DATA.md)).
- **Readable and disclosed:** canonical function/global/field names, shared records and
  constants, bounded variable-frame views, [subsystem guides](subsystems/README.md),
  and a [qualification ledger](QUALIFICATIONS.md) accompany the matching source.
  Both revisions share the source; REV 02 has [eight logic fixes](REVISIONS.md).

**Recorded verification.** The [c033 preparation](../accept/c033-collision-assembly/prepare.json)
is PASS with the suite OK and the two fresh builds per revision bound into the accepted
record. The [independent review](../accept/c033-collision-assembly/review-C033Review-review.json)
records byte-identical builds, zero generated instructions/function/fragment units,
191 passing tests with no failures, errors, or skips, denominator verification, and
a public-history audit. Its verdict is pass with disclosed qualifications, including
the finite scope of the collision compiler evidence. The
[c033 finalization](../accept/c033-collision-assembly/finalize.json) binds the final
images for both revisions to the reviewed inputs. The gates are described in
[ACCEPTANCE](ACCEPTANCE.md).

Required matching/source/data work is complete. [RESUME](RESUME.md) now describes how
to rebuild and verify the result; further readability polish is optional.

## 2026-10-22 (evening) — 597 of 603 C functions matched; hand-written code and data from source

**Result.**
- **Credited C:** 597 of 603 C functions in each revision from the same source (checkpoints
  through c030): 173,328 of 175,412 bytes in REV 02 and 173,216 of 175,300 bytes in REV 00
  (98.8% each). Every function has a canonical name. Shared headers name record fields,
  gameplay state, hardware/layout constants, and bounded sprite-frame access views; those
  views do not establish fixed ROM-frame extents. Sources live in subsystem folders, with
  completed translation units merged into shared files; `docs/subsystems/` explains each
  subsystem with notes for a native port.
- **Hand-written code as source (ruling R007):** 42 routines plus three fragments (start-up,
  interrupts, VDP/DMA, RLE unpackers, controllers, Z80 bus and sound interface, MCC68K runtime
  helpers) are readable 68000 assembly in `src/asm/`, and the Z80 sound driver and boot stub
  are Z80 source in `src/z80/`, assembled in place of generated/extracted units; all reviewed.
- **Data:** no raw bytes left in either revision; 6 bytes without a reader are typed
  `unreferenced` with evidence (`docs/DATA.md`).
- **Denominator r11:** 645 functions = 603 C + 36 hand-written + 6 runtime helpers.
- **Qualified matches:** disclosed compiler-coaxing spellings remain marked by
  `MATCHING QUALIFICATION:` comments and reviewed as qualifications, not historical source.
- **Open:** 6 C functions: `PlayerProcessTerrainMotion` and five collision queries whose
  classification awaits a user decision (see `docs/RESUME.md`).

## 2026-10-22 (morning) — 71% of the C credited; both revisions' data from assets; shared headers

**Result.**
- **Credited C (REV 02):** 419 of 606 C functions, 125,136 of 175,502 bytes (71.3%), every one
  also matching REV 00 from the same source (checkpoints through c016). All original translation
  units have had an author pass; ~50 functions in the tree's waiting room (waves 24-29, hard cases)
  await their checkpoint, and ~25 still resist.
- **Denominator r9:** 28 more routines classified hand-written with high confidence (corpus
  statistics over the matched C plus MCC68K probes, independently reviewed), so the C
  denominator is 606 functions; catalogue r5/r6 fixed switch functions split by imported seeds
  and two retry loops glued to the sound wrappers.
- **Shared headers:** `include/` holds 37 per-subsystem K&R headers (one definition per record,
  proven return types); every source includes only what it uses, which also keeps MCC68K inside
  its DOS heap. The 27 per-file declaration exceptions each fail with the shared declaration.
- **Data:** REV 00 now also builds every data byte from extracted assets (`docs/DATA.md`).
- **Public-repo audit:** `tools/audit_public.py` runs in CI ROM-free and before every push with
  the ROMs; its known gaps are listed in `docs/AUTHOR_TOOLS.md`.
- **In flight:** last header migration (waves 24-29, hard cases), naming of the ~430 unnamed
  functions and the RAM/data spellings, a third hard-cases pass.

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
