# Writing matching C

How a function goes from generated assembly to credited C. Compiler facts: `docs/COMPILER.md`;
acceptance rules: `docs/ACCEPTANCE.md`.

## Loop

1. **Pick work.** Prefer a whole original translation unit: `config/rev02/tus.json` lists the
   spans that PC-relative calls tie together (the original C files, at least in part). Functions
   outside every span can be done alone. Leaf functions first; `config/rev02/classification.json`
   says which functions are compiled C.
2. **Read it.** `uv run python tools/packet.py <name>` writes a private packet (assembly with names,
   callers, callees, switch tables, argument hints) to `scratch/packets/`. Never commit packets.
3. **Write C** (rules below) in a file of your own outside `src/` until it matches.
4. **Match.** `uv run python tools/match.py path/to/file.c` compiles it with MCC68K and compares
   every function in the file with the original bytes at its address: `MATCH`, or a side-by-side
   diff at the first difference. `--version rev00` checks the other revision; `--keep DIR` keeps the
   compiler listing and object. `tools/diff.py` compares an object you built yourself.
5. **Hand over** the file; Main places it under `src/<subsystem>/`, lists it in
   `config/sources.json` and takes it through a checkpoint (`tools/accept.py`).

## Rules

- **K&R C only.** No prototypes (`extern short f();`), no `signed`, no `volatile`; old-style
  parameter lists (`short f(a) short a; { ... }`). Default compiler options; no `#pragma`, no
  inline assembly, no byte or word arrays standing in for code.
- **Canonical names.** A function or global is spelled with its name in `config/rev02/names.json`
  when it has one, else with its REV 02 address: `func_XXXXXX` (code), `dat_`/`rom_XXXXXX` (ROM
  data), `ram_XXXXXX` (RAM). Address spellings always mean REV 02 addresses, also when building
  REV 00. The functions of one file must be one contiguous run in ROM order.
- **Declarations** of other files' functions and globals are `extern` with the type the code
  shows (return width matters: a `char` function's callers read `d0.b`). Shared declarations move
  to `include/` when Main consolidates them; propose additions with your file.
- **Natural code.** Write what a 1991 programmer would have written. If only a contortion matches
  (a pointless temporary, an odd cast, a `goto`), say so in a comment and keep looking for the
  plain spelling; reviewers check.
- **Revision differences.** Where REV 00 and REV 02 were built from different source, write
  `#include "revision.h"` and guard the difference with `#if REVISION >= 2` (REV 00 has
  `REVISION 0`); the header is generated for each compile. Check both revisions with match.py.
- **Comments state evidence.** Describe what the function does as observed (callers, data it
  touches); mark guesses as guesses. Names for locals and parameters should say what they hold.

## When it will not match

- Register allocation or instruction order differs: try declaration order, `register` on locals,
  splitting or merging expressions, `short` versus `int` temporaries, `unsigned` comparisons.
- A call is absolute where the original is PC-relative (or the reverse): the callee belongs (or
  does not belong) to the same original file; check `tus.json`.
- A branch differs in size: forward branches are always word-sized, backward ones shortest (see
  the assembler section of `docs/COMPILER.md`); the code before or after the branch differs.
- Record what you tried. A function that resists every plain spelling may be hand-written
  assembly; the classification records the evidence for that, never a guess.

## Idioms observed so far

Each was seen to decide a match in this game; none is a general law of MCC68K.

- **Bounded arrays index with words.** `extern char table[80];` gives a sign-extended word index
  and `.w` addressing; an unsized `table[]` or a pointer gives long index arithmetic, and a `(short)`
  cast does not substitute for the bound.
- **Take a record pointer once.** `p = &defs[i];` then `p->a`, `p->b` matches where repeating
  `defs[i].a`, `defs[i].b` recomputes the address each time.
- **Initialisers and assignments order code differently.** Initialising locals in their
  declarations places those stores before hoisted address loads; plain assignments after the
  declarations follow source order.
- **Operand order of commutative operators shows.** `a == b` and `b == a` (likewise `^`, `+`)
  load in different orders.
- **Byte temporaries keep byte code.** A named `char` temporary and an `if/else` avoid the int
  widening of a `char`-valued `?:`.
- **Keep parameters unmodified** when the original copies them: separate locals for derived values
  allow in-place shifts on registers.
- **`register` is rarely needed.** Qualifying several locals changes allocation; in the one case
  seen so far the hint turned out unnecessary on review, so try without it first.
- **Literal types pick scratch registers.** `long` vs `unsigned long` operands and literals can move
  a test from `d1` to `d0`.
