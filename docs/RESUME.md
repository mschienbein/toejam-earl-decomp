# Resume

Matching/source completion is finalized through checkpoint c033. Both revisions have
**598 of 598 C functions matched (100%)**: REV 02 is 174,344 of 174,344 C bytes;
REV 00 is 174,232 of 174,232 C bytes. Every other routine is readable assembly,
there is zero generated assembly, and data comes from typed assets, except the Z80
driver (Z80 source, R007) and 43 strings that the matched C emits from its own string
literals. Six bytes without a known reader are explicitly typed `unreferenced`, not raw
data; ruling [R012](rulings/R012-closure-interpretations.md) closes both cases.

## Build from a fresh checkout

```sh
git clone https://github.com/mschienbein/toejam-earl-decomp.git
cd toejam-earl-decomp
uv sync
uv run python tools/setup.py
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World) (Rev A).zip"
uv run python tools/rom.py import --rom "<path>/ToeJam & Earl (World).zip"
uv run python tools/build.py
uv run python tools/build.py --version rev00
```

The setup pins binutils 2.47, emu2, and Microtec MCC68K 3.3K through
`toolchain.lock.json`. Supply your own legally obtained dumps; ROMs, extracted
assets, and build outputs remain private and uncommitted. PASS means the complete
image equals the revision's original hash in `config/title.json`, not merely that
individual source functions compile.

## Verify the result

```sh
uv run python tools/setup.py --check
uv run python tools/analyze.py --check
uv run python tools/analyze.py --version rev00 --check
uv run python tools/denominator.py verify
uv run python -m unittest discover -s tests
uv run python tools/audit_public.py --history
```

Importing both dumps enables the real-ROM tests as well as the synthetic tests.
The accepted c033 records establish byte-identical builds of both revisions with
zero generated instructions/function/fragment units and no raw data bytes. The
[independent review](../accept/c033-collision-assembly/review-C033Review-review.json)
records all 191 tests passing, with no failures, errors, or skips; it also records
the denominator and public-history checks. The
[preparation](../accept/c033-collision-assembly/prepare.json) and
[finalization](../accept/c033-collision-assembly/finalize.json) bind the accepted
source inputs and images. See [ACCEPTANCE](ACCEPTANCE.md) for the gates and
[QUALIFICATIONS](QUALIFICATIONS.md) for the disclosed source/ABI spellings.

## Where things are

- `src/` contains the subsystem C sources; `src/asm/` contains readable 68000
  assembly, and `src/z80/` contains the sound driver and bootstrap.
- `src/player/entity_motion_terrain.c` is the completed seven-function terrain
  motion/proximity TU, including `PlayerProcessTerrainMotion` and both installed
  player callbacks. All C functions are matched.
- `src/asm/collision.s` contains the five queries classified `not_reproducible_c`
  under [R011](rulings/R011-unreproducible-functions.md). They are excluded from C
  progress under denominator r12, built from readable assembly, and not attributed
  to an established historical compiler or source mechanism.
- `include/` contains shared K&R interfaces, record layouts, RAM aliases, and named
  constants. `include/sprite_frame_views.h` contains bounded variable-frame access
  views, not fixed ROM-frame allocations or stride types.
- `config/rev02/catalogue.json` and `config/rev00/catalogue.json` hold the code maps;
  `config/rev02/names.json` holds canonical identities, and
  `config/rev02/classification.json` records the classification evidence. Per-revision
  configuration also describes typed data, seeds, TU spans, and correspondence.
- `config/matches.json` records accepted C matches; `docs/denominator-history.jsonl`
  records r12 and its predecessors; `accept/` holds immutable checkpoint records.
- [Subsystem guides](subsystems/README.md) describe data, flow, preserved original
  bugs, and native-port considerations. [REVISIONS](REVISIONS.md) records the eight
  REV 02 logic fixes; [DATA](DATA.md) explains asset typing and the six unreferenced
  bytes; [STATUS](STATUS.md) retains the completion evidence and earlier history.

## Optional future readability

No matching, assembly-source, or data-typing work remains. Further semantic naming
or explanatory polish is optional, not an unfinished implementation checklist.
Future edits must preserve byte identity and retain the reviewed qualifications
and explicitly documented original behavior.
