# Toolchain

| Piece | Version | Source | Pinned by |
|---|---|---|---|
| GNU binutils (`as`, `ld`, `objcopy`, `objdump`, `nm`, `readelf`) for `m68k-elf` and `z80-elf` | 2.47 | `https://ftp.gnu.org/gnu/binutils/binutils-2.47.tar.xz`, GPG signature verified (Nick Clifton, `3A24BC1E8FB409FA9F14371813FCEF89DD9E3C4F`) | `toolchain.lock.json` (size + sha256); `.cache/toolchain/installed.json` records every program's sha256 and banner |
| capstone (68000 decoding) | 5.0.7 | PyPI | `pyproject.toml` + `uv.lock` |
| pyelftools (object inspection) | 0.33 | PyPI | `pyproject.toml` + `uv.lock` |
| Python | 3.12 | uv | `pyproject.toml` |

`tools/setup.py` downloads the tarball once into `.cache/downloads/`, verifies size and sha256,
builds each target out of tree (`--disable-nls --disable-werror --disable-gdb --disable-sim
--disable-gprofng --disable-plugins --without-zstd`, `MAKEINFO=true`) into
`.cache/toolchain/binutils-2.47-<target>/`, then records the installed programs. `--check` verifies
the installation against that record. No system-wide installation; 102 s for both targets with
`--jobs 16` (2026-10-21).

## Assembler flags and why

`m68k-elf-as -m68000 --bitwise-or --pcrel -L`

- `-m68000`: reject 68020+ encodings.
- `--bitwise-or`: `|` is an operator, not a comment.
- `--pcrel`: never relax a branch to an undefined symbol into an absolute jump. Without it a
  `dbra` to a label in another unit becomes `dbf; bra.s; jmp` (12 bytes instead of 4).
- `-L`: keep the `.Li_<address>` instruction markers so `tools/build.py` can check each
  instruction's size and bytes in the object.

## GNU as quirks handled by the renderer

| Quirk | Handling |
|---|---|
| capstone prints `moveq`/`link` immediates unsigned (`#$ff`) | rendered signed (`#-1`) |
| gas has no `abcd.b`; capstone appends `.l` to `exg`, `lea`, `btst` | sizes dropped where implied |
| a short branch (`bcc.s`) to a symbol in another unit is range-checked against its own in-section offset and rejected beyond 127/255 bytes | spelled `.reloc .+1, R_68K_PC8, target-1; .byte 0x6c, 0` (explicit relocation) |
| `d8(pc,Xn)` to a symbol in another section has the same problem | switch tables are kept inside their function, so table operands are local |
| capstone reports `d8(pc,Xn)` with `d8 = 0` as the 68020 base-displacement mode | treated as the 68000 brief format |

Any spelling that still does not reproduce falls back to raw `.short` words automatically; the
receipt counts them (`raw_words`, currently 0).

## Cross-check

[bstreiff/tje-disasm](https://github.com/bstreiff/tje-disasm) (cd85d2e), preprocessed with the
host `cpp` and assembled/linked with these binutils, also rebuilds REV 02 byte-identically
(genesis-experiments `tools/tje_disasm_codemap.py`, 2026-10-21).
