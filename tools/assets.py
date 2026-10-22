"""Extract private, named Genesis assets and rebuild them without reading the ROM.

The committed config contains formats, addresses and provenance, never payload bytes.
Each owning unit has exactly one asset. PNG tEXt / WAV tjmd chunks carry layout
metadata. Most RLE streams use a pixel-only encoder; exceptional operation/count
plans live only in private PNG tjRP chunks, never in the committed config.
Formats are restated from bstreiff/tje-disasm under ruling R003, not copied source.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import time
import zlib
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve()
ROOT = next((p for p in HERE.parents if (p / "config/title.json").is_file()), HERE.parents[1])
CONFIG_ROOT = HERE.parents[1] / "config"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
KINDS = {"vectors", "cartridge_header", "palette", "tiles", "metasprite", "sprite_header",
         "pointer_table", "pcm", "z80_binary", "text", "values", "padding", "raw", "records", "animation", "music"}
HEADER_FIELDS = (
    ("console", 16, "text"), ("copyright", 16, "text"),
    ("domestic_title", 48, "text"), ("overseas_title", 48, "text"),
    ("serial_revision", 14, "text"), ("checksum", 2, "number"),
    ("io_support", 16, "text"), ("rom_start", 4, "number"), ("rom_end", 4, "number"),
    ("ram_start", 4, "number"), ("ram_end", 4, "number"), ("sram_flags", 4, "text"),
    ("sram_start", 4, "number"), ("sram_end", 4, "number"),
    ("modem", 12, "text"), ("memo", 40, "text"), ("region", 16, "text"),
)


class AssetError(ValueError):
    """A malformed asset or an inconsistent unit description."""


def integer(value: int | str) -> int:
    return int(value, 0) if isinstance(value, str) else value


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, ensure_ascii=True, indent=2) + "\n").encode("ascii")


def asset_path(root: Path, relative: str) -> Path:
    path = root / relative
    if Path(relative).is_absolute() or ".." in Path(relative).parts:
        raise AssetError(f"unsafe asset path: {relative}")
    return path


def validate_config(config: dict, catalogue: dict | None = None) -> None:
    if config.get("schema_version") != 1:
        raise AssetError("unsupported data-unit schema")
    names, paths = set(), set()
    previous_end = 0
    units = config["units"]
    for unit in units:
        name = unit["name"]
        start, size = integer(unit["address"]), unit["size"]
        if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) or name in names:
            raise AssetError(f"invalid/duplicate unit name: {name}")
        if unit["kind"] not in KINDS or size <= 0 or start < previous_end:
            raise AssetError(f"invalid/overlapping unit: {name}")
        if start + size > config["rom_size"]:
            raise AssetError(f"unit outside ROM: {name}")
        asset_path(Path("."), unit["asset"])
        if unit["asset"] in paths or not unit.get("provenance"):
            raise AssetError(f"duplicate asset or missing provenance: {name}")
        if unit["kind"] == "raw" and not unit.get("raw_reason"):
            raise AssetError(f"raw unit lacks a reason: {name}")
        names.add(name)
        paths.add(unit["asset"])
        previous_end = start + size
    for unit in units:
        if unit.get("palette") and unit["palette"] not in names:
            raise AssetError(f"unknown palette: {unit['palette']}")
    for name, address in config.get("symbols", {}).items():
        if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) or not 0 <= integer(address) <= 0xFFFFFFFF:
            raise AssetError(f"invalid symbol: {name}")
    if catalogue is not None:
        if config["version"] != catalogue["version"] or config["rom_sha256"] != catalogue["rom_sha256"]:
            raise AssetError("catalogue/version identity mismatch")
        wanted = [(catalogue["header"]["address"], catalogue["header"]["size"])]
        wanted += [(d["address"], d["size"]) for d in catalogue["data"]]
        index = 0
        for start, size in sorted(wanted):
            cursor = start
            while index < len(units) and integer(units[index]["address"]) < start + size:
                unit = units[index]
                if integer(unit["address"]) != cursor or cursor + unit["size"] > start + size:
                    raise AssetError(f"data partition mismatch at 0x{cursor:06x}")
                cursor += unit["size"]
                index += 1
            if cursor != start + size:
                raise AssetError(f"unowned data at 0x{cursor:06x}")
        if index != len(units):
            raise AssetError("unit overlaps catalogued code")


def pointer(value: int, symbols: dict) -> dict:
    if value == 0:
        return {"value": 0}
    names = [name for name, address in symbols.items() if integer(address) == value]
    if names:
        return {"symbol": names[0]}
    return {"value": value}


def resolve_pointer(value: dict, symbols: dict) -> int:
    if "symbol" in value:
        if value["symbol"] not in symbols:
            raise AssetError(f"unknown pointer symbol: {value['symbol']}")
        return integer(symbols[value["symbol"]]) + value.get("addend", 0)
    return value["value"]


def decode_palette(data: bytes) -> dict:
    if len(data) % 2:
        raise AssetError("odd palette length")
    colours = []
    for (word,) in struct.iter_unpack(">H", data):
        colours.append({"r": (word >> 1) & 7, "g": (word >> 5) & 7, "b": (word >> 9) & 7,
                        "unused_bits": word & ~0x0EEE})
    return {"format": "genesis-cram-9bit", "colours": colours}


def encode_palette(asset: dict) -> bytes:
    result = bytearray()
    for c in asset["colours"]:
        if any(not 0 <= c[k] <= 7 for k in ("r", "g", "b")) or c["unused_bits"] & 0x0EEE:
            raise AssetError("invalid CRAM colour")
        result.extend(struct.pack(">H", (c["b"] << 9) | (c["g"] << 5) | (c["r"] << 1) | c["unused_bits"]))
    return bytes(result)


def palette_rgb(asset: dict) -> list[tuple[int, int, int]]:
    return [tuple(c[k] * 255 // 7 for k in ("r", "g", "b")) for c in asset["colours"]]


def decode_rle_token(token: int, mode: str) -> tuple[str, int]:
    if mode not in ("simple", "interleaved"):
        raise AssetError("invalid RLE mode")
    if token >= 0xC0:
        op, count = "repeat", 256 - token
    elif token >= 0x80:
        op, count = "zero", 192 - token
    elif token == 0:
        raise AssetError("zero literal count underflows the original DBRA loop")
    elif token < 0x40:
        op, count = "literal", token
    elif mode == "interleaved" and token >= 0x60:
        op, count = "alternate", token & 31
    else:
        op, count = "ones", token & (31 if mode == "interleaved" else 63)
    # Original consumers enter the DBRA body unconditionally after count-1.
    # FF: 025AE4..025AF0 / 025B76..025B80; alternating pairs: 025AC2..025ADA.
    if op == "ones" and count == 0:
        raise AssetError("zero FF count underflows the original DBRA loop")
    if op == "alternate" and count < 2:
        raise AssetError("alternating count below two underflows the original pair DBRA loop")
    return op, count


def encode_rle_token(op: str, count: int, mode: str) -> int:
    limits = {"zero": (1, 64), "repeat": (1, 64), "literal": (1, 63),
              "ones": (1, 31 if mode == "interleaved" else 63), "alternate": (2, 31)}
    if (mode not in ("simple", "interleaved") or op not in limits
            or (op == "alternate" and mode != "interleaved")
            or not limits[op][0] <= count <= limits[op][1]):
        raise AssetError("invalid RLE operation/count")
    if op == "literal":
        return count
    if op == "repeat":
        return 256 - count
    if op == "zero":
        return 192 - count
    return (0x60 if op == "alternate" else 0x40) | count


def decode_rle(data: bytes, output_size: int, mode: str) -> tuple[bytes, list, int]:
    """Decode bounded pixels and record operation/count choices, never operands."""
    if mode not in ("simple", "interleaved") or output_size <= 0 or (mode == "interleaved" and output_size % 4):
        raise AssetError("invalid RLE mode/output size")
    lanes = 4 if mode == "interleaved" else 1
    result = bytearray(output_size)
    plans = []
    pos = 0
    for lane in range(lanes):
        decoded = bytearray()
        plan = []
        limit = output_size // lanes
        while len(decoded) < limit:
            if pos >= len(data):
                raise AssetError("truncated RLE token")
            op, count = decode_rle_token(data[pos], mode)
            pos += 1
            if len(decoded) + count > limit:
                raise AssetError("RLE run exceeds output dimensions")
            payload_size = count if op == "literal" else int(op in ("repeat", "alternate"))
            if pos + payload_size > len(data):
                raise AssetError("truncated RLE payload")
            if op == "literal":
                decoded.extend(data[pos:pos + count])
            elif op in ("repeat", "alternate"):
                value = data[pos]
                swapped = ((value & 15) << 4) | (value >> 4)
                decoded.extend(value if op == "repeat" or i % 2 == 0 else swapped for i in range(count))
            else:
                decoded.extend(bytes([0 if op == "zero" else 255]) * count)
            pos += payload_size
            plan.append([op, count])
        result[lane::lanes] = decoded
        plans.append(plan)
    return bytes(result), plans, pos


def encode_rle(data: bytes, plans: list, mode: str) -> bytes:
    lanes = 4 if mode == "interleaved" else 1
    if mode not in ("simple", "interleaved") or len(plans) != lanes or len(data) % lanes:
        raise AssetError("invalid RLE encoding plan")
    result = bytearray()
    for lane, plan in enumerate(plans):
        pixels = data[lane::lanes]
        cursor = 0
        for op, count in plan:
            token = encode_rle_token(op, count, mode)
            run = pixels[cursor:cursor + count]
            if len(run) != count:
                raise AssetError("RLE plan exceeds pixel extent")
            result.append(token)
            if op == "literal":
                result.extend(run)
            elif op in ("repeat", "alternate"):
                value = run[0]
                swapped = ((value & 15) << 4) | (value >> 4)
                expected = bytes(value if op == "repeat" or i % 2 == 0 else swapped for i in range(count))
                if run != expected:
                    raise AssetError("edited pixels no longer fit the stored RLE choices")
                result.append(value)
            elif run != bytes([0 if op == "zero" else 255]) * count:
                raise AssetError("edited pixels violate a constant RLE run")
            cursor += count
        if cursor != len(pixels):
            raise AssetError("RLE plan leaves pixels unencoded")
    return bytes(result)


def deterministic_lane_plan(data: bytes, mode: str) -> list:
    """Inferred encoder rule: maximal runs, local short-run merge, zero/FF forms.

    No address, resource name, original stream or extracted choice enters this rule.
    Equal-byte runs take precedence over nibble-alternation. The original encoder
    uses 63-byte ordinary runs/literals and at most 31 alternating bytes. A single
    two-byte run between literal groups is folded into one <=63-byte literal;
    chains of short runs retain their independent tokens. Finally zero/FF runs
    use their single-token forms when representable, including isolated zero.
    """
    if not data or mode not in ("simple", "interleaved"):
        raise AssetError("invalid deterministic RLE input")

    def run_at(pos: int) -> tuple[str, int] | None:
        value = data[pos]
        end = len(data)
        if pos + 1 < end and data[pos + 1] == value:
            count = 2
            while count < 63 and pos + count < end and data[pos + count] == value:
                count += 1
            return "repeat", count
        swapped = ((value & 15) << 4) | (value >> 4)
        if mode == "interleaved" and pos + 1 < end and data[pos + 1] == swapped:
            count = 2
            while count < 31 and pos + count < end and data[pos + count] == (value if count % 2 == 0 else swapped):
                count += 1
            return "alternate", count
        return None

    plan, pos = [], 0
    while pos < len(data):
        run = run_at(pos)
        if run is None:
            count = 1
            while count < 63 and pos + count < len(data) and run_at(pos + count) is None:
                count += 1
            run = "literal", count
        plan.append(list(run))
        pos += run[1]
    index = 1
    while index + 1 < len(plan):
        left, middle, right = plan[index - 1:index + 2]
        if (left[0] == right[0] == "literal" and middle[0] in ("repeat", "alternate")
                and middle[1] == 2 and left[1] + 2 + right[1] <= 63):
            plan[index - 1:index + 2] = [["literal", left[1] + 2 + right[1]]]
            index = max(1, index - 1)
        else:
            index += 1
    pos = 0
    for item in plan:
        op, count = item
        if op == "repeat":
            if data[pos] == 0:
                item[0] = "zero"
            elif data[pos] == 255 and count <= (31 if mode == "interleaved" else 63):
                item[0] = "ones"
        elif op == "literal" and count == 1 and data[pos] == 0:
            item[0] = "zero"
        pos += count
    return plan


def encode_deterministic_rle(data: bytes, mode: str) -> bytes:
    lanes = 4 if mode == "interleaved" else 1
    if len(data) % lanes:
        raise AssetError("RLE pixels do not divide into lanes")
    plans = [deterministic_lane_plan(data[lane::lanes], mode) for lane in range(lanes)]
    return encode_rle(data, plans, mode)


def encode_compact_plan(plans: list, mode: str) -> bytes:
    """Private PNG payload: raw/zlib selector, then one opcode/count byte per run."""
    tokens = bytes(encode_rle_token(op, count, mode) for lane in plans for op, count in lane)
    compressed = zlib.compress(tokens, 9)
    return b"\x01" + compressed if len(compressed) < len(tokens) else b"\x00" + tokens


def compact_plan_tokens(payload: bytes) -> bytes:
    if not payload:
        raise AssetError("empty compact RLE plan")
    if payload[0] == 0:
        return payload[1:]
    if payload[0] == 1:
        return zlib.decompress(payload[1:])
    raise AssetError("unknown compact RLE plan codec")


def decode_compact_plan(payload: bytes, output_size: int, mode: str) -> list:
    tokens = compact_plan_tokens(payload)
    lanes = 4 if mode == "interleaved" else 1
    if mode not in ("simple", "interleaved") or output_size <= 0 or output_size % lanes:
        raise AssetError("invalid compact RLE plan dimensions")
    plans, pos = [], 0
    for lane in range(lanes):
        plan, produced = [], 0
        while produced < output_size // lanes:
            if pos >= len(tokens):
                raise AssetError("truncated compact RLE plan")
            op, count = decode_rle_token(tokens[pos], mode)
            pos += 1
            produced += count
            if produced > output_size // lanes:
                raise AssetError("compact RLE run exceeds lane dimensions")
            plan.append([op, count])
        plans.append(plan)
    if pos != len(tokens):
        raise AssetError("trailing compact RLE tokens")
    return plans


def tiles_to_pixels(data: bytes, width_tiles: int, height_tiles: int, order: str) -> list[bytes]:
    if len(data) != width_tiles * height_tiles * 32 or order not in ("column", "row"):
        raise AssetError("invalid tile layout")
    rows = [bytearray(width_tiles * 8) for _ in range(height_tiles * 8)]
    for tile in range(width_tiles * height_tiles):
        tx, ty = (tile // height_tiles, tile % height_tiles) if order == "column" else (tile % width_tiles, tile // width_tiles)
        for y in range(8):
            for x in range(4):
                value = data[tile * 32 + y * 4 + x]
                rows[ty * 8 + y][tx * 8 + x * 2:tx * 8 + x * 2 + 2] = bytes((value >> 4, value & 15))
    return [bytes(row) for row in rows]


def pixels_to_tiles(rows: list[bytes], width_tiles: int, height_tiles: int, order: str) -> bytes:
    if len(rows) != height_tiles * 8 or any(len(row) != width_tiles * 8 for row in rows) or order not in ("column", "row"):
        raise AssetError("PNG dimensions do not match tile layout")
    result = bytearray()
    for tile in range(width_tiles * height_tiles):
        tx, ty = (tile // height_tiles, tile % height_tiles) if order == "column" else (tile % width_tiles, tile // width_tiles)
        for y in range(8):
            row = rows[ty * 8 + y]
            for x in range(0, 8, 2):
                a, b = row[tx * 8 + x:tx * 8 + x + 2]
                if a > 15 or b > 15:
                    raise AssetError("tile palette index exceeds 4bpp")
                result.append((a << 4) | b)
    return bytes(result)


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)


def encode_png(rows: list[bytes], palette: list, metadata: dict) -> bytes:
    width, height = len(rows[0]), len(rows)
    if width % 2 or len(palette) != 16 or any(len(row) != width or any(v > 15 for v in row) for row in rows):
        raise AssetError("expected even-width 4bpp indexed pixels and 16 colours")
    scanlines = bytearray()
    for row in rows:
        scanlines.append(0)
        scanlines.extend((row[i] << 4) | row[i + 1] for i in range(0, width, 2))
    plan = metadata.get("rle_plan")
    if plan is not None and not isinstance(plan, bytes):
        raise AssetError("compact RLE plan must be a private binary PNG chunk")
    layout = {key: value for key, value in metadata.items() if key != "rle_plan"}
    private_plan = png_chunk(b"tjRP", plan) if plan is not None else b""
    return (PNG_SIGNATURE + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 4, 3, 0, 0, 0))
            + png_chunk(b"PLTE", bytes(c for colour in palette for c in colour))
            + png_chunk(b"tRNS", b"\x00" + b"\xff" * 15)
            + png_chunk(b"tEXt", b"tje-layout\x00" + json.dumps(layout, separators=(",", ":")).encode("ascii"))
            + private_plan
            + png_chunk(b"IDAT", zlib.compress(bytes(scanlines), 9)) + png_chunk(b"IEND", b""))


def decode_png(data: bytes) -> tuple[list[bytes], dict]:
    if not data.startswith(PNG_SIGNATURE):
        raise AssetError("not a PNG")
    pos, packed, metadata, header = 8, bytearray(), None, None
    private_plan = None
    ended = False
    while pos < len(data):
        if pos + 12 > len(data):
            raise AssetError("truncated PNG chunk")
        size = struct.unpack_from(">I", data, pos)[0]
        kind, payload = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + size]
        if pos + size + 12 > len(data) or zlib.crc32(kind + payload) & 0xFFFFFFFF != struct.unpack_from(">I", data, pos + 8 + size)[0]:
            raise AssetError("PNG chunk length/CRC mismatch")
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            packed.extend(payload)
        elif kind == b"tEXt" and payload.startswith(b"tje-layout\x00"):
            metadata = json.loads(payload.split(b"\x00", 1)[1])
        elif kind == b"tjRP":
            if private_plan is not None:
                raise AssetError("duplicate private RLE plan chunk")
            private_plan = payload
        elif kind == b"IEND":
            ended = True
            break
        pos += size + 12
    if not ended or header is None or metadata is None:
        raise AssetError("PNG lacks header, end marker or reversible tje-layout metadata")
    if private_plan is not None:
        metadata["rle_plan"] = private_plan
    width, height, depth, colour, compression, filtering, interlace = header
    if depth != 4 or colour != 3 or compression or filtering or interlace or not width or not height:
        raise AssetError("expected non-interlaced 4bpp indexed PNG")
    stride = (width + 1) // 2
    raw = zlib.decompress(packed)
    if len(raw) != (stride + 1) * height:
        raise AssetError("PNG scanline length mismatch")
    rows, prior = [], bytes(stride)
    for y in range(height):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for x in range(stride):
            left, up, corner = line[x - 1] if x else 0, prior[x], prior[x - 1] if x else 0
            if f == 0:
                predictor = 0
            elif f == 1:
                predictor = left
            elif f == 2:
                predictor = up
            elif f == 3:
                predictor = (left + up) // 2
            elif f == 4:
                p = left + up - corner
                distances = (abs(p - left), abs(p - up), abs(p - corner))
                predictor = (left, up, corner)[distances.index(min(distances))]
            else:
                raise AssetError("unknown PNG filter")
            line[x] = (line[x] + predictor) & 255
        rows.append(bytes(v for b in line for v in (b >> 4, b & 15))[:width])
        prior = line
    return rows, metadata


def riff_chunk(kind: bytes, data: bytes) -> bytes:
    return kind + struct.pack("<I", len(data)) + data + (b"\x00" if len(data) % 2 else b"")


def encode_wav(samples: bytes, rate: int, metadata: dict) -> bytes:
    body = (b"WAVE" + riff_chunk(b"fmt ", struct.pack("<HHIIHH", 1, 1, rate, rate, 1, 8))
            + riff_chunk(b"tjmd", json_bytes(metadata)) + riff_chunk(b"data", samples))
    return b"RIFF" + struct.pack("<I", len(body)) + body


def decode_wav(data: bytes) -> tuple[bytes, int, dict]:
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE" or len(data) < 12 or struct.unpack_from("<I", data, 4)[0] != len(data) - 8:
        raise AssetError("invalid WAV/RIFF length")
    pos, samples, metadata, fmt = 12, None, None, None
    while pos < len(data):
        if pos + 8 > len(data):
            raise AssetError("truncated WAV chunk")
        size = struct.unpack_from("<I", data, pos + 4)[0]
        kind, payload = data[pos:pos + 4], data[pos + 8:pos + 8 + size]
        if len(payload) != size:
            raise AssetError("truncated WAV payload")
        if kind == b"fmt ":
            fmt = struct.unpack("<HHIIHH", payload[:16])
        elif kind == b"tjmd":
            metadata = json.loads(payload)
        elif kind == b"data":
            samples = payload
        pos += 8 + size + (size & 1)
    if fmt is None or samples is None or metadata is None or fmt[0:2] != (1, 1) or fmt[4:6] != (1, 8):
        raise AssetError("expected mono 8-bit PCM WAV with tjmd metadata")
    return samples, fmt[2], metadata


def decode_sprite(data: bytes, symbols: dict) -> dict:
    if len(data) != 10:
        raise AssetError("sprite header must be ten bytes")
    w, h, x, y, z, flags, address = struct.unpack(">BBbbbBI", data)
    return {"width_tiles": w, "height_tiles": h, "x": x, "y": y, "z": z,
            "flags": flags, "tiles": pointer(address, symbols)}


def encode_sprite(asset: dict, symbols: dict) -> bytes:
    return struct.pack(">BBbbbBI", *(asset[k] for k in ("width_tiles", "height_tiles", "x", "y", "z", "flags")), resolve_pointer(asset["tiles"], symbols))


def decode_records(data: bytes, fields: list, symbols: dict | None = None) -> list:
    stride = sum(f["width"] for f in fields)
    if len(data) % stride:
        raise AssetError("record extent is not a multiple of its stride")
    records = []
    for start in range(0, len(data), stride):
        record, pos = {}, start
        for field in fields:
            end = pos + field["width"]
            value = int.from_bytes(data[pos:end], "big", signed=field.get("signed", False))
            record[field["name"]] = pointer(value, symbols or {}) if field.get("type") == "pointer" else value
            pos = end
        records.append(record)
    return records


def encode_records(records: list, fields: list, symbols: dict | None = None) -> bytes:
    chunks = []
    for record in records:
        for field in fields:
            value = record[field["name"]]
            if field.get("type") == "pointer":
                value = resolve_pointer(value, symbols or {})
            chunks.append(value.to_bytes(field["width"], "big", signed=field.get("signed", False)))
    return b"".join(chunks)


def decode_music(data: bytes, address: int, symbols: dict) -> list:
    """Restate the 68000 sound VM's bytecode, preserving callback alignment."""
    names = ("end", "key_on", "key_off", "frequency", "instrument", "loop_start",
             "loop_repeat", "callback", "jump", "scratch", "clap", "bass_drum", "snare")
    commands, pos = [], 0
    while pos < len(data):
        opcode = data[pos]
        pos += 1
        if opcode & 128:
            commands.append({"op": "wait", "ticks": opcode & 127})
            continue
        if opcode >= len(names):
            raise AssetError(f"unknown music opcode at 0x{address + pos - 1:06x}")
        command = {"op": names[opcode]}
        width = 2 if opcode in (1, 3, 5, 6, 8) else 1 if opcode == 4 else 0
        if opcode == 7:
            if (address + pos) & 1:
                if pos >= len(data):
                    raise AssetError("missing music callback alignment byte")
                command["alignment"] = data[pos]
                pos += 1
            width = 4
        if pos + width > len(data):
            raise AssetError("truncated music operand")
        if width:
            value = int.from_bytes(data[pos:pos + width], "big", signed=opcode in (6, 8))
            command["operand"] = pointer(value, symbols) if opcode == 7 else value
        pos += width
        commands.append(command)
    return commands


