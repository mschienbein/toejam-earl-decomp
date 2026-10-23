# How the game works, by subsystem

One document per source folder (`src/`), written from the matched sources and reviewed
against them (DocSmithA/DocSmithB, DocReview). Each describes the files, data structures and RAM
state, the main flows with function names, interactions with other subsystems, every
`ORIGINAL BUG:` the sources preserve, open questions, and **Port notes**: what a native port
would replace or rework (hardware access, shared-screen coupling such as the global pause behind
the map and inventory, determinism of the random number generator and frame timing, content
tables that could become data, and what a wider camera would change).

| Document | Folder | Covers |
| --- | --- | --- |
| [core.md](core.md) | `src/core/`, `src/asm/startup.s`, `src/asm/interrupts.s` | reset, interrupts, main loop and game scheduler |
| [platform.md](platform.md) | `src/asm/` | 68000 assembly: VDP/DMA, decompressors, controllers, runtime helpers, collision queries |
| [camera.md](camera.md) | `src/camera/` | camera following, streamed tile rows/columns, split screen |
| [sprites.md](sprites.md) | `src/sprites/` | entities, sprite slots and tiles, drawing order, visibility |
| [map.md](map.md) | `src/map/` | world generation, terrain, level changes, random numbers |
| [player.md](player.md) | `src/player/` | player movement, jumping, falling, abilities |
| [earthlings.md](earthlings.md) | `src/earthlings/` | Earthling banks, behaviour, targeting, contact |
| [presents.md](presents.md) | `src/presents/` | present identities and effects, world items, telephones, projectiles |
| [elevator.md](elevator.md) | `src/elevator/` | elevators and level rides |
| [ship.md](ship.md) | `src/ship/` | ship pieces and the ship screen |
| [ui.md](ui.md) | `src/ui/` | HUD, inventory, text, windows, menus, pause |
| [speech.md](speech.md) | `src/speech/` | speech bubbles and delayed conversations |
| [audio.md](audio.md) | `src/audio/`, `src/asm/sound*.s`, `src/z80/` | 68000 sequencer, FM, PSG effects, PCM through the Z80 |
| [cutscenes.md](cutscenes.md) | `src/cutscenes/` | Sega logo, title, intro, ending, Funkotron |
| [jamout.md](jamout.md) | `src/jamout/` | Jam Out mode |
