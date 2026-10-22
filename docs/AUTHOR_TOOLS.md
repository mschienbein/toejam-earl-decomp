# Public audit and private author tools

These tools do not credit a function as matched. Full-ROM builds and independent acceptance in
[ACCEPTANCE.md](ACCEPTANCE.md) remain the gate. Never commit function packets or differ output.

## Before every public push

```sh
uv run python tools/audit_public.py --history
```

The auditor reads tracked working-tree files, every index stage, HEAD, and (with `--history`) every
root tree reachable from all Git refs. It also checks historical paths: renaming the same blob into
`scratch/` cannot evade the check. It does not change Git or print offending bytes. Exit 1 means a
finding or an audit error. Shallow clones are refused for `--history`; fetch the full history first.
Clean ignored/untracked private files are intentionally outside scope.

Normal mode requires **every** reference ROM configured in `config/title.json`, with its pinned
positive size and SHA-256. Missing/invalid inputs are errors, not an implicit weaker PASS; only
explicit `--no-rom` skips ROM comparison. This gate is defense in depth against **accidental**
author/tool leaks, not a detector for deliberate obfuscation.

**ROM-free structural layer:** payload/comment text may not contain a whitespace-free run of
**48 or more encoding-alphabet characters** (hex, Base32, Base64/URL-safe, Base85/Z85/Ascii85 or
uuencode). Intel HEX, Motorola S-record, uuencode M-lines and dense `=XX` quoted-printable lines
are forbidden by record shape. The only general opaque-token exception is a **standing 40- or
64-character hex hash**, not a hash prefix embedded in a larger encoding. The ROM decoder
remains active on hash-shaped tokens, so calling a ROM extract a hash does not bypass the
private check.

Source identifiers, compiled regular expressions, comment rulers, well-formed links/paths/citation
references and explicit lists of non-byte addresses are syntactically structured facts, not opaque
payloads. This classification is content/grammar based, never a per-path exemption. Literal
encoding data inside source assignments/comments is still checked.

Two reviewed, independently authored synthetic test literals predate this policy and remain in
immutable history. Their **exact literal SHA-256s**, not their paths, exempt opaque classification
only; all ROM/media checks remain active:

- `6fdad49cd15ee2f4f0ba4915a7a3439f3fb1332b1495a6c0d431569b8714692d`:
  38-byte synthetic switch fixture in `tests/support.py` / `tests/toolsmith_support.py`.
- `718061e0ac3631f4b6fa9c9872d390361a1c41947f2fd3b5f1410a0fe0044549`:
  24-byte synthetic argument/frame fixture in `tests/test_packet.py`.

Do not add further encoded test literals. Generate fixture bytes from instructions/word values
instead; changes to either approved literal do not inherit its exception.

ROM evidence is an exact binary-looking window of at least 16 raw bytes or 12 decoded bytes.
An ordinary printable 12/16-byte overlap is **not** evidence: prose and JSON explanations can
share words with the game. Printable material must quote at least **64 contiguous ROM bytes**
before it counts. The same rule applies to decoded JSON/hex/Base64, not just raw text.
The exact-window hash indexes are built once; ordinary text prefixes are extended against the
private images only when they match. Fixed public header exceptions remain; brittle per-source
indentation exemptions are no longer needed.

Below that structural layer, bounded decoders normalize text before exact window/signature checks.
Short Base32, Base85/Ascii85/Z85, uuencode, quoted-printable and addressed firmware records are
decoded as well; Intel HEX/S-record checksums and address continuity are validated. One byte-atom
parser handles signs, hexadecimal (`0x`/`$`), binary (`0b`), explicit/traditional octal and `od` rows,
decimal and C/Python/Rust integer suffix/underscore spellings,
and escaped character constants (`'x'`, `'\xNN'`, `'\NNN'`, `'\n'`, etc.). Only values in
**-128..255** map to bytes (`value & 0xFF`); larger integer values are not silently truncated.
Fixed canonical/decimal/hex-dump views retain ambiguous bare spellings, with a packed-hex view for
word dumps. Address-filtered views are additional candidates, not a reason to discard literals.
This replaces the old separate hex-only C initializer decoder.