def encode_music(commands: list, address: int, symbols: dict) -> bytes:
    names = ("end", "key_on", "key_off", "frequency", "instrument", "loop_start",
             "loop_repeat", "callback", "jump", "scratch", "clap", "bass_drum", "snare")
    output = bytearray()
    for command in commands:
        if command["op"] == "wait":
            if not 0 <= command["ticks"] <= 127:
                raise AssetError("invalid music wait")
            output.append(128 | command["ticks"])
            continue
        opcode = names.index(command["op"])
        output.append(opcode)
        width = 2 if opcode in (1, 3, 5, 6, 8) else 1 if opcode == 4 else 0
        if opcode == 7:
            if (address + len(output)) & 1:
                output.append(command["alignment"])
            width = 4
        if width:
            value = resolve_pointer(command["operand"], symbols) if opcode == 7 else command["operand"]
            output.extend(value.to_bytes(width, "big", signed=opcode in (6, 8)))
    return bytes(output)


def extract_unit(unit: dict, data: bytes, config: dict, palette: dict | None = None) -> bytes:
    kind, params, symbols = unit["kind"], unit.get("params", {}), config.get("symbols", {})
    if kind in ("raw", "z80_binary"):
        return data
    if kind == "tiles":
        mode = params["compression"]
        if mode == "none":
            pixels, plan, used = data[:params["decoded_size"]], None, params["decoded_size"]
        else:
            pixels, plan, used = decode_rle(data, params["decoded_size"], mode)
        metadata = {"compression": mode, "width_tiles": params["width_tiles"], "height_tiles": params["height_tiles"],
                    "order": params["order"], "tail": list(data[used:]),
                    "palette": unit["palette"], "encoder": "uncompressed-v2"}
        if mode != "none":
            if encode_deterministic_rle(pixels, mode) == data[:used]:
                metadata["encoder"] = "maxrun-merge-v2"
            else:
                metadata["encoder"] = "token-plan-v2"
                metadata["rle_plan"] = encode_compact_plan(plan, mode)
        return encode_png(tiles_to_pixels(pixels, params["width_tiles"], params["height_tiles"], params["order"]), palette_rgb(palette), metadata)
    if kind == "pcm":
        count = params.get("header_bytes", 2)
        header = data[:count]
        samples = data[count:]
        if params.get("signed", False):
            samples = bytes(v ^ 128 for v in samples)
        metadata = {"rom_header_u16_be": int.from_bytes(header, "big"), "rom_header_bytes": count,
                    "payload_bytes": len(samples), "signed_rom_samples": params.get("signed", False),
                    "rate_evidence": params["rate_evidence"], "length_policy": "preserve-header-not-recompute"}
        return encode_wav(samples, params["sample_rate"], metadata)
    if kind == "palette":
        asset = decode_palette(data)
    elif kind in ("vectors", "pointer_table"):
        asset = {"entries": [pointer(int.from_bytes(data[i:i + 4], "big"), symbols) for i in range(0, len(data), 4)]}
        if kind == "vectors" and params.get("initial_stack_symbol"):
            asset["entries"][0] = {"symbol": params["initial_stack_symbol"]}
    elif kind == "cartridge_header":
        asset, pos = {}, 0
        for name, width, field_kind in HEADER_FIELDS:
            value = data[pos:pos + width]
            asset[name] = value.decode("latin-1") if field_kind == "text" else int.from_bytes(value, "big")
            pos += width
    elif kind == "sprite_header":
        asset = decode_sprite(data, symbols)
    elif kind == "metasprite":
        if len(data) != 4 + 10 * data[0]:
            raise AssetError("metasprite count/size mismatch")
        asset = {"prefix_unknown": list(data[1:4]), "sprites": [decode_sprite(data[i:i + 10], symbols) for i in range(4, len(data), 10)]}
    elif kind == "text":
        asset = {"encoding": "latin-1", "text": data.decode("latin-1")}
    elif kind == "animation":
        asset = {"commands": [{"op": "loop" if v == 255 else "hold"} if v >= 254
                               else {"frame": v & 127, "hflip": bool(v & 128)} for v in data]}
    elif kind == "music":
        asset = {"commands": decode_music(data, integer(unit["address"]), symbols)}
    elif kind == "values":
        width = params["width"]
        asset = {"values": [int.from_bytes(data[i:i + width], "big", signed=params.get("signed", False)) for i in range(0, len(data), width)]}
    elif kind == "records":
        asset = {"records": decode_records(data, params["fields"], symbols)}
    elif kind == "padding":
        if len(set(data)) != 1:
            raise AssetError("nonconstant padding")
        asset = {"fill": data[0], "count": len(data)}
    else:
        raise AssetError(f"unknown asset kind: {kind}")
    return json_bytes(asset)


