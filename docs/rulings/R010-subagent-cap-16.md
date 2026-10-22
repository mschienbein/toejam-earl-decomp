# R010 — Sub-agent cap raised to sixteen (2026-10-22)

User: "up the sub agents to 16, im gonna walk away fromthis fro a few hours so just keep rolling
through and keep any decisions for when i return".

Supersedes the concurrency limit in [R008](R008-subagent-cap-8.md): at most sixteen
`openai-codex/gpt-6.1-sol` (max effort) sub-agents at once, nested sub-agents included. While the
user is away, work continues and decisions that need the user are queued for their return
instead of asked. Everything else in R004 stands.