The same pipeline accepts Python byte/C literals, JavaScript typed-array and Go slice wrappers,
numeric CSV/TSV columns, data URIs, JSON escaped bytes, array chunks and explicitly consecutive
numbered part fields. Unrelated object fields or files are not concatenated. Base64 runs normalize
whitespace, indentation, YAML scalars and Markdown fences without executing source or requiring YAML.
JSON is bounded to 64 levels / 400,000 nodes; encoded wrappers to four levels; total decoded
candidates to 16,000,000 bytes per blob. Bounds fail closed.

**Embedded-binary policy:** recognized image, audio/video, font, archive/compression and object
headers are findings independently of ROM availability. This includes PNG/GIF/JPEG/BMP/RIFF,
gzip/zlib/ZIP/7z/xz/bzip2, IEEE-695, ELF and the other standard signatures in the tool's catalog.
Gzip/images/archives are not expanded; zlib's ambiguous short header receives at most 4,097 bytes
of bounded validation output. Bare short magic constants are not classified as complete payloads.

**C/header text policy:** every string literal is decoded before any exemption. Only decoded,
printable live text under `src/` or `include/` is exempt from direct ROM-quotation checks.
Binary literals are still compared to both originals; more than eight escaped non-text bytes
in a literal/adjacent run are suspicious even in `--no-rom`. Encoded media and numeric dumps
inside otherwise printable literals are still checked. Comments never hide binary experiments.

**Numeric-block policy:** normalized signed/unsigned byte atoms are read in order across contiguous
data lines, including comments and disabled arrays. Labels/address columns have bounded alternate
views; scalar JSON/YAML metadata and prose are not dumps. A matching ROM window is a finding;
64 or more byte values are a structural finding even without a ROM, **including independently
authored test vectors/calibration tables**. Tests must generate such data programmatically instead
of committing literal dumps. There are no numeric path, content or recipe exceptions.
Blank/non-data lines delimit numeric blocks.

Independent checks reject private output directories, generated instruction markers, baserom
includes, genuine GNU objdump output, asm-differ headers/rows, Capstone listings at low or high
addresses, assets/binary/archive filenames, non-text blobs and non-Markdown `.md` files.
`config/**/*.json` has an explicit **4,000,000-byte** cap; **every other file** keeps the
**1,000,000-byte** cap. Large config JSON is still scanned, not exempt from content checks.
Opaque encoded prose and long padding strings are deliberately disallowed too, even when their
decoded contents are independently authored. The structural policy avoids a new decoder race for
every large encoding. Small split-file exports, unrecognized transforms and expression-built
streams are not exhaustively covered; the tool does not prove copyright safety.

**Add** this ROM-free CI step after the existing full-history checkout; copying the tools does not
install it. It needs only Python's standard library and Git, not a ROM, binutils or project packages:

```yaml
- uses: actions/checkout@v4
  with:
    fetch-depth: 0
- name: Audit public repository and reachable history (ROM-free)
  run: python tools/audit_public.py --history --no-rom
```

Keep a private pre-push audit as well; `--no-rom` is explicitly weaker. A local pre-push hook may
`exec .venv/bin/python tools/audit_public.py --history` from the repository root. No hook is installed
automatically.

Known gaps (independent review AuditReview6, 2026-10-22; being closed): encoded runs inside
JavaScript single-quoted or template strings and Go raw strings, and numeric data written as
tuples, call or return arguments, compound literals, list initialisation or INI values, can pass.
Until the next revision lands, check such files by hand before pushing.

## Private function packets

```sh
uv run python tools/packet.py func_027154
uv run python tools/packet.py func_000200 --version rev00 --out scratch/my-packets
uv run python tools/packet.py func_027154 --json
```

Use an actual unit address from that revision's catalogue. Default paths are
`scratch/packets/<version>/<unit>.md` and `.json`; `--json` also prints JSON to stdout, so redirect
only into private output locations. File output is refused unless both its lexical destination
(or symlink alias) and fully resolved physical destination are untracked and Git-ignored inside
the same repository. Public/tracked paths and symlink escapes are rejected before writing.
`--repo DIR` supports running an unintegrated proposal copy. The ROM and catalogue hashes are
checked before decoding with the existing `tools/m68k.py`.

Optional `config/<version>/names.json` uses schema version 1 and a `symbols` array of
`name`, `address`, `kind`, optional `size`, `source`, and `evidence`. Addresses may be integers or
`0x` strings. Absence is supported; generated catalogue names remain available as aliases.
Packets include symbolized instructions, table entries, direct `bsr`/`jsr` callers/callees,
cross-unit table targets, tail jumps, unresolved computed control flow, RAM/data references,
name provenance, and a qualified C signature/body sketch. Internal switch branches are recorded as
control-flow edges, not misrepresented as calls.

