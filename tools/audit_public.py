"""Refuse private material in tracked worktree files, the index, HEAD, and optionally all history.

    python tools/audit_public.py [--history] [--no-rom] [--repo DIR]

No Git objects or files are changed. Reports never echo the offending bytes. --no-rom is suitable
for ROM-free CI; otherwise every configured reference ROM is required and verified before indexing.
"""
from __future__ import annotations

import argparse
import ast
import base64
import binascii
import csv
import hashlib
import io
import json
import math
import quopri
import string
import re
import os
import subprocess
import sys
import time
import tokenize
import zlib
from collections import Counter
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from urllib.parse import unquote_to_bytes

ROOT = Path(__file__).resolve().parents[1]
MAX_BYTES = 1_000_000
CONFIG_MAX_BYTES = 4_000_000
MAX_DECODED_BYTES = 4 * CONFIG_MAX_BYTES
MAX_JSON_NODES = 400_000
MIN_TEXT_MATCH = 64
MAX_JSON_DEPTH = 64
MAX_NON_TEXT_ESCAPES = 8
MAX_NUMERIC_RUN = 64
MAX_SIGNATURE_OUTPUT = 4096
MAX_ENCODING_DEPTH = 4
DATA_URI = re.compile(
    rb"data:([^,\s\"'<>]*),"
    rb"([^\s\"'<>]+)", re.I)
PRIVATE_DIRS = {"build", "asm", "orig", ".cache", "scratch"}
ASSET_SUFFIXES = {
    ".bin", ".gen", ".smd", ".sgd", ".zip", ".7z", ".gz", ".xz", ".rar", ".bz2", ".tar", ".zlib", ".lib",
    ".o", ".obj", ".a", ".so", ".dll", ".exe", ".elf", ".pyc", ".pdf", ".ieee", ".695",
    ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".ico", ".svg",
    ".wav", ".mp3", ".ogg", ".flac", ".aiff", ".aif", ".vgm", ".vgz", ".mid",
    ".mp4", ".webm", ".mov", ".ttf", ".woff", ".woff2",
}
# Fixed, reviewable exceptions, NOT derived from attacker-controlled config or arbitrary strings.
# Sixteen spaces are cartridge-header padding as well as ordinary source indentation.
PADDING = b" " * 16
HEADER_TEXT = (b"(C)SEGA 1991.OCT", b"(C)SEGA 1991.JUL", b"GM MK-1020 -02", b"GM MK-1020 -00")
ALLOWED_TEXT = {"tests/support.py": (b"SEGA GENESIS" + b" " * 4,)}
# Committed blobs whose findings an independent review judged false positives: exact git blob id
# and path -> the starts of the reasons it suppresses. Any other blob, path or reason is audited
# normally, so a changed file never inherits an exception.
REVIEWED_FALSE_POSITIVES = {
    # An unspaced Z80 length expression in the boot stub (C020 review); written with spaces since 2fc5393.
    ("76fd193d0b318c6266751e9eee359c569c3df8a4", "src/z80/initial_bootstrap.s"): ("opaque URL-safe Base64 run",),
    # Long C call/index expressions after the names pass (964aba3), reformatted in 4f8a21a.
    ("7419bb845cff0c33098eacb36bafdf4348a4209f", "src/graphics/camera_redraw_and_motion.c"): ("opaque Z85 run",),
    ("16d4cfe1f65a180b6cd9b5c44106fd500088fd28", "src/graphics/camera_row_transfer.c"): ("opaque Z85 run",),
    ("cb0b0808307fa40241da39d17333c2c10ef7b13d", "src/presents/level_item_setup.c"): ("opaque Z85 run",),
    ("e8f09c3d1cd07bc068e7357af119e5b0852a2b40", "src/sprites/entity_slots_tu14.c"): ("opaque Base85 run",),
    # The same expressions as test fixtures of a withdrawn audit rule (8244974).
    ("bfe33b94d9bb63ec3c7c41c7fd530c59cd4847aa", "tests/test_audit_public.py"): ("opaque Base85 run", "opaque Z85 run"),
    # The immutable c020 review record quotes those source lines as its evidence.
    ("f3acb1e2d208ed94285d6dc83d4240d5a5e05ae2", "accept/c020-names-hardcases-data/review-C020Review-review.json"):
        ("opaque Base85 run",),
}


def reviewed_reasons(oid: str, path: str, reasons: list[str]) -> list[str]:
    """Drop the reasons an exact reviewed (blob, path) exception names; keep everything else."""
    allowed = REVIEWED_FALSE_POSITIVES.get((oid, path), ())
    return [r for r in reasons if not r.startswith(allowed)] if allowed else reasons
MARKER = re.compile(
    rb"(?m)^\s*(?:[0-9a-fA-F]+\s+<)?"
    rb"\.Li_[0-9a-fA-F]{6,8}>?\s*:")
INCBIN = re.compile(rb"(?im)^\s*\.?incbin\s+[\"'][^\"'\r\n{}]*baserom(?:\.bin|[/\\])")
CONDITION = rb"(?:ra|sr|hi|ls|cc|cs|ne|eq|vc|vs|pl|mi|ge|lt|gt|le|f|t)"
ASM_WORD = (
    rb"(?:move(?:a|m|p|q)?|lea|pea|link|unlk|rts|rte|rtr|nop|reset|stop|trapv?|illegal|"
    rb"add[aiqx]?|sub[aiqx]?|cmp[aim]?|andi?|ori?|eori?|clr|tst|not|negx?|ext|swap|exg|"
    rb"asl|asr|lsl|lsr|rol|ror|roxl|roxr|div[su]|mul[su]|abcd|sbcd|nbcd|"
    rb"btst|bset|bclr|bchg|chk|tas|b" + CONDITION + rb"|db" + CONDITION + rb"|s" + CONDITION + rb")"
    rb"(?:\.[bwlst]|[bwls])?(?=[ \t\r\n]|$)"
)
LISTING = re.compile(
    rb"(?im)^[ \t]*(?:0x)?[0-9a-f]{1,16}:?[ \t]+"
    rb"(?:(?:[0-9a-f]{2}|[0-9a-f]{4})[ \t]+){1,10}" + ASM_WORD
)
ADDRESS_ASM = re.compile(
    rb"(?im)^[ \t]*(?:0x)?[0-9a-f]{1,16}(?::[ \t]+|\t+)" + ASM_WORD
)
ASM_DIFFER_HEADER = re.compile(rb"(?m)^[ \t]*TARGET[ \t]{2,}CURRENT(?:[ \t]+\([^)\r\n]*\))?[ \t]*$")
OBJDUMP_HEADER = re.compile(rb"(?im)^\s*(?:[0-9a-f]{1,16}\s+<[^>\r\n]+>:\s*$|Disassembly of section [^\r\n]+:\s*$)")
# Contiguous hex, byte/word arrays, and adjacent quoted literals. Odd-length tokens are excluded.
HEX_TOKEN = rb"(?<![\w])(?:0[xX]|\$)?(?:[0-9a-fA-F]{2})+(?![\w])"
HEX_RUN = re.compile(HEX_TOKEN + rb"(?:(?:[\s,]+|[\"']\s*[\"'])" + HEX_TOKEN + rb")*")
HEX_PART = re.compile(rb"(?:0[xX]|\$)?([0-9a-fA-F]+)")
HEX_ESCAPE = re.compile(rb"(?:\\x[0-9a-fA-F]{2}){6,}")
BASE64_TOKEN = re.compile(rb"(?<![A-Za-z0-9+/_=-])[A-Za-z0-9+/_-]{8,}={0,2}(?![A-Za-z0-9+/_=-])")
BASE64_ATOM = re.compile(rb"(?<![A-Za-z0-9+/_=-])(?:[A-Za-z0-9+/_-]+={0,2}|={1,2})(?![A-Za-z0-9+/_=-])")
MARKDOWN = re.compile(r"(?m)^\s*(?:#{1,6}\s|```|~~~|[-*+]\s|\d+\.\s|>\s|\|)|\[[^\n]+\]\([^\n]+\)")
CONTROL_BYTES = re.compile(rb"[\x00-\x08\x0b\x0e-\x1f\x7f]")
GZIP_MAGIC = b"\x1f\x8b\x08"
PY_BYTES_START = re.compile(rb"(?i)(?<![\w\"'])(?:br|rb|b)(?=[\"'])")
PY_BYTES_TOKEN = re.compile(r"(?i)^(?:br|rb|b)[\"']")
C_LEXEME = re.compile(rb"""//[^\r\n]*|/\*[\s\S]*?\*/|(?:u8|[uUL])?"(?:\\[\s\S]|[^"\\])*"|'(?:\\[\s\S]|[^'\\])*'""")
C_STRING = re.compile(rb'(?:u8|[uUL])?"(?:\\[\s\S]|[^"\\])*"')
C_GAP = re.compile(rb"(?:\s|/\*[\s\S]*?\*/|//[^\r\n]*(?:\n|$))*")
C_ESCAPE = re.compile(rb"\\(?:x[0-9a-fA-F]|[0-7])")
CHARACTER = rb"(?:u8|[uUL])?'(?:\\[\s\S]|[^'\\])*'"
INTEGER = rb"(?:0[xX]_?[0-9a-fA-F](?:_?[0-9a-fA-F])*|\$[0-9a-fA-F](?:_?[0-9a-fA-F])*|0[oO]_?[0-7](?:_?[0-7])*|0[bB]_?[01](?:_?[01])*|[0-9](?:_?[0-9])*|[0-9a-fA-F]{2,})(?:_?[uUiI](?:8|16|32|64|128|size)|[uUlL]*)"
NUMBER = re.compile(rb"(?<![\w.\\])[+-]?(?:" + CHARACTER + rb"|" + INTEGER + rb")(?![\w.])")
INTEGER_NUMBER = re.compile(rb"(?<![\w.\\])[+-]?(?:" + INTEGER + rb")(?![\w.])")
LINE_COMMENT = re.compile(rb"//[^\r\n]*|/\*[^\r\n]*?\*/")
BYTE_SEPARATORS = re.compile(rb"""^[\s,;:{}\[\]()=:+*/\\|<>?&!%~^`"'-]*$""")
INT_SUFFIX = re.compile(rb"(?:_?[uUiI](?:8|16|32|64|128|size)|[uUlL]+)$")
ROW_LABEL = re.compile(
    rb"(?m)^[ \t]*[A-Za-z_]\w*:"
    rb"[ \t]*(?=\r?$)")
