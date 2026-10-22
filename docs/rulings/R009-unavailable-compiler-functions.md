# R009 — Functions from an unavailable compiler release become assembly source (2026-10-22)

Context: five adjacent collision queries (REV 02 0x025BCE–0x025FF6, 1,068 bytes:
`EntityFindNarrowCollisionAtPosition`, `EntityFindMenuAwareCollisionAtPosition`,
`EntityFindVolumeCollisionAtPosition`, `ProjectileFindCollisionExcludingOwner`,
`EntityFindHorizontalCollisionAtX`) are compiler-shaped but contain twelve `SUB.W/BPL/NEG.W`
sign tests that MCC68K 3.3K never emits (none in the 51,419 instructions of the 577 matched C
functions measured, nor in 402 probe compilations over six profiles), while MCC68K 4.3D and 4.4
emit that test but differ elsewhere (435 comparisons, no match). Microtec's 4.2D release notes
(March 1992, Known Problems #28) record a defect in exactly this kind of "less than zero"
comparison. The release that built these functions is not available from any public archive
searched (HardCases6, CompilerHunter2).

User: chose "Assembly source" when asked how to finish them.

- Functions shown, with independently reviewed evidence, to come from a compiler release that is
  not available are classified as such (not C for the progress denominator) and are committed as
  readable 68000 assembly source under `src/asm/`, built like the hand-written routines of
  [R007](R007-assembly-sources.md).
- The classification records the evidence; it is revisited if the compiler release turns up.

Erratum (2026-10-22, AsmSmith): the five functions span REV 02 0x025BCE up to 0x025FFA (exclusive);
the last instruction ends at 0x025FF9 and the data that follows starts at 0x025FFA.