Argument hints correct displacement-based SP/A6 accesses for `link`, saved registers, stack
adjustments, and control-flow joins. They are **[INFERENCE]**, not proven parameter types, ABI,
return types, or behavior. Register parameters, stack aliases, and dynamic allocation are not
reconstructed. Do not adopt the C sketch as a candidate implementation.

## Relocation-aware function comparison

```sh
uv run python tools/diff.py func_027154 scratch/my-author/candidate.o --symbol my_function
```

The input must be a big-endian ELF32 m68k relocatable object with a defined function symbol. Without
`--symbol`, the catalogue unit name is used. The explicit symbol size is preferred; a zero-size
assembly symbol extends to the next typed function or the section end (not the next local label).

All non-relocated bytes and the total extent must match. Scalar absolute and PC-relative
`R_68K_{8,16,32,PC8,PC16,PC32}` fields are compared by canonical original target/addend, using known
names/aliases and the selected function's actual section placement. A named symbol's normalized
addend stays attached to that canonical base: a different function/data symbol plus a compensating
addend is **not** a match. Genuine aliases, local labels within one function, section relocations
and PC8's ELF-versus-CPU bias are normalized. REL/RELA and absolute-short RAM sign extension work.
Candidate definitions are not overridden by spelling: known `SHN_ABS` values and symbols in the
selected section must agree with original metadata. Defined symbols in other, unplaced sections
fail closed. Both relocation identity and encoded field bytes must match.
Unknown symbols, contradictory definitions, overlapping/unsupported relocations and out-of-range
PC-relative operands fail closed. Add provenance-backed names for genuinely known external targets
rather than inventing target addresses. Output is a Capstone-based
instruction side-by-side with relocation annotations. Exit 0 means this function compares exactly,
not that a complete ROM has passed acceptance.

## Optional visual differs

[asm-differ](https://github.com/simonlindholm/asm-differ) was evaluated at commit
[`0dd09af8f8008f1f880327cf0aca3b26d2562ea2`](https://github.com/simonlindholm/asm-differ/tree/0dd09af8f8008f1f880327cf0aca3b26d2562ea2).
Its m68k binary mode works with the pinned GNU binutils 2.47 `m68k-elf-objdump`. Optional root
`diff_settings.py` configures it:

```sh
# Install upstream in a separate private environment, not into the locked project environment.
# Run from the project root with diff_settings.py present.
TJE_DIFF_CANDIDATE=scratch/my-author/linked-image.bin asm-differ 0xSTART 0xEND --format plain --no-pager
```

Replace the addresses with catalogue start and exclusive end; the candidate must be a linked binary
at the original ROM layout. `TJE_DIFF_VERSION` selects a revision; `TJE_DIFF_ROOT` supports proposal
settings run outside the root. No automatic `make`/watch mode is configured before there is a C
build pipeline. Function-name lookup through the map parser was not qualified; numeric ranges
avoid that assumption.

Upstream object mode recognises absolute m68k relocations but its processor does not implement
`R_68K_PC*`. A real assembled `bsr.w` emitted an “unknown relocation type” diagnostic while
asm-differ still exited 0. Binary mismatch display also exits 0. It is a visual aid, never the
exact-match gate; use `tools/diff.py` for object comparisons.

[objdiff](https://github.com/encounter/objdiff), evaluated from source at
[`9aeec9cbab1a056ade7dc1f1625aacc41b0f8511` (3.8.3)](https://github.com/encounter/objdiff/tree/9aeec9cbab1a056ade7dc1f1625aacc41b0f8511),
has no m68k architecture backend: its dispatcher only selects PPC, MIPS, x86, ARM/ARM64 and SuperH,
and rejects other architectures. Supplying our objdump does not add a backend. No working objdiff
configuration is proposed. Its report format can still be used independently by the existing
progress reporter.

## Tests and ignored output

```sh
uv run python -m unittest discover -s tests
```

The author-tool suite uses synthetic ROMs and ELF objects; private-ROM, real-assembler/linker and
genuine baseline-objdump cross-checks skip when their inputs are absent. It needs only the pinned
Capstone and pyelftools dependencies. Existing `/scratch/`, `/build/`, `/orig/`, `/.cache/`, binary extensions,
and `__pycache__/` ignores cover generated outputs; no new ignore rule is required.