ARRAY_DECLARATION = re.compile(
    rb"(?m)(?:^|(?<=[;{}]))[ \t]*"
    rb"(?:[A-Za-z_]\w*(?:[ \t]+|\*[ \t]*)){1,8}"
    rb"[A-Za-z_]\w*[ \t]*\[[^\]\r\n]*\]"
    rb"[ \t]*=[ \t]*\{")
MAX_OPAQUE_RUN = 48
OPAQUE_ALPHABETS = {
    "hex": string.hexdigits.encode(),
    "Base32": b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=",
    "Base64": (string.ascii_letters + string.digits + "+/=").encode(),
    "URL-safe Base64": (string.ascii_letters + string.digits + "-_=").encode(),
    "Base85": (string.digits + string.ascii_uppercase + string.ascii_lowercase + "!#$%&()*+-;<=>?@^_`{|}~").encode(),
    "Z85": (string.digits + string.ascii_lowercase + string.ascii_uppercase + ".-:+=^!/*?&<>()[]{}@%$#").encode(),
    "Ascii85": bytes(range(33, 118)),
    "uuencode": bytes(range(33, 97)),
}
OPAQUE_RUNS = {name: re.compile(b"[" + re.escape(alphabet) + b"]{" + str(MAX_OPAQUE_RUN).encode() + b",}")
               for name, alphabet in OPAQUE_ALPHABETS.items()}
OPAQUE_ENTROPY = {
    name: 4.0 if name in ("hex", "Base32") else 4.45 if name in ("Base64", "URL-safe Base64") else 4.5
    for name in OPAQUE_RUNS
}
HASH_WORD = re.compile(rb"(?<![A-Za-z0-9_+/=-])(?:[0-9a-fA-F]{40}|[0-9a-fA-F]{64})(?![A-Za-z0-9_+/=-])")
URI_REFERENCE = re.compile(rb"""(?:https?|ftp|ssh|git)://[^\s"'<>]+|(?<![\w.@-])[\w.-]+@[\w.-]+:[^\s"'<>]+""")
PATH_REFERENCE = re.compile(rb"(?<![\w.@-])(?:\.{0,2}/)?[\w.@-]+(?:/[\w.@-]+)+(?:[?#][\w.,:=%&+@/?#~-]*)?")
CITATION_REFERENCE = re.compile(rb"(?<![\w.@$+-])[\w.-]+:(?:[\w.@-]+/)*[\w.@-]+\.[A-Za-z0-9]{1,12}(?::[\w.@$+/-]+)*")
NAMESPACE_REFERENCE = re.compile(rb"(?<!\w)[a-z][a-z0-9_]{0,31}:[a-z][a-z0-9_]{0,31}(?:-[a-z][a-z0-9_]{0,31}){2,}")
HASH_COMPONENT = re.compile(rb"""(?<=/)(?:[0-9a-fA-F]{40}|[0-9a-fA-F]{64})(?=[/?.#"'`]|$)""")
PACKAGE_HASH = re.compile(rb"(?<=/packages/)[0-9a-fA-F]{2}/[0-9a-fA-F]{2}/[0-9a-fA-F]{60}(?=/)")
QUOTED_IDENTIFIER = re.compile(rb"""(["'])([A-Za-z_]\w*)\1""")
REFERENCE_EXPRESSION = re.compile(rb"[A-Za-z_]\w*(?:[.\[\](),;]+(?:[A-Za-z_]\w*|[0-9]+)?)+")
INTEL_LINE = re.compile(rb"^[ \t]*:[0-9A-Fa-f]{10,}[ \t]*$")
SRECORD_LINE = re.compile(rb"^[ \t]*S[0-9][0-9A-Fa-f]{4,}[ \t]*$")
UU_LINE = re.compile(rb"^M[ -`]{60}[ \t]*$")
QP_ESCAPE = re.compile(rb"=[0-9A-Fa-f]{2}")
# Only these two exact historical r6 regex-source spans survive entropy calibration.
# Current DATA_URI/MARKER definitions are split into short source fragments instead.
# This never exempts their files or any numeric/ROM/media check.
LEGACY_OPAQUE_SHA256 = frozenset({
    "910106863497592ff6eb3f0acfcff2e3251937fd99b4fa6cdaee5884c0249230",
    "6286a75fe5594367894c754702055b644638a7b5b30516d30fffd7f553fe0b89",
})


def ordered_string_chunks(mapping: dict):
    """Only an explicit, consecutive numbered-part schema defines a joined object stream."""
    groups = {}
    for key, value in mapping.items():
        match = re.fullmatch(r"([A-Za-z_][A-Za-z_.-]*)([0-9]+)", str(key))
        if match and isinstance(value, str):
            groups.setdefault(match[1], []).append((int(match[2]), value))
    for entries in groups.values():
        entries.sort()
        if len(entries) < 2 or [entry[0] for entry in entries] != list(range(len(entries))):
            continue
        values = [value.encode("utf-8", "surrogatepass") for _, value in entries]
        if all(re.fullmatch(rb"(?:[0-9a-fA-F]{40}|[0-9a-fA-F]{64})", value) for value in values):
            continue
        if all(re.fullmatch(rb"0[xX][0-9a-fA-F]{4,8}", value) and int(value, 16) > 255 for value in values):
            continue  # Numbered address fields are facts, not an encoded byte stream.
        yield b"".join(values)


def opaque_metrics(value: bytes) -> tuple[float, int]:
    """Shannon bits per character and the number of digit/upper/lower/symbol classes."""
    counts = Counter(value)
    if not counts:
        return 0.0, 0
    classes = {0 if 48 <= byte <= 57 else 1 if 65 <= byte <= 90 else 2 if 97 <= byte <= 122 else 3
               for byte in counts}
    size = len(value)
    entropy = math.log2(size) - sum(count * math.log2(count) for count in counts.values()) / size
    return entropy, len(classes)


