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
    """The validated names for `version`, or no names when the file does not exist.

    For a revision other than the primary one, every function paired with a primary function
    (config/<version>/correspondence.json, tools/revmap.py) that has no name of its own takes the
    primary function's identity: its imported name, else the primary spelling `func_XXXXXX`. One
    C source then names each function the same way in every revision.

    The typed data units (config/<primary>/data_units.json) name every data unit start that has no
    imported name; where an imported name exists, the unit's name becomes an alias. Other revisions
    take them at the corresponding place (tools/revmap.py translator).
    """
    file = path(version, root)
    if not file.is_file():
        return {}
    by_address = parse(json.loads(file.read_text(encoding="utf-8")), version)
    taken = {spelling for n in by_address.values() for spelling in (n.name, *n.aliases)}
    pairs = correspondence(version, root)
    primary_version = pairs["primary"] if pairs else version
    if pairs:
        primary = parse(json.loads(path(pairs["primary"], root).read_text(encoding="utf-8")), pairs["primary"])
        catalogue = root / "config" / pairs["primary"] / "catalogue.json"
        fragments = {f["address"] for f in json.loads(catalogue.read_text(encoding="utf-8"))["functions"]
                     if f.get("kind") == "fragment"} if catalogue.is_file() else set()
        for theirs, ours, size in pairs["functions"]:
            if ours in by_address:
                continue
            fragment = theirs in fragments   # a fragment is labelled loc_XXXXXX, like the build names it
            identity = primary[theirs].name if theirs in primary else f"{'loc' if fragment else 'func'}_{theirs:06x}"
            if identity in taken:
                raise NamesError(f"names: {identity} (primary 0x{theirs:06x}) already names another {version} address")
            by_address[ours] = Name(identity, ours, "label" if fragment else "function", size,
                                    "revision correspondence (tools/revmap.py)")
            taken.add(identity)
    units = data_units(primary_version, root)
    if units:
        if version == primary_version:
            place = lambda address: address  # noqa: E731
        else:
            import revmap  # local import: revmap imports this module
            place = revmap.translator(version, root)
        for theirs, name, size in units:
            ours = place(theirs)
            if ours is None or name in taken:
                continue
            if not IDENT.fullmatch(name) or RESERVED.fullmatch(name):
                raise NamesError(f"names: data unit {name!r} is not a usable identifier")
            known = by_address.get(ours)
            if known is None:
                by_address[ours] = Name(name, ours, "rom_data", size, "typed data unit (config/data_units.json)")
            else:
                by_address[ours] = Name(known.name, ours, known.kind, known.size, known.source, (*known.aliases, name))
            taken.add(name)
    return by_address


def data_units(version: str, root: Path = ROOT) -> list[tuple[int, str, int]]:
    """(address, name, size) of the typed data units of `version`, if it has data_units.json."""
    file = root / "config" / version / "data_units.json"
    if not file.is_file():
        return []
    units = json.loads(file.read_text(encoding="utf-8"))["units"]
    return [(int(u["address"], 16), u["name"], u["size"]) for u in units]


def correspondence(version: str, root: Path = ROOT) -> dict | None:
    """config/<version>/correspondence.json as (primary address, own address, own size) triples."""
    file = root / "config" / version / "correspondence.json"
    if not file.is_file():
        return None
    document = json.loads(file.read_text(encoding="utf-8"))
    primary = document["primary"]
    return {"primary": primary,
            "functions": [(int(p[primary], 16), int(p[version], 16), p["size"][1]) for p in document["functions"]]}