def pack_unit(unit: dict, asset: bytes, config: dict) -> bytes:
    kind, params, symbols = unit["kind"], unit.get("params", {}), config.get("symbols", {})
    if kind in ("raw", "z80_binary"):
        result = asset
    elif kind == "tiles":
        rows, meta = decode_png(asset)
        for field in ("compression", "width_tiles", "height_tiles", "order"):
            if meta[field] != params[field]:
                raise AssetError(f"PNG/config layout mismatch: {field}")
        if meta["palette"] != unit["palette"]:
            raise AssetError("PNG/config palette mismatch")
        pixels = pixels_to_tiles(rows, params["width_tiles"], params["height_tiles"], params["order"])
        if params["compression"] == "none":
            encoded = pixels
        elif meta["encoder"] == "maxrun-merge-v2":
            if "rle_plan" in meta:
                raise AssetError("deterministic asset contains a redundant private plan")
            encoded = encode_deterministic_rle(pixels, params["compression"])
        elif meta["encoder"] == "token-plan-v2":
            plan = decode_compact_plan(meta["rle_plan"], len(pixels), params["compression"])
            encoded = encode_rle(pixels, plan, params["compression"])
        else:
            raise AssetError("unknown private PNG RLE encoder")
        result = encoded + bytes(meta["tail"])
    elif kind == "pcm":
        samples, rate, meta = decode_wav(asset)
        if rate != params["sample_rate"] or len(samples) != meta["payload_bytes"] or meta["rom_header_bytes"] != params["header_bytes"]:
            raise AssetError("WAV/config format or length mismatch")
        if meta["signed_rom_samples"] != params.get("signed", False):
            raise AssetError("WAV/config signedness mismatch")
        if params.get("signed", False):
            samples = bytes(v ^ 128 for v in samples)
        result = meta["rom_header_u16_be"].to_bytes(params["header_bytes"], "big") + samples
    else:
        obj = json.loads(asset)
        if kind == "palette":
            result = encode_palette(obj)
        elif kind in ("vectors", "pointer_table"):
            result = b"".join(struct.pack(">I", resolve_pointer(p, symbols)) for p in obj["entries"])
        elif kind == "cartridge_header":
            chunks = []
            for name, width, field_kind in HEADER_FIELDS:
                chunk = obj[name].encode("latin-1") if field_kind == "text" else obj[name].to_bytes(width, "big")
                if len(chunk) != width:
                    raise AssetError(f"cartridge header width changed: {name}")
                chunks.append(chunk)
            result = b"".join(chunks)
        elif kind == "sprite_header":
            result = encode_sprite(obj, symbols)
        elif kind == "metasprite":
            if len(obj["prefix_unknown"]) != 3 or len(obj["sprites"]) > 127:
                raise AssetError("invalid metasprite prefix/count")
            result = bytes([len(obj["sprites"]), *obj["prefix_unknown"]]) + b"".join(encode_sprite(s, symbols) for s in obj["sprites"])
        elif kind == "text":
            result = obj["text"].encode("latin-1")
        elif kind == "animation":
            result = bytes(255 if c.get("op") == "loop" else 254 if c.get("op") == "hold"
                           else c["frame"] | (128 if c["hflip"] else 0) for c in obj["commands"])
        elif kind == "music":
            result = encode_music(obj["commands"], integer(unit["address"]), symbols)
        elif kind == "values":
            result = b"".join(v.to_bytes(params["width"], "big", signed=params.get("signed", False)) for v in obj["values"])
        elif kind == "records":
            result = encode_records(obj["records"], params["fields"], symbols)
        elif kind == "padding":
            result = bytes([obj["fill"]]) * obj["count"]
        else:
            raise AssetError(f"unknown asset kind: {kind}")
    if len(result) != unit["size"]:
        raise AssetError(f"{unit['name']}: rebuilt {len(result)} bytes, expected {unit['size']}")
    return result


