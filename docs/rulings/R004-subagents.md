# R004 — Sub-agents (2026-10-21)

User: "we can hit this with two gpt-6p1-sol max sub agents at a time to start … figuring out what
roles they play as we go, whatever you think is best".

- Sub-agents run `openai-codex/gpt-6.1-sol` at max effort only, at most two at once until the user
  raises the cap. No other model; on quota limits or an outage, pause or narrow the work and tell
  the user.
- Main (the Anthropic integrator) chooses and evolves their roles. First lanes: CompilerHunter
  (compiler identification) and Cartographer (knowledge import and classification). Later: two
  authors on disjoint functions, or an author and a reviewer.
- A reviewer is always a fresh session that neither authored nor reviewed the item before; Main
  confirms. Every task is logged in the private repository (`records/tje/subagent-log.jsonl`).
