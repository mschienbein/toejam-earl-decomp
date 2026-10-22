"""Synthetic fixtures shared by author-tool tests; no retail ROM bytes."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import struct
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/title.json").is_file() and (p / "tools/m68k.py").is_file())
TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(TOOLS))

import analyze
import packet


def load_diff():
    spec = importlib.util.spec_from_file_location("author_diff", TOOLS / "diff.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def synthetic_rom() -> bytes:
    image = bytearray(0x1000)
    struct.pack_into(">II", image, 0, 0xFF8000, 0x200)
    image[0x200:0x20C] = bytes.fromhex("4eb900000300" "610000f8" "60fe")
    image[0x300:0x326] = bytes.fromhex(
        "0c400002" "6e1c" "d040" "303b0006" "4efb0002"
        "0006000a000e" "70014e75" "70024e75" "70ff4e75" "70004e75")
    image[0x400:0x410] = b"SYNTHETIC-DATA!!"
    return bytes(image)


def program_for(code: bytes, extra: list[dict] | None = None, names: dict | None = None):
    image = bytearray(0x1000)
    image[0x200:0x200 + len(code)] = code
    units = [{"address": 0x200, "size": len(code), "kind": "function"}]
    for unit in extra or []:
        raw = unit.get("raw", bytes.fromhex("4e75"))
        image[unit["address"]:unit["address"] + len(raw)] = raw
        units.append({key: value for key, value in {**unit, "size": len(raw), "kind": "function"}.items() if key != "raw"})
    catalogue = {"schema_version": 1, "version": "synthetic", "rom_sha256": hashlib.sha256(image).hexdigest(),
                 "functions": units, "data": [{"address": 0x400, "size": 0x100}], "labels": [], "jump_tables": []}
    return packet.Program(bytes(image), catalogue, packet.Symbols(catalogue, names), packet.decoder(ROOT))


def synthetic_repo(root: Path, image: bytes | None = None, names: dict | None = None) -> dict:
    image = image or synthetic_rom()
    for directory in ("tools", "config/rev02", "orig/rev02"):
        (root / directory).mkdir(parents=True, exist_ok=True)
    subprocess.run(["git", "-C", str(root), "init", "-q"], check=True, capture_output=True)
    (root / ".gitignore").write_text("/scratch/\n/orig/\n")
    # A private temporary fixture, not a second committed decoder.
    (root / "tools/m68k.py").write_bytes((ROOT / "tools/m68k.py").read_bytes())
    (root / "orig/rev02/baserom.bin").write_bytes(image)
    title = {"default_version": "rev02", "versions": {"rev02": {"orig": "orig/rev02/baserom.bin", "size": len(image), "sha256": hashlib.sha256(image).hexdigest()}}}
    (root / "config/title.json").write_text(json.dumps(title))
    catalogue = analyze.build_catalogue(image, "rev02", {})
    (root / "config/rev02/catalogue.json").write_text(json.dumps(catalogue))
    if names is not None:
        (root / "config/rev02/names.json").write_text(json.dumps(names))
    return catalogue


def elf_object(code: bytes, relocations: list[tuple[int, int, str, int]] = (), *, name: str = "candidate", size: int | None = None,
               rela: bool = True, prefix: bytes = b"", machine: int = 4, extra_symbols: list[tuple[str, int, int, int, int]] = ()) -> bytes:
    """Minimal real ELF32/m68k object, with REL or RELA and arbitrary extra symbols.

    Relocations: (offset within function, R_68K type, target symbol name, addend).
    Extra symbols: (name, value, size, section index, ELF st_info).
    """
    targets = list(dict.fromkeys(r[2] for r in relocations))
    records = [(name, len(prefix), len(code) if size is None else size, 1, 0x12)]
    records += [(target, 0, 0, 0, 0x10) for target in targets if target != name]
    records += list(extra_symbols)
    strings = bytearray(b"\0")
    symbols = bytearray(16)
    indices = {}
    for index, (sym_name, value, extent, section, info) in enumerate(records, 1):
        offset = len(strings)
        strings += sym_name.encode() + b"\0"
        symbols += struct.pack(">IIIBBH", offset, value, extent, info, 0, section)
        indices[sym_name] = index
    rel_data = bytearray()
    for offset, kind, target, addend in relocations:
        fields = (offset + len(prefix), indices[target] << 8 | kind)
        rel_data += struct.pack(">IIi", *fields, addend) if rela else struct.pack(">II", *fields)
    section_names = ["", ".text", ".rela.text" if rela else ".rel.text", ".symtab", ".strtab", ".shstrtab"]
    shstrings = bytearray(b"\0")
    offsets = [0]
    for section_name in section_names[1:]:
        offsets.append(len(shstrings))
        shstrings += section_name.encode() + b"\0"
    sections = [(b"", 0, 0, 0, 0, 0, 0), (prefix + code, 1, 6, 0, 0, 2, 0),
                (bytes(rel_data), 4 if rela else 9, 0, 3, 1, 4, 12 if rela else 8),
                (bytes(symbols), 2, 0, 4, 1, 4, 16), (bytes(strings), 3, 0, 0, 0, 1, 0),
                (bytes(shstrings), 3, 0, 0, 0, 1, 0)]
    blob = bytearray(52)
    headers = []
    for index, (payload, kind, flags, link, info, alignment, entsize) in enumerate(sections):
        while alignment and len(blob) % alignment:
            blob.append(0)
        offset = len(blob)
        blob += payload
        headers.append(struct.pack(">IIIIIIIIII", offsets[index], kind, flags, 0, offset, len(payload), link, info, alignment, entsize))
    while len(blob) % 4:
        blob.append(0)
    shoff = len(blob)
    blob += b"".join(headers)
    ident = bytes([0x7F]) + b"ELF" + bytes([1, 2, 1, 0]) + bytes(8)
    blob[:52] = struct.pack(">16sHHIIIIIHHHHHH", ident, 1, machine, 1, 0, 0, shoff, 0, 52, 0, 0, 40, len(sections), 5)
    return bytes(blob)