def rle_storage_summary(config: dict, asset_root: Path) -> dict:
    """Measure actual private-plan storage, including PNG and encoder-tag overhead."""
    result = {"compressed_units": 0, "original_bytes": 0, "deterministic_units": 0, "planned_units": 0,
              "plan_intrinsic_token_bytes": 0, "plan_payload_bytes": 0, "plan_chunk_bytes": 0,
              "plan_encoder_tag_bytes": 0, "plan_bytes": 0, "plan_bytes_ge_original_units": 0,
              "per_mode": {}}
    for unit in config["units"]:
        if unit["kind"] != "tiles" or unit["params"]["compression"] == "none":
            continue
        _, metadata = decode_png(asset_path(asset_root, unit["asset"]).read_bytes())
        mode = unit["params"]["compression"]
        counter = result["per_mode"].setdefault(mode, {"deterministic": 0, "planned": 0})
        result["compressed_units"] += 1
        result["original_bytes"] += unit["size"]
        if metadata["encoder"] == "maxrun-merge-v2":
            if "rle_plan" in metadata:
                raise AssetError("redundant plan on deterministic PNG")
            result["deterministic_units"] += 1
            counter["deterministic"] += 1
        elif metadata["encoder"] == "token-plan-v2":
            payload = metadata["rle_plan"]
            # Count the exact private chunk (length, type, data, CRC) and its JSON
            # encoder field plus separator; not just the intrinsic token entropy.
            chunk_size = len(payload) + 12
            tag_size = len(json.dumps({"encoder": metadata["encoder"]}, separators=(",", ":"))) - 1
            result["planned_units"] += 1
            counter["planned"] += 1
            result["plan_intrinsic_token_bytes"] += len(compact_plan_tokens(payload))
            result["plan_payload_bytes"] += len(payload)
            result["plan_chunk_bytes"] += chunk_size
            result["plan_encoder_tag_bytes"] += tag_size
            result["plan_bytes"] += chunk_size + tag_size
            result["plan_bytes_ge_original_units"] += int(chunk_size + tag_size >= unit["size"])
        else:
            raise AssetError("unknown private PNG RLE encoder in summary")
    result["plan_original_ratio"] = result["plan_bytes"] / result["original_bytes"] if result["original_bytes"] else 0
    return result