def opaque_component(value: bytes) -> bool:
    if len(value) < MAX_OPAQUE_RUN:
        return False
    entropy, classes = opaque_metrics(value)
    return classes >= 2 and any(pattern.fullmatch(value) and entropy >= OPAQUE_ENTROPY[name]
                                for name, pattern in OPAQUE_RUNS.items())


def structured_text_view(text: bytes) -> bytes:
    """Remove complete reviewed fact shapes, never a language, file or string category."""
    spans = [match.span() for pattern in (HASH_WORD, HASH_COMPONENT, PACKAGE_HASH)
             for match in pattern.finditer(text)]
    for pattern in (URI_REFERENCE, CITATION_REFERENCE, PATH_REFERENCE, NAMESPACE_REFERENCE):
        for match in pattern.finditer(text):
            token = match.group()
            if pattern is PATH_REFERENCE and b"." not in token and b"-" not in token:
                continue
            components = HASH_COMPONENT.sub(b"", PACKAGE_HASH.sub(b"", token))
            if not any(opaque_component(part) for part in re.split(rb"[/?:=&]", components)):
                spans.append(match.span())
    # Short/low-entropy identifier atoms do not become secrets just because JSON/code framing
    # supplies extra punctuation. High-entropy literal identifiers remain candidates.
    for match in QUOTED_IDENTIFIER.finditer(text):
        if not opaque_component(match[2]):
            spans.append(match.span())
    for match in re.finditer(rb"\S+", text):
        word = match.group().strip(b"();,")
        if REFERENCE_EXPRESSION.fullmatch(word) and not any(
                opaque_component(atom) for atom in re.findall(rb"[A-Za-z_]\w*|[0-9]+", word)):
            spans.append(match.span())
    view = bytearray(text)
    for start, end in spans:
        view[start:end] = b" " * (end - start)
    return bytes(view)


def opaque_reason(path: str, data: bytes) -> str | None:
    # This raw pass is unconditional and path-independent: no source-language parser may
    # discard single quotes, templates, raw strings, comments or malformed source snippets.
    view = structured_text_view(data)
    for line in view.splitlines():
        if INTEL_LINE.fullmatch(line):
            return "Intel HEX record-line representation is not public source"
        if SRECORD_LINE.fullmatch(line):
            return "Motorola S-record representation is not public source"
        if UU_LINE.fullmatch(line):
            return "uuencode M-record representation is not public source"
        escapes = list(QP_ESCAPE.finditer(line))
        if len(escapes) >= 4 and 3 * len(escapes) >= len(line.strip()) // 4:
            return "dense quoted-printable record-line representation is not public source"
    metrics = {}
    for name, pattern in OPAQUE_RUNS.items():
        for match in pattern.finditer(view):
            key = match.span()
            if key not in metrics:
                metrics[key] = opaque_metrics(match.group())
            entropy, classes = metrics[key]
            if classes >= 2 and entropy >= OPAQUE_ENTROPY[name]:
                if hashlib.sha256(match.group()).hexdigest() in LEGACY_OPAQUE_SHA256:
                    continue
                return (f"opaque {name} run of {len(match.group())} characters; "
                        f"entropy {entropy:.3f} >= {OPAQUE_ENTROPY[name]:.2f} bits/char, "
                        f"{classes} character classes")
    return None


def printable_text(data: bytes) -> bool:
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return False
    return all(char.isprintable() or char in "\n\r\t\f" for char in text)


def looks_binary(data: bytes) -> bool:
    """Do not turn short ordinary English/Unicode overlaps into ROM evidence."""
    if CONTROL_BYTES.search(data):
        return True
    try:
        data.decode("utf-8")
        return False
    except UnicodeDecodeError:
        return any(0x80 <= byte <= 0x9F for byte in data) or sum(byte >= 0x80 for byte in data) * 3 >= len(data)


def blob_limit(path: str) -> int:
    p = PurePosixPath(path)
    return CONFIG_MAX_BYTES if p.parts[0] == "config" and p.suffix.lower() == ".json" else MAX_BYTES


def source_view(path: str, data: bytes, literals: list[CLiteral]) -> bytes:
    """Only live, fully decoded printable C text receives the source-text exemption."""
    p = PurePosixPath(path)
    if p.parts[0] not in ("src", "include") or p.suffix.lower() not in (".c", ".h"):
        return data
    spans = [span for group in literals if group.live and printable_text(group.raw) for span in group.spans]
    if not spans:
        return data
    masked = bytearray(data)
    for start, end in spans:
        masked[start:end] = b" " * (end - start)
    return bytes(masked)


class AuditError(RuntimeError):
    pass


@dataclass(frozen=True)
class Finding:
    source: str
    path: str
    reason: str


@dataclass
class CLiteral:
    spans: list[tuple[int, int]]
    raw: bytes
    escaped_nontext: int
    live: bool


def git(repo: Path, *args: str) -> bytes:
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True)
    if result.returncode:
        raise AuditError(result.stderr.decode("utf-8", "replace").strip())
    return result.stdout


class RomIndex:
    """Python's hash tables index exact windows (not collision-prone truncated digests)."""

    def __init__(self, images: list[bytes]):
        self.images = len(images)
        self.roms = images
        self.windows = {
            width: {rom[pos:pos + width] for rom in images for pos in range(len(rom) - width + 1)}
            for width in (12, 16)
        }

    def contains(self, data: bytes, width: int, allowed: tuple[bytes, ...] = (), *, evidence: bool = False) -> int | None:
        windows = self.windows[width]
        for pos in range(len(data) - width + 1):
            window = data[pos:pos + width]
            if window not in windows or (width == 16 and window == PADDING):
                continue
            # Header fields may touch space padding; require at least twelve field characters.
            unpadded = window.strip(b" ")
            if any(window in text or (width == 16 and len(unpadded) >= 12 and unpadded in text) for text in allowed):
                continue
            if evidence and not looks_binary(window):
                # A prose overlap of 12/16 bytes is not evidence. Require a long contiguous ROM
                # quotation instead; C literals are removed before this comparison.
                text = data[pos:pos + MIN_TEXT_MATCH]
                if len(text) < MIN_TEXT_MATCH or not any(text in rom for rom in self.roms):
                    continue
            return pos
        return None


class DecodeLimitError(AuditError):
    pass


class DecodeBudget:
    def __init__(self):
        self.used = 0
        self.structural_failure = False
        self.numeric_scanned = set()
        self.opaque_scanned = set()

    def charge(self, size: int) -> None:
        self.used += size
        if self.used > MAX_DECODED_BYTES:
            raise DecodeLimitError("encoded-content decoding exceeds the per-blob byte budget")


def decode_c_literal(token: bytes) -> tuple[bytes, int]:
    """Decode C byte-string escapes, not expressions or executable source."""
    body = token[token.index(b'"') + 1:-1]
    out, escaped_nontext = bytearray(), 0
    simple = {ord("a"): 7, ord("b"): 8, ord("f"): 12, ord("n"): 10, ord("r"): 13,
              ord("t"): 9, ord("v"): 11, ord("\\"): 92, ord("'"): 39, ord('"'): 34, ord("?"): 63}
    position = 0
    while position < len(body):
        value = body[position]
        position += 1
        if value != 92:
            out.append(value)
            continue
        if position == len(body):
            raise DecodeLimitError("unterminated C escape")
        value = body[position]
        position += 1
        if value in (10, 13):
            if value == 13 and body[position:position + 1] == b"\n":
                position += 1
            continue
        if value in simple:
            decoded = bytes([simple[value]])
        elif 48 <= value <= 55:
            digits = bytearray([value])
            while len(digits) < 3 and position < len(body) and 48 <= body[position] <= 55:
                digits.append(body[position])
                position += 1
            decoded = bytes([int(digits, 8) & 0xFF])
        elif value == ord("x"):
            start = position
            while position < len(body) and body[position] in b"0123456789abcdefABCDEF":
                position += 1
            if position == start:
                raise DecodeLimitError("invalid C hexadecimal escape")
            decoded = bytes([int(body[start:position], 16) & 0xFF])
        elif value in (ord("u"), ord("U")):
            length = 4 if value == ord("u") else 8
            try:
                decoded = chr(int(body[position:position + length], 16)).encode("utf-8")
            except (ValueError, UnicodeError) as error:
                raise DecodeLimitError("invalid C universal character escape") from error
            position += length
        else:
            decoded = bytes([value])  # Traditional C's implementation-defined unknown escape.
        out.extend(decoded)
        escaped_nontext += sum(byte < 32 and byte not in b"\n\r\t\f" or byte >= 127 for byte in decoded)
    return bytes(out), escaped_nontext


