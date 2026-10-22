# Rulings

User decisions, one file each, never edited after the fact (a later ruling supersedes).

| Id | Date | Decision |
|---|---|---|
| [R001](R001-targets.md) | 2026-10-21 | REV 02 (the user's Rev A download) is the primary target; REV 00 is kept as a secondary version |
| [R002](R002-bytematch.md) | 2026-10-21 | Track progress on ByteMatch "as soon as the repo is up", like the previous projects |
| [R003](R003-existing-work.md) | 2026-10-21 | Use the existing disassemblies (tje-disasm, RyanFae): import names, RAM layout, structures, formats with attribution; never paste their source |
| [R004](R004-subagents.md) | 2026-10-21 | Sub-agents: `openai-codex/gpt-6.1-sol` max only, two at a time to start; roles chosen by Main; fresh reviewers |
| [R005](R005-subagent-cap-4.md) | 2026-10-21 | Sub-agent cap raised to four at once (supersedes R004's two) |
| [R006](R006-compiler-acquisition.md) | 2026-10-21 | Acquire the archive.org Microtec MCC68K 3.3A set and, standing, other proprietary/abandonware 68k compilers from public archives when evidence points at them; private cache only, recorded with hashes |
| [R007](R007-assembly-sources.md) | 2026-10-22 | Hand-written 68000 routines and the Z80 driver are committed as readable assembly source (`src/asm/`, Z80 source), assembled in place of generated/extracted units |
| [R008](R008-subagent-cap-8.md) | 2026-10-22 | Sub-agent cap raised to eight at once (supersedes R005's four) |
| [R009](R009-unavailable-compiler-functions.md) | 2026-10-22 | Functions shown (reviewed evidence) to come from an unavailable compiler release are classified as such and committed as readable assembly source, like R007 |
| [R010](R010-subagent-cap-16.md) | 2026-10-22 | Sub-agent cap raised to sixteen; while the user is away, decisions are queued, not asked (supersedes R008) |
