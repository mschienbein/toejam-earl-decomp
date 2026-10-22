"""Symbol names with provenance: config/<version>/names.json (standard library only).

    {"schema_version": 1, "version": "rev02",
     "symbols": [{"name": "GameProgram", "address": "0x0002fc", "kind": "function",
                  "size": 38, "source": "tje-disasm:src/main.S", "evidence": "...", "aliases": [...]}]}

kind: function | label (code addresses) | rom_data (ROM data) | ram (0xFF0000-0xFFFFFF).
Names are unique C identifiers and never use the generated prefixes (func_, loc_, dat_, ...), so
an imported name can never collide with a generated one.
"""
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KINDS = ("function", "label", "rom_data", "ram")
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
RESERVED = re.compile(r"^(func|frag|loc|dat|data|jtbl|header)_[0-9a-fA-F]{6}$")
ROM_END = 0x400000
RAM = range(0xFF0000, 0x1000000)


class NamesError(ValueError):
    pass


@dataclass(frozen=True)
class Name:
    name: str
    address: int
    kind: str
    size: int | None
    source: str
    aliases: tuple[str, ...] = ()


def path(version: str, root: Path = ROOT) -> Path:
    return root / "config" / version / "names.json"


def parse(document: dict, version: str | None = None) -> dict[int, Name]:
    """Validate a names document; the result maps each address to its single name."""
    if document.get("schema_version") != 1:
        raise NamesError("names: schema_version must be 1")
    if version is not None and document.get("version") != version:
        raise NamesError(f"names: file is for {document.get('version')!r}, not {version!r}")
    by_address: dict[int, Name] = {}
    seen: dict[str, int] = {}
    for entry in document.get("symbols", []):
        name, kind = entry.get("name", ""), entry.get("kind")
        if not IDENT.fullmatch(name) or RESERVED.fullmatch(name):
            raise NamesError(f"names: {name!r} is not a usable identifier (generated prefixes are reserved)")
        if kind not in KINDS:
            raise NamesError(f"names: {name}: unknown kind {kind!r}")
        try:
            address = int(entry["address"], 16)
        except (KeyError, TypeError, ValueError):
            raise NamesError(f"names: {name}: address must be a hex string") from None
        if kind == "ram" and address not in RAM:
            raise NamesError(f"names: {name}: RAM name outside 0xFF0000-0xFFFFFF")
        if kind != "ram" and not 0 <= address < ROM_END:
            raise NamesError(f"names: {name}: ROM name outside the cartridge space")
        aliases = [a if isinstance(a, str) else a.get("name", "") for a in entry.get("aliases", [])]
        for alias in [name, *aliases]:
            if alias != name and (not IDENT.fullmatch(alias) or RESERVED.fullmatch(alias)):
                raise NamesError(f"names: alias {alias!r} of {name} is not a usable identifier")
            if alias in seen and seen[alias] != address:
                raise NamesError(f"names: {alias!r} names two addresses")
            seen[alias] = address
        if address in by_address:
            raise NamesError(f"names: 0x{address:06x} has two names ({by_address[address].name}, {name}); list extras as aliases")
        size = entry.get("size")
        by_address[address] = Name(name, address, kind, int(size) if size is not None else None,
                                   entry.get("source", ""), tuple(a for a in aliases if a != name))
    return by_address


def load(version: str, root: Path = ROOT) -> dict[int, Name]:
    """The validated names for `version`, or no names when the file does not exist."""
    file = path(version, root)
    if not file.is_file():
        return {}
    return parse(json.loads(file.read_text(encoding="utf-8")), version)