def c_literals(data: bytes, budget: DecodeBudget):
    """Join adjacent tokens across whitespace/comments; inspect commented-out literals too."""
    tokens = list(C_LEXEME.finditer(data))

    def groups(matches, region: bytes, base: int, live: bool):
        spans, parts, escaped = [], [], 0
        previous = 0
        for match in matches:
            token = match.group()
            if token.startswith((b"//", b"/*", b"'")):
                continue
            if parts and C_GAP.fullmatch(region[previous:match.start()]) is None:
                raw = b"".join(parts)
                budget.charge(len(raw))
                yield CLiteral(spans, raw, escaped, live)
                spans, parts, escaped = [], [], 0
            raw, count = decode_c_literal(token)
            spans.append((base + match.start(), base + match.end()))
            parts.append(raw)
            escaped += count
            previous = match.end()
        if parts:
            raw = b"".join(parts)
            budget.charge(len(raw))
            yield CLiteral(spans, raw, escaped, live)

    yield from groups(tokens, data, 0, True)
    for token in tokens:
        if token.group().startswith((b"//", b"/*")):
            end = -2 if token.group().startswith(b"/*") else None
            region = token.group()[2:end]
            yield from groups(C_STRING.finditer(region), region, token.start() + 2, False)


def json_payloads(text: str, budget: DecodeBudget):
    """Decode JSON strings/keys and adjacent array strings; never join unrelated object fields."""
    if not text.lstrip().startswith(("{", "[", '"')):
        return
    try:
        document = json.loads(text)
    except json.JSONDecodeError:
        return
    except RecursionError as error:
        raise DecodeLimitError("JSON exceeds the bounded decoding depth") from error

    def variants(value: str, literal: bool = True):
        raw = value.encode("utf-8", "surrogatepass")
        budget.charge(len(raw))
        yield raw, literal
        try:
            latin = value.encode("latin1")
        except UnicodeEncodeError:
            return
        if latin != raw:
            budget.charge(len(latin))
            yield latin, literal

    work = [(document, 0)]
    nodes = 0
    while work:
        value, depth = work.pop()
        nodes += 1
        if nodes > MAX_JSON_NODES or depth > MAX_JSON_DEPTH:
            raise DecodeLimitError("JSON exceeds the bounded decoding node/depth budget")
        if isinstance(value, str):
            yield from variants(value)
        elif isinstance(value, dict):
            for joined in ordered_string_chunks(value):
                budget.charge(len(joined))
                yield joined, False
            work.extend((item, depth + 1) for pair in value.items() for item in pair)
        elif isinstance(value, list):
            # Preserve adjacency, including runs separated by non-string elements.
            run = []
            for item in value + [None]:
                if isinstance(item, str):
                    run.append(item)
                else:
                    if len(run) > 1:
                        yield from variants("".join(run), literal=False)
                    run = []
            work.extend((item, depth + 1) for item in value)


def python_byte_payloads(data: bytes, budget: DecodeBudget):
    """Tokenize, never execute: repr(bytes), adjacent literals and simple literal '+' runs."""
    if not PY_BYTES_START.search(data):
        return
    parts = []

    def flush():
        if parts:
            raw = b"".join(parts)
            parts.clear()
            budget.charge(len(raw))
            return raw
        return None

    try:
        for token in tokenize.tokenize(io.BytesIO(data).readline):
            if token.type == tokenize.STRING and PY_BYTES_TOKEN.match(token.string):
                try:
                    value = ast.literal_eval(token.string)
                except (ValueError, SyntaxError):
                    value = None
                if isinstance(value, bytes):
                    parts.append(value)
                    continue
            elif token.type in (tokenize.ENCODING, tokenize.NL, tokenize.COMMENT, tokenize.INDENT, tokenize.DEDENT):
                continue
            elif token.type == tokenize.OP and token.string == "+" and parts:
                continue
            raw = flush()
            if raw is not None:
                yield raw
    except (tokenize.TokenError, IndentationError, SyntaxError, UnicodeError):
        pass  # Snippets/Markdown need not be complete Python; already tokenized literals count.
    raw = flush()
    if raw is not None:
        yield raw


def literal_values(token: bytes) -> tuple[tuple[int | None, ...], bytes]:
    """One number/escape decoder: canonical, decimal-dump, hex-dump, packed-hex views.

    Do not discard a minus sign or silently truncate a non-byte integer. All byte views are
    mapped only for -128..255; the packed view preserves explicit hexadecimal word dumps.
    """
    sign = -1 if token.startswith(b"-") else 1
    unsigned = token[1:] if token[:1] in (b"+", b"-") else token
    if b"'" in unsigned:
        body = unsigned[unsigned.index(b"'") + 1:-1]
        try:
            raw, _ = decode_c_literal(b'"' + body + b'"')
        except DecodeLimitError:
            return (None, None, None, None), b""  # A template is not a character literal.
        if len(raw) == 1:
            value = raw[0] * sign
        else:
            try:
                text = raw.decode("utf-8")
            except UnicodeDecodeError:
                return (None, None, None, None), b""
            if len(text) != 1:
                return (None, None, None, None), b""
            value = ord(text) * sign
        return (value, value, value, value), bytes([value & 255]) if -128 <= value <= 255 else b""
    spelling = INT_SUFFIX.sub(b"", unsigned).replace(b"_", b"")
    lower = spelling.lower()
    explicit_hex = lower.startswith(b"0x") or spelling.startswith(b"$")
    if explicit_hex:
        digits = spelling[2:] if lower.startswith(b"0x") else spelling[1:]
        value = int(digits, 16) * sign
    elif re.fullmatch(rb"0[oO][0-7]+|0[bB][01]+", spelling):
        value = int(spelling[2:], 8 if lower.startswith(b"0o") else 2) * sign
    elif any(byte in b"abcdefABCDEF" for byte in spelling):
        value = int(spelling, 16) * sign
    else:
        base = 8 if len(spelling) > 1 and spelling.startswith(b"0") and all(byte in b"01234567" for byte in spelling) else 10
        value = int(spelling, base) * sign
    decimal = int(spelling, 10) * sign if spelling.isdigit() else value
    hexadecimal = int(spelling, 16) if token[:1] not in (b"+", b"-") and not explicit_hex and len(spelling) <= 2 else value
    octal = int(spelling, 8) * sign if spelling.isdigit() and all(byte in b"01234567" for byte in spelling) else value
    packed = b""
    if explicit_hex and sign > 0 and len(digits) <= 8:
        packed = bytes.fromhex(digits.zfill(len(digits) + len(digits) % 2).decode())
    elif -128 <= value <= 255:
        packed = bytes([value & 255])
    return (value, decimal, hexadecimal, octal), packed


