# R003 — Use the existing disassembly work (2026-10-21)

User: "lets use the existing decomp work and begin getting this worked on".

- Import from [bstreiff/tje-disasm](https://github.com/bstreiff/tje-disasm) (REV 02) and
  [RyanFaeScotland/toejamandearldisassembly](https://bitbucket.org/RyanFaeScotland/toejamandearldisassembly)
  (REV 00): names of functions, labels and RAM variables, the RAM layout, structure layouts and
  data-format knowledge.
- Attribute both projects in `README.md` and record per-entry provenance (`config/names.json`
  `source` fields; comments in headers derived from their structure notes).
- Never paste their source files verbatim: no copied assembly, comment blocks or headers. Names,
  addresses, sizes and formats are restated in this project's own files.
- Supersedes the "needs a ruling" note in `AGENTS.md` (private material) for these two projects.
