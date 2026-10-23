# R011 — Functions not reproducible from plain C become assembly source (2026-10-22)

Context: [R009](R009-unavailable-compiler-functions.md) sent the five collision queries
(`EntityFindNarrowCollisionAtPosition`, `EntityFindMenuAwareCollisionAtPosition`,
`EntityFindVolumeCollisionAtPosition`, `ProjectileFindCollisionExcludingOwner`,
`EntityFindHorizontalCollisionAtX`; REV 02 0x025BCE up to 0x025FFA) to assembly source on the
grounds that an unavailable compiler release built them. The independent classification review
(ClassReview3) confirmed the anomaly — twelve `SUB.W/BPL/NEG.W` sign tests, none in the
51,419 instructions of the matched C or in 438 fresh MCC68K 3.3K probe functions — but showed the
same pattern also arises from 3.3K C with inline assembly, so the mechanism (another release,
inline assembly, or hand-edited output) is not established. A C reconstruction with an
inline-assembly absolute-value macro reproduced the sign tests but not the surrounding code.

User: chose "Assembly source" again, for the broader classification.

- Supersedes R009's attribution: functions shown with independently reviewed evidence to be **not
  reproducible from plain C by the pinned compiler** (mechanism unresolved) are classified as such,
  are not part of the C progress denominator, and are committed as readable 68000 assembly source
  under `src/asm/`, built like the hand-written routines of [R007](R007-assembly-sources.md).
- The classification and the source's header record the evidence and the open mechanism; it is
  revisited if a compiler release or source form that reproduces them turns up.