def numeric_variants(data: bytes, matches: list[re.Match]) -> tuple[bytes, ...]:
    """Decode one lexical run, retaining lossless and optional address-filtered views."""
    streams = [bytearray() for _ in range(10)]
    line_start = 0
    previous_end = 0
    for ordinal, match in enumerate(matches):
        newline = data.rfind(b"\n", previous_end, match.start())
        if newline >= 0:
            line_start = newline + 1
        first_on_line = ordinal == 0 or matches[ordinal - 1].end() <= line_start
        token = match.group()
        spelling = INT_SUFFIX.sub(b"", token.lstrip(b"+-")).replace(b"_", b"")
        digits = spelling[2:] if spelling.lower().startswith(b"0x") else spelling[1:] if spelling.startswith(b"$") else spelling
        offset = b"'" not in spelling and token[:1] not in (b"+", b"-") and not spelling.lower().startswith((b"0o", b"0b"))
        offset = offset and first_on_line and ordinal + 1 < len(matches) and len(digits) >= 4
        if offset:
            prefix = data[line_start:match.start()].strip()
            gap = data[match.end():matches[ordinal + 1].start()]
            offset = not prefix and bool(gap) and b"\n" not in gap and not gap.strip(b": \t\r")
        values, packed = literal_values(token)
        chunks = tuple(bytes([value & 255]) if value is not None and -128 <= value <= 255 else b"" for value in values) + (packed,)
        for stream, chunk in zip(streams[:5], chunks):
            stream.extend(chunk)
        if not offset:
            for stream, chunk in zip(streams[5:], chunks):
                stream.extend(chunk)
        previous_end = match.end()
    return tuple(bytes(stream) for stream in streams)


def numeric_fields(data: bytes) -> tuple[bytes, ...] | None:
    """Decode an all-numeric spelling; never infer an assignment or container grammar."""
    matches = list(NUMBER.finditer(data))
    if not matches or BYTE_SEPARATORS.fullmatch(NUMBER.sub(b" ", data)) is None:
        return None
    return numeric_variants(data, matches)


def numeric_blocks(data: bytes, budget: DecodeBudget):
    """Every maximal numeric-token run, in any lexical context or container syntax."""
    if CONTROL_BYTES.search(data):
        return
    try:
        data.decode("utf-8")
    except UnicodeDecodeError:
        return

    def annotation(match):
        token = match.group()
        body = token[2:-2] if token.startswith(b"/*") else token[2:]
        if numeric_fields(body) is not None:
            return token
        return bytes(10 if byte == 10 else 32 for byte in token)

    def views():
        yield data  # Always inspect the raw run before doing optional normalization.
        normalized = LINE_COMMENT.sub(annotation, data)
        for pattern in (ROW_LABEL, ARRAY_DECLARATION):
            normalized = pattern.sub(lambda match: bytes(10 if byte == 10 else 32 for byte in match.group()), normalized)
        if normalized != data:
            yield normalized

    seen = set()
    for view in views():
        # The integer lexer sees through every kind of quoted string. A second character-atom
        # view preserves C/Python escape arrays without letting a long quote swallow numbers.
        lexers = (INTEGER_NUMBER, NUMBER) if b"'" in view else (INTEGER_NUMBER,)
        for lexer in lexers:
            run = []
            first_line = 0
            current_line = 1
            previous_end = 0

            def flush():
                nonlocal run, first_line
                variants = numeric_variants(view, run)
                result = (first_line, variants)
                run = []
                if variants in seen:
                    return None
                seen.add(variants)
                for value in set(variants):
                    budget.charge(len(value))
                return result

            for match in lexer.finditer(view):
                gap = view[previous_end:match.start()]
                current_line += gap.count(b"\n")
                if run and BYTE_SEPARATORS.fullmatch(gap) is None:
                    result = flush()
                    if result is not None:
                        yield result
                if not run:
                    first_line = current_line
                run.append(match)
                current_line += match.group().count(b"\n")
                previous_end = match.end()
            if run:
                result = flush()
                if result is not None:
                    yield result


def numeric_reason(data: bytes, index: RomIndex | None, allowed: tuple[bytes, ...],
                   budget: DecodeBudget, origin: str, path: str) -> str | None:
    key = hashlib.sha256(data).digest()
    if key in budget.numeric_scanned:
        return None
    budget.numeric_scanned.add(key)
    for line, variants in numeric_blocks(data, budget):
        for raw in dict.fromkeys(variants):
            reason = payload_reason(raw, index, allowed, f"{origin}: numeric run at line {line}", budget)
            if reason is not None:
                return reason
        count = max(map(len, variants[:4]), default=0)
        if count >= MAX_NUMERIC_RUN:
            return f"{origin}: numeric run contains {count} byte values (limit {MAX_NUMERIC_RUN - 1}; no exemptions)"
    return None


def base64_candidates(data: bytes) -> set[bytes]:
    """Normalize lexical Base64 runs separated only by whitespace, not by container syntax."""
    tokens = {match.group() for match in BASE64_TOKEN.finditer(data)}
    # Formatting headers delimit a run; their language names/chomping sigils are not payload.
    markup = re.sub(rb"""(?m)^[ \t]*(?:(?:[\w.-]+|"[^"\r\n]*"|'[^'\r\n]*'):[ \t]*)?(?:![\w!:/.-]+[ \t]+)?[|>](?:[1-9][+-]?|[+-][1-9]?)?[ \t]*(?:#[^\r\n]*)?$""", b"@", data)
    markup = re.sub(rb"(?m)^[ \t]*(?:>[ \t]*)+", b"", markup)
    markup = re.sub(rb"(?m)^[ \t]*(?:`{3,}|~{3,})[^\r\n]*", b"@", markup)
    markup = re.sub(rb"""(?m)^[ \t]*["']|["'][ \t]*$""", b"", markup)
    for view in dict.fromkeys((data, markup)):
        chunks = []
        previous = None

        def flush() -> None:
            if len(chunks) > 1:
                value = b"".join(chunks)
                if len(value) >= 8:
                    tokens.add(value)
            chunks.clear()

        for atom in BASE64_ATOM.finditer(view):
            if previous is not None and not view[previous:atom.start()].isspace():
                flush()
            chunks.append(atom.group())
            previous = atom.end()
            if b"=" in atom.group():
                flush()
                previous = None
        flush()
    return tokens


def embedded_kind(raw: bytes, budget: DecodeBudget) -> str | None:
    """Recognize binary formats, not fixture paths. Bare short magic constants are not files."""
    # A complete zlib stream can be smaller than twelve bytes (unlike a bare two-byte magic).
    if len(raw) >= 6 and raw[0] & 15 == 8 and raw[0] >> 4 <= 7 and int.from_bytes(raw[:2], "big") % 31 == 0:
        try:
            stream = zlib.decompressobj()
            output = stream.decompress(raw, MAX_SIGNATURE_OUTPUT + 1)
            budget.charge(len(output))
            if stream.eof or len(output) > MAX_SIGNATURE_OUTPUT:
                return "zlib compressed data"
        except zlib.error:
            pass
    if len(raw) < 12:
        return None
    signatures = (
        ("PNG image", b"\x89PNG\r\n\x1a\n"), ("GIF image", b"GIF87a"), ("GIF image", b"GIF89a"),
        ("JPEG image", b"\xff\xd8\xff"), ("TIFF image", b"II*\0"), ("TIFF image", b"MM\0*"),
        ("Photoshop image", b"8BPS"), ("ZIP archive", b"PK\x03\x04"), ("ZIP archive", b"PK\x05\x06"),
        ("ZIP archive", b"PK\x07\x08"), ("7z archive", b"7z\xbc\xaf'\x1c"),
        ("xz compressed data", b"\xfd7zXZ\0"), ("bzip2 compressed data", b"BZh"),
        ("RAR archive", b"Rar!\x1a\x07"), ("Unix archive", b"!<arch>\n"),
        ("zstd compressed data", b"\x28\xb5\x2f\xfd"), ("lz4 compressed data", b"\x04\x22\x4d\x18"),
        ("CAB archive", b"MSCF"), ("RPM package", b"\xed\xab\xee\xdb"),
        ("ELF object", b"\x7fELF"), ("PDF document", b"%PDF-"), ("Ogg audio", b"OggS"),
        ("FLAC audio", b"fLaC"), ("MP3 audio", b"ID3"), ("MIDI audio", b"MThd"),
        ("TrueType font", b"\0\1\0\0"), ("OpenType font", b"OTTO"), ("WOFF font", b"wOFF"),
        ("WOFF2 font", b"wOF2"), ("CPIO archive", b"070701"), ("CPIO archive", b"070702"), ("CPIO archive", b"070707"),
    )
    for name, signature in signatures:
        if raw.startswith(signature):
            return name
    if raw.startswith(GZIP_MAGIC) and not raw[3] & 0xE0:
        return "gzip-encoded archive"
    if raw.startswith(b"RIFF"):
        return "RIFF media"
    if raw.startswith(b"FORM") and raw[8:12] in (b"AIFF", b"AIFC"):
        return "AIFF audio"
    if raw.startswith(b"BM") and len(raw) >= 14 and raw[6:10] == bytes(4):
        size, offset = int.from_bytes(raw[2:6], "little"), int.from_bytes(raw[10:14], "little")
        if 14 <= offset <= size:
            return "BMP image"
    if raw.startswith((bytes([0, 0, 1, 0]), bytes([0, 0, 2, 0]))) and len(raw) >= 22:
        return "icon/cursor image"
    if raw[4:8] == b"ftyp":
        return "ISO media"
    if raw.startswith(b"SQLite format 3" + bytes(1)):
        return "SQLite database"
    if raw.lstrip().startswith(b"<svg") or raw.lstrip().startswith(b"<?xml") and b"<svg" in raw[:512]:
        return "SVG image"
    if len(raw) >= 262 and raw[257:262] == b"ustar":
        return "tar archive"
    if len(raw) >= 272 and raw[256:260] == b"SEGA" and (raw[272:275] == b"(C)" or not raw[:256].isascii()
                                                       or CONTROL_BYTES.search(raw[:256])):
        # A cartridge header: the system name at 0x100 with the copyright field at 0x110 or a binary
        # vector table before it; "SEGA" merely landing at offset 256 of a text file is not one.
        return "Genesis ROM"
    if raw[0] == 0xE0:
        # IEEE-695 MB: two length-prefixed ASCII identifiers, then the AD record.
        position = 1
        try:
            for ordinal in range(2):
                length = raw[position]
                position += 1
                if length in (0xDE, 0xDF):
                    width = 1 if length == 0xDE else 2
                    length = int.from_bytes(raw[position:position + width], "big")
                    position += width
                elif length > 0x7F:
                    break
                identifier = raw[position:position + length]
                if len(identifier) != length or ordinal == 0 and not identifier or not all(32 <= byte < 127 for byte in identifier):
                    break
                position += length
            else:
                if raw[position:position + 1] == bytes([0xEC]):
                    return "IEEE-695 object/library"
        except IndexError:
            pass
    return None