def extract(config: dict, rom: bytes, out: Path) -> dict:
    started = time.perf_counter()
    if len(rom) != config["rom_size"] or digest(rom) != config["rom_sha256"]:
        raise AssetError("input ROM does not match data-unit config")
    validate_config(config)
    palettes = {u["name"]: decode_palette(rom[integer(u["address"]):integer(u["address"]) + u["size"]])
                for u in config["units"] if u["kind"] == "palette"}
    manifest = []
    for unit in config["units"]:
        start = integer(unit["address"])
        data = rom[start:start + unit["size"]]
        try:
            asset = extract_unit(unit, data, config, palettes.get(unit.get("palette")))
        except (ValueError, KeyError, struct.error) as exc:
            raise AssetError(f"{unit['name']}: {exc}") from exc
        path = asset_path(out, unit["asset"])
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(asset)
        manifest.append({"name": unit["name"], "sha256": digest(data)})
    rle_storage = rle_storage_summary(config, out)
    report = {"version": config["version"], "rom_sha256": config["rom_sha256"],
              "config_sha256": digest(json_bytes(config)), "counts": dict(Counter(u["kind"] for u in config["units"])),
              "elapsed_seconds": time.perf_counter() - started, "rle_storage": rle_storage, "units": manifest}
    (out / "extraction.json").write_bytes(json_bytes(report))
    return report


