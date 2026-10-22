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

**ROM-free structural layer:** every maximal encoding-alphabet run of **48 or more characters**
is inspected in raw text, regardless of file extension, source syntax or quoting style. A run is
opaque only when it has at least two digit/uppercase/lowercase/symbol classes and Shannon entropy
at or above **4.00 bits/character for hex/Base32**, **4.45 for Base64/URL-safe Base64**, or
**4.50 for Base85/Z85/Ascii85/uuencode**. Low-entropy identifiers, word-like text and rulers are
not opaque merely because they are long. Intel HEX, Motorola S-record, uuencode M-lines and dense
`=XX` quoted-printable lines remain forbidden by record shape.

Standing **40/64-hex hashes** and hash-shaped URL path components are allowed, including the
exact `2/2/60` hexadecimal package-digest shard format. Complete URLs, paths, provenance references
and identifier expressions are classified by whole fact shapes; a high-entropy component is
not hidden by that classification. This never exempts a file, source language or quote category.
There is no blanket `re.compile` or C-string structural exemption. ROM/media checks remain active.

Two exact historical regex-source spans in r6's `tools/audit_public.py` survive this calibration.
Their SHA-256s exempt opaque classification only; their current definitions are split into short
source fragments. Appending a byte or adding another opaque run does not inherit the exception:

- `910106863497592ff6eb3f0acfcff2e3251937fd99b4fa6cdaee5884c0249230`:
  old `DATA_URI` definition fragment (49 characters; entropy 4.645).
- `6286a75fe5594367894c754702055b644638a7b5b30516d30fffd7f553fe0b89`:
  old `MARKER` definition fragment (59 characters; entropy 4.836).

The older synthetic-byte exceptions are removed: those low-entropy fixtures need none under the
entropy policy. Continue generating test bytes programmatically instead of committing dumps.

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
Adjacent literals/array strings still supply private decoding candidates, but concatenation alone
does not establish an opaque run that never occurred literally in the input.
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

**Numeric-block policy:** every maximal numeric-token sequence is inspected directly in raw text,
including quoted values, tuples, return/call arguments, compound/direct-list initializers,
INI/TOML/YAML fields, comments and disabled code. Assignment/container syntax never decides whether
to scan. A matching ROM window is a finding; **64 or more byte-range values** is a structural
finding even without a ROM, including independent test vectors/calibration tables. Blank lines
do not interrupt a sequence. Identifier keys separate unrelated scalar metadata. Bounded alternate
views retain annotated rows and address columns; they never replace or suppress the raw pass.
There are no numeric path, content or recipe exceptions. Generate long test data programmatically.

Independent checks reject private output directories, generated instruction markers, baserom
includes, genuine GNU objdump output, asm-differ headers/rows, Capstone listings at low or high
addresses, assets/binary/archive filenames, non-text blobs and non-Markdown `.md` files.
`config/**/*.json` has an explicit **4,000,000-byte** cap; **every other file** keeps the
**1,000,000-byte** cap. Large config JSON is still scanned, not exempt from content checks.
High-entropy encoded prose may be rejected even when independently authored. Low-entropy padding
is not opaque, but size, decode-budget and private ROM checks still apply. Entropy screening is a
heuristic; short/low-entropy encodings can pass ROM-free CI. Small split-file exports, unrecognized
transforms and expression-built streams are not exhaustively covered. This is not a copyright proof.

The ROM-free CI step follows the full-history checkout. It needs only Python's standard library
and Git, not a ROM, binutils or project packages:

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

Known gaps (independent review AuditReview7, 2026-10-22): substrings that look like facts can
mask high-entropy Z85 runs; 40/64-hex Intel HEX record bodies pass as hashes; short Base85 inside
JavaScript/TypeScript, Go, Rust, Lua and Python f-string literals escapes the private decoders;
annotated 80-value arrays in Python, Lua and YAML pass. The gate is defence in depth against
accidents; check such files by hand before pushing.

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