def payload_reason(raw: bytes, index: RomIndex | None, allowed: tuple[bytes, ...], origin: str,
                   budget: DecodeBudget) -> str | None:
    kind = embedded_kind(raw, budget)
    if kind is not None:
        return f"{origin}: embedded {kind} payload"
    if index is not None and index.contains(raw, 12, allowed, evidence=True) is not None:
        return f"{origin}: ROM window in decoded bytes (binary >=12 bytes or text >={MIN_TEXT_MATCH})"
    return None


def inspect_decoded(raw: bytes, index: RomIndex | None, allowed: tuple[bytes, ...],
                    budget: DecodeBudget, origin: str, path: str, depth: int) -> str | None:
    reason = payload_reason(raw, index, allowed, origin, budget)
    if reason is not None:
        return reason
    if len(raw) >= 12 and printable_text(raw):
        if depth >= MAX_ENCODING_DEPTH:
            raise DecodeLimitError("encoded-content nesting exceeds the bounded decoding depth")
        return encoded_reason(raw, index, allowed, budget, origin, path=path, depth=depth + 1)
    return None


def table_payloads(data: bytes, budget: DecodeBudget):
    """Project rectangular numeric columns; a schema-labelled index/address is annotation."""
    try:
        lines = data.decode("utf-8").splitlines()
    except UnicodeDecodeError:
        return
    rows, header, width = [], [], 0

    def flush():
        nonlocal rows, width
        if not rows:
            return []
        emitted = []
        for column in range(width):
            if column < len(header) and header[column] in ("index", "address", "offset", "row", "line"):
                continue
            for mode in range(4):
                values = [row[column][mode] for row in rows]
                raw = bytes(value & 255 for value in values if value is not None and -128 <= value <= 255)
                if raw:
                    emitted.append(raw)
        rows, width = [], 0
        for raw in dict.fromkeys(emitted):
            budget.charge(len(raw))
        return list(dict.fromkeys(emitted))

    for line in lines:
        delimiter = "\t" if "\t" in line else "," if "," in line else None
        if delimiter is None:
            yield from flush()
            continue
        try:
            fields = next(csv.reader([line], delimiter=delimiter))
        except csv.Error:
            yield from flush()
            continue
        fields = [field.strip() for field in fields]
        if fields and not fields[-1]:
            fields.pop()
        if not 2 <= len(fields) <= 32:
            yield from flush()
            continue
        if all(re.fullmatch(r"[A-Za-z_][\w -]*", field) for field in fields):
            yield from flush()
            header = [field.lower() for field in fields]
            continue
        if not all(NUMBER.fullmatch(field.encode()) for field in fields):
            yield from flush()
            continue
        if width and width != len(fields):
            yield from flush()
        width = len(fields)
        rows.append([literal_values(field.encode())[0] for field in fields])
    yield from flush()