def pack(config: dict, assets: Path, out: Path) -> dict:
    started = time.perf_counter()
    validate_config(config)
    extracted = json.loads((assets / "extraction.json").read_text())
    if extracted["version"] != config["version"] or extracted["config_sha256"] != digest(json_bytes(config)):
        raise AssetError("assets were extracted with a different config")
    expected = {u["name"]: u["sha256"] for u in extracted["units"]}
    manifest, counts, raw_only = [], {}, []
    out.mkdir(parents=True, exist_ok=True)
    for unit in config["units"]:
        try:
            result = pack_unit(unit, asset_path(assets, unit["asset"]).read_bytes(), config)
        except (ValueError, KeyError, struct.error) as exc:
            raise AssetError(f"{unit['name']}: {exc}") from exc
        name = unit["name"]
        exact = digest(result) == expected[name]
        (out / f"{name}.bin").write_bytes(result)
        counter = counts.setdefault(unit["kind"], {"units": 0, "bytes": 0, "exact": 0, "changed": 0})
        counter["units"] += 1
        counter["bytes"] += len(result)
        counter["exact" if exact else "changed"] += 1
        manifest.append({"name": name, "kind": unit["kind"], "address": unit["address"], "size": len(result),
                         "path": f"{name}.bin", "sha256": digest(result), "exact": exact})
        if unit["kind"] == "raw":
            raw_only.append({"name": name, "size": len(result), "reason": unit["raw_reason"]})
    report = {"version": config["version"], "config_sha256": extracted["config_sha256"],
              "counts": counts, "all_exact": all(u["exact"] for u in manifest), "raw_only": raw_only,
              "elapsed_seconds": time.perf_counter() - started, "units": manifest}
    (out / "manifest.json").write_bytes(json_bytes(report))
    return report


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("direction", choices=("extract", "pack"))
    parser.add_argument("--version", default="rev02")
    parser.add_argument("--root", type=Path, default=ROOT, help="repository containing orig/ and catalogue")
    parser.add_argument("--config", type=Path, help="defaults to config beside this tool")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--assets", type=Path, help="extracted assets (pack only)")
    args = parser.parse_args(argv)
    config_path = args.config or CONFIG_ROOT / args.version / "data_units.json"
    try:
        config = json.loads(config_path.read_text())
        if config["version"] != args.version:
            raise AssetError("--version does not match config")
        catalogue = json.loads((args.root / "config" / args.version / "catalogue.json").read_text())
        validate_config(config, catalogue)
        if args.direction == "extract":
            report = extract(config, (args.root / "orig" / args.version / "baserom.bin").read_bytes(), args.out)
        else:
            if args.assets is None:
                parser.error("pack requires --assets")
            report = pack(config, args.assets, args.out)
    except (OSError, ValueError, KeyError, struct.error) as exc:
        parser.exit(1, f"assets: {exc}\n")
    print(json.dumps({k: v for k, v in report.items() if k != "units"}, indent=2))


if __name__ == "__main__":
    main()