def conventional_payloads(data: bytes, budget: DecodeBudget):
    """Bounded standard-library encodings plus addressed firmware records, not format guesses."""
    for match in re.finditer(rb"(?<![A-Za-z2-7=])[A-Za-z2-7]{20,}={0,6}(?![A-Za-z2-7=])", data):
        token = match.group()
        try:
            raw = base64.b32decode(token + b"=" * (-len(token) % 8), casefold=True)
        except (ValueError, binascii.Error):
            continue
        budget.charge(len(raw))
        yield "Base32", raw
    candidates = {match.group() for match in re.finditer(rb"\S{15,}", data)}
    candidates.update(match.group() for match in re.finditer(rb"<~[\s!-uz]+?~>", data))
    z85 = {byte: value for value, byte in enumerate(OPAQUE_ALPHABETS["Z85"])}
    for token in candidates:
        for name, decoder in (("Base85", base64.b85decode),
                              ("Ascii85", lambda value: base64.a85decode(value, adobe=value.startswith(b"<~")))):
            try:
                raw = decoder(token)
            except (ValueError, binascii.Error):
                continue
            budget.charge(len(raw))
            yield name, raw
        if len(token) % 5 == 0 and all(byte in z85 for byte in token):
            output = bytearray()
            for start in range(0, len(token), 5):
                value = 0
                for byte in token[start:start + 5]:
                    value = value * 85 + z85[byte]
                if value > 0xFFFFFFFF:
                    break
                output.extend(value.to_bytes(4, "big"))
            else:
                raw = bytes(output)
                budget.charge(len(raw))
                yield "Z85", raw
    if QP_ESCAPE.search(data) or b"=\n" in data or b"=\r\n" in data:
        raw = quopri.decodestring(data)
        budget.charge(len(raw))
        yield "quoted-printable", raw

    records, base, uu_parts = [], 0, []

    def finish_uu():
        if not uu_parts:
            return None
        raw = b"".join(uu_parts)
        uu_parts.clear()
        budget.charge(len(raw))
        return raw

    for line in data.splitlines():
        stripped = line.strip()
        if stripped.startswith(b":") and re.fullmatch(rb":[0-9a-fA-F]+", stripped):
            try:
                record = bytes.fromhex(stripped[1:].decode())
            except ValueError:
                continue
            if len(record) >= 5 and len(record) == record[0] + 5 and sum(record) & 255 == 0:
                kind, payload = record[3], record[4:-1]
                if kind == 0:
                    records.append((base + int.from_bytes(record[1:3], "big"), payload))
                elif kind in (2, 4) and len(payload) == 2:
                    base = int.from_bytes(payload, "big") << (4 if kind == 2 else 16)
        elif re.fullmatch(rb"S[123][0-9a-fA-F]+", stripped):
            try:
                record = bytes.fromhex(stripped[2:].decode())
            except ValueError:
                continue
            address_size = int(chr(stripped[1])) + 1
            if len(record) >= address_size + 2 and record[0] + 1 == len(record) and sum(record) & 255 == 255:
                records.append((int.from_bytes(record[1:1 + address_size], "big"), record[1 + address_size:-1]))
        uu_line = line.rstrip(b"\r")
        count = (uu_line[0] - 32) & 63 if uu_line else 0
        if 1 <= count <= 63 and len(uu_line) == 1 + ((count + 2) // 3) * 4 and all(32 <= byte <= 96 for byte in uu_line):
            try:
                raw = binascii.a2b_uu(uu_line)
            except binascii.Error:
                raw = None
            if raw is not None and len(raw) == count:
                uu_parts.append(raw)
                continue
        raw = finish_uu()
        if raw is not None:
            yield "uuencode", raw
    raw = finish_uu()
    if raw is not None:
        yield "uuencode", raw
    current, end = bytearray(), None
    for address, payload in sorted(records):
        if end is not None and address != end:
            raw = bytes(current)
            budget.charge(len(raw))
            yield "addressed firmware records", raw
            current.clear()
        current.extend(payload)
        end = address + len(payload)
    if current:
        raw = bytes(current)
        budget.charge(len(raw))
        yield "addressed firmware records", raw


def encoded_reason(data: bytes, index: RomIndex | None, allowed: tuple[bytes, ...],
                   budget: DecodeBudget, origin: str, decoded: bool = False, path: str = "", depth: int = 0,
                   check_opaque: bool = True) -> str | None:
    key = hashlib.sha256(data).digest()
    if check_opaque and key not in budget.opaque_scanned:
        budget.opaque_scanned.add(key)
        reason = opaque_reason(path, data)
        if reason is not None:
            budget.structural_failure = True
            return reason
    if decoded:
        reason = payload_reason(data, index, allowed, origin, budget)
        if reason is not None:
            return reason
    reason = numeric_reason(data, index, allowed, budget, origin, path)
    if reason is not None:
        return reason
    if not budget.structural_failure:
        for raw in table_payloads(data, budget):
            reason = inspect_decoded(raw, index, allowed, budget, origin + ": numeric table column", path, depth)
            if reason is not None:
                return reason
            if len(raw) >= MAX_NUMERIC_RUN:
                return f"{origin}: numeric table column contains {len(raw)} byte values (limit {MAX_NUMERIC_RUN - 1})"
        for kind, raw in conventional_payloads(data, budget):
            reason = inspect_decoded(raw, index, allowed, budget, origin + ": " + kind, path, depth)
            if reason is not None:
                return reason
    for uri in DATA_URI.finditer(data):
        raw = unquote_to_bytes(uri[2])
        if b";base64" in uri[1].lower():
            try:
                raw = base64.b64decode(raw + b"=" * (-len(raw) % 4), altchars=b"-_", validate=True)
            except (ValueError, binascii.Error):
                continue
        budget.charge(len(raw))
        reason = inspect_decoded(raw, index, allowed, budget, origin + ": data URI", path, depth)
        if reason is not None:
            return reason
    for match in HEX_RUN.finditer(data):
        parts = [m.group(1) for m in HEX_PART.finditer(match.group())]
        if sum(map(len, parts)) < 12:
            continue
        raw = bytes.fromhex(b"".join(parts).decode("ascii"))
        budget.charge(len(raw))
        reason = inspect_decoded(raw, index, allowed, budget, origin + ": hex-encoded", path, depth)
        if reason is not None:
            return reason
    for match in HEX_ESCAPE.finditer(data):
        raw = bytes.fromhex(match.group().replace(b"\\x", b"").decode("ascii"))
        budget.charge(len(raw))
        reason = inspect_decoded(raw, index, allowed, budget, origin + ": hex-escaped", path, depth)
        if reason is not None:
            return reason
    for raw in python_byte_payloads(data, budget):
        reason = inspect_decoded(raw, index, allowed, budget, origin + ": Python bytes literal", path, depth)
        if reason is not None:
            return reason
    if C_ESCAPE.search(data):
        for group in c_literals(data, budget):
            reason = inspect_decoded(group.raw, index, allowed, budget, origin + ": C string literal", path, depth)
            if reason is not None:
                return reason
    for token in base64_candidates(data):
        if len(token) % 4 == 1:
            continue
        try:
            raw = base64.b64decode(token + b"=" * (-len(token) % 4), altchars=b"-_", validate=True)
        except (binascii.Error, ValueError):
            continue
        budget.charge(len(raw))
        reason = inspect_decoded(raw, index, allowed, budget, origin + ": Base64/URL-safe Base64", path, depth)
        if reason is not None:
            return reason
    return None


def content_reasons(path: str, data: bytes, index: RomIndex | None) -> list[str]:
    reasons = []
    try:
        text = data.decode("utf-8")
        is_text = CONTROL_BYTES.search(data) is None
    except UnicodeDecodeError:
        text, is_text = "", False
    if not is_text:
        reasons.append("non-text blob (not UTF-8 source/Markdown)")
    if PurePosixPath(path).suffix.lower() == ".md" and (not is_text or not MARKDOWN.search(text)):
        reasons.append(".md file is not recognisable Markdown (possible ROM)")
    allowed = HEADER_TEXT + ALLOWED_TEXT.get(path, ())
    budget = DecodeBudget()
    structural = opaque_reason(path, data)
    budget.opaque_scanned.add(hashlib.sha256(data).digest())
    if structural is not None:
        reasons.append(structural)
        budget.structural_failure = True
    try:
        numeric = numeric_reason(data, index, allowed, budget, "blob", path)
        if numeric is not None:
            reasons.append(numeric)
            budget.structural_failure = True
        raw_kind = embedded_kind(data, budget)
        if raw_kind is not None:
            reasons.append(f"embedded {raw_kind} payload")
        is_c = PurePosixPath(path).suffix.lower() in (".c", ".h")
        literals = list(c_literals(data, budget)) if is_c else []
        for group in literals:
            is_text_literal = printable_text(group.raw)
            exempt = group.live and is_text_literal and PurePosixPath(path).parts[0] in ("src", "include")
            if not is_text_literal and group.escaped_nontext > MAX_NON_TEXT_ESCAPES:
                reasons.append(f"suspicious C literal: {group.escaped_nontext} non-text escaped bytes (limit {MAX_NON_TEXT_ESCAPES})")
            reason = payload_reason(group.raw, None if exempt else index, allowed, "C string literal", budget)
            if reason is None:
                reason = encoded_reason(group.raw, index, allowed, budget, "C string literal",
                                        path=path, check_opaque=len(group.spans) == 1)
            if reason is not None and reason not in reasons:
                reasons.append(reason)
        view = source_view(path, data, literals)
        if MARKER.search(view):
            reasons.append("generated assembly instruction marker")
        if INCBIN.search(view):
            reasons.append("assembly includes a private baserom")
        if LISTING.search(view) or ADDRESS_ASM.search(view) or OBJDUMP_HEADER.search(view) or ASM_DIFFER_HEADER.search(view):
            reasons.append("address/instruction disassembly listing")
        if index is not None:
            offset = index.contains(view, 16, allowed, evidence=True)
            if offset is not None:
                reasons.append(f"raw ROM window (binary >=16 bytes or text >={MIN_TEXT_MATCH}) at blob offset 0x{offset:x}")
        reason = encoded_reason(view, index, allowed, budget, "blob", path=path)
        if reason is None and is_text:
            for payload, literal in json_payloads(text if view is data else view.decode("utf-8"), budget):
                reason = encoded_reason(payload, index, allowed, budget, "JSON string/adjacent array",
                                        decoded=True, path=path, check_opaque=literal)
                if reason is not None:
                    break
        if reason is not None:
            reasons.append(reason)
    except DecodeLimitError as error:
        reasons.append(str(error))
    return reasons


def reviewed_assembly_source(p: PurePosixPath) -> bool:
    """Ruling R007: hand-written 68000 sources are committed as src/asm/<name>.s (exactly that
    directory, no deeper); every content rule still applies to them."""
    return p.parts[:2] == ("src", "asm") and len(p.parts) == 3 and p.suffix == ".s"


def path_reasons(path: str, size: int, mode: str) -> list[str]:
    p = PurePosixPath(path)
    reasons = []
    if any(part.lower() in PRIVATE_DIRS for part in p.parts[:-1]) and not reviewed_assembly_source(p):
        reasons.append("file under a private/generated directory")
    if p.suffix.lower() in ASSET_SUFFIXES or p.name.lower().startswith("baserom"):
        reasons.append("ROM/binary/archive/asset filename")
    limit = blob_limit(path)
    if size > limit:
        reasons.append(f"blob exceeds {limit // 1_000_000} MB ({size} bytes)")
    if mode == "160000":
        reasons.append("gitlink cannot be audited as a public text blob")
    return reasons


class BlobReader:
    """One persistent cat-file process; do not materialise giant historical blobs."""

    def __init__(self, repo: Path):
        self.proc = subprocess.Popen(["git", "-C", str(repo), "cat-file", "--batch"],
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def read(self, oid: str) -> bytes:
        self.proc.stdin.write(oid.encode() + b"\n")
        self.proc.stdin.flush()
        header = self.proc.stdout.readline().split()
        if len(header) != 3 or header[1] != b"blob":
            raise AuditError(f"cannot read Git blob {oid}")
        size = int(header[2])
        data = self.proc.stdout.read(size)
        if len(data) != size or self.proc.stdout.read(1) != b"\n":
            raise AuditError(f"truncated Git blob {oid}")
        return data

    def close(self) -> None:
        self.proc.stdin.close()
        self.proc.stdout.close()
        if self.proc.wait() != 0:
            raise AuditError("git cat-file failed")


def reference_images(repo: Path) -> list[bytes]:
    """Normal mode is a private gate, not an implicit fallback to ROM-free CI."""
    try:
        title = json.loads((repo / "config/title.json").read_text())
        versions = title["versions"]
        if not isinstance(versions, dict) or not versions:
            raise ValueError("no configured ROM versions")
        images = []
        for version, spec in versions.items():
            size, digest = spec["size"], spec["sha256"]
            if not isinstance(size, int) or size <= 0 or not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
                raise ValueError(f"{version}: missing/invalid pinned size or SHA-256")
            path = repo / spec["orig"]
            if not path.is_file():
                raise AuditError(f"{version}: required reference ROM {path} is missing; only --no-rom skips comparison")
            if path.stat().st_size != size:
                raise AuditError(f"{version}: reference ROM size differs from config/title.json")
            image = path.read_bytes()
            if hashlib.sha256(image).hexdigest() != digest.lower():
                raise AuditError(f"{version}: reference ROM SHA-256 differs from config/title.json")
            images.append(image)
        return images
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise AuditError(f"cannot load pinned reference ROMs: {error}; use --no-rom only for ROM-free CI") from error


def audit(repo: Path, history: bool = False, no_rom: bool = False) -> tuple[list[Finding], dict]:
    started = time.perf_counter()
    repo = Path(os.fsdecode(git(repo, "rev-parse", "--show-toplevel").strip()))
    object_format = git(repo, "rev-parse", "--show-object-format").decode().strip()
    images = [] if no_rom else reference_images(repo)
    index = None if no_rom else RomIndex(images)
    refs: set[tuple[str, str, str, str]] = set()
    work_paths = set()
    for entry in git(repo, "ls-files", "--stage", "-z").split(b"\0"):
        if not entry:
            continue
        meta, path = entry.split(b"\t", 1)
        mode, oid, stage = meta.decode().split()
        name = os.fsdecode(path)
        work_paths.add(name)
        if set(oid) != {"0"}:
            refs.add((oid, name, f"index:{stage}", mode))
    if history and git(repo, "rev-parse", "--is-shallow-repository").strip() == b"true":
        raise AuditError("--history requires full history; fetch the shallow clone before auditing")
    # --all includes branches, tags and other refs. Deduplicate root trees, but never paths: the
    # same harmless blob at README.md and scratch/leak.txt must still be rejected at the latter.
    trees = git(repo, "log", "--all", "--format=%T").splitlines() if history else []
    head = subprocess.run(["git", "-C", str(repo), "rev-parse", "--verify", "HEAD^{tree}"], capture_output=True)
    if head.returncode == 0:
        trees.append(head.stdout.strip())
    trees = sorted(set(trees))
    for tree in trees:
        for entry in git(repo, "ls-tree", "-r", "-z", tree.decode()).split(b"\0"):
            if entry:
                meta, path = entry.split(b"\t", 1)
                mode, kind, oid = meta.decode().split()
                refs.add((oid, os.fsdecode(path), f"tree:{tree.decode()[:12]}", mode))
    oids = sorted({oid for oid, _, _, mode in refs if mode != "160000"})
    sizes = {}
    if oids:
        result = subprocess.run(["git", "-C", str(repo), "cat-file", "--batch-check=%(objectname) %(objecttype) %(objectsize)"],
                                input=("\n".join(oids) + "\n").encode(), capture_output=True, check=True)
        for row in result.stdout.splitlines():
            oid, kind, size = row.decode().split()
            if kind != "blob":
                raise AuditError(f"expected a blob, got {kind}: {oid}")
            sizes[oid] = int(size)
    findings = []
    cache: dict[tuple[str, str], list[str]] = {}
    reader = BlobReader(repo)
    try:
        for oid, path, source, mode in sorted(refs):
            size = sizes.get(oid, 0)
            reasons = path_reasons(path, size, mode)
            key = (oid, path)  # C-literal/Markdown/header policy is path-specific.
            if mode != "160000" and size <= blob_limit(path):
                if key not in cache:
                    cache[key] = content_reasons(path, reader.read(oid), index)
                reasons += reviewed_reasons(oid, path, cache[key])
            findings.extend(Finding(source, path, reason) for reason in reasons)
    finally:
        reader.close()
    work_count = 0
    for path in sorted(work_paths):
        absolute = repo / path
        if absolute.is_symlink():
            data = os.fsencode(os.readlink(absolute))
        elif absolute.is_file():
            if absolute.stat().st_size > blob_limit(path):
                findings.extend(Finding("worktree", path, reason) for reason in path_reasons(path, absolute.stat().st_size, ""))
                work_count += 1
                continue
            data = absolute.read_bytes()
        else:
            continue
        work_count += 1
        hasher = hashlib.new(object_format)
        hasher.update(f"blob {len(data)}\0".encode())
        hasher.update(data)
        key = (hasher.hexdigest(), path)
        if key not in cache:
            cache[key] = content_reasons(path, data, index)
        reasons = path_reasons(path, len(data), "") + reviewed_reasons(*key, cache[key])
        findings.extend(Finding("worktree", path, reason) for reason in reasons)
    summary = {"blobs": len(oids), "trees": len(trees), "tracked_worktree_files": work_count,
               "roms": len(images), "rom_check": index is not None, "history": history,
               "seconds": round(time.perf_counter() - started, 3)}
    return findings, summary


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", type=Path, default=ROOT)
    parser.add_argument("--history", action="store_true", help="include every tree reachable from all Git refs")
    parser.add_argument("--no-rom", action="store_true", help="CI: skip only the private ROM-byte comparison")
    args = parser.parse_args(argv)
    try:
        findings, summary = audit(args.repo, args.history, args.no_rom)
    except (AuditError, OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"audit_public: ERROR: {error}", file=sys.stderr)
        return 1
    # A finding may occur in many commits/index/worktree; report it once, retaining all origins.
    grouped: dict[tuple[str, str], set[str]] = {}
    for finding in findings:
        grouped.setdefault((finding.path, finding.reason), set()).add(finding.source)
    for (path, reason), sources in sorted(grouped.items()):
        print(f"FAIL {path!r}: {reason} [{', '.join(sorted(sources))}]")
    status = "FAIL" if findings else "PASS"
    rom_status = f"{summary['roms']} ROM(s) indexed" if summary["rom_check"] else "ROM comparison SKIPPED"
    print(f"audit_public: {status}; {summary['blobs']} unique Git blobs, {summary['trees']} trees, "
          f"{summary['tracked_worktree_files']} tracked worktree files; {rom_status}; "
          f"{len(grouped)} finding(s); {summary['seconds']:.3f}s")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
