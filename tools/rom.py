"""Import, verify and describe the private ROM dumps pinned in config/title.json.

    python tools/rom.py import --rom "ToeJam & Earl (World) (Rev A).zip"   # -> orig/rev02/baserom.bin
    python tools/rom.py verify [--version rev02]
    python tools/rom.py info   [--version rev02]

Accepts a plain dump (.md/.bin/.gen) or a .zip holding exactly one. The version is identified by the
SHA-256 pinned in config/title.json; anything else is refused (no guessing, no repair). Imported
files are read-only (0444) under git-ignored orig/. Nothing here prints ROM bytes beyond the textual
header fields.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import struct
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TITLE = ROOT / "config/title.json"
HEADER = 0x100


class RomError(RuntimeError):
    pass


def title() -> dict:
    return json.loads(TITLE.read_text())


def parse_header(rom: bytes) -> dict:
    """The Genesis cartridge header at 0x100 plus the reset vectors."""
    if len(rom) < 0x200:
        raise RomError("image shorter than the 512-byte vector table and header")
    h = rom[HEADER:HEADER + 0x100]
    text = lambda a, b: h[a:b].decode("ascii", "replace").rstrip()
    return {
        "console": text(0x00, 0x10),
        "copyright": text(0x10, 0x20),
        "domestic_name": " ".join(text(0x20, 0x50).split()),
        "overseas_name": " ".join(text(0x50, 0x80).split()),
        "serial": text(0x80, 0x8E),
        "checksum": struct.unpack(">H", h[0x8E:0x90])[0],
        "io_support": text(0x90, 0xA0),
        "rom_start": struct.unpack(">I", h[0xA0:0xA4])[0],
        "rom_end": struct.unpack(">I", h[0xA4:0xA8])[0],
        "ram_start": struct.unpack(">I", h[0xA8:0xAC])[0],
        "ram_end": struct.unpack(">I", h[0xAC:0xB0])[0],
        "region": text(0xF0, 0xF3),
        "initial_sp": struct.unpack(">I", rom[0:4])[0],
        "reset_pc": struct.unpack(">I", rom[4:8])[0],
    }


def compute_checksum(rom: bytes) -> int:
    """Sum of big-endian 16-bit words from 0x200 to the end, modulo 0x10000 (the BIOS/TMSS check)."""
    body = rom[0x200:]
    if len(body) % 2:
        body += b"\0"
    return sum(struct.unpack(f">{len(body) // 2}H", body)) & 0xFFFF


def looks_interleaved(raw: bytes) -> bool:
    """SMD dumps carry a 512-byte copier header and 16 KiB interleaved blocks."""
    return len(raw) % 0x4000 == 0x200 and raw[0x200 + HEADER:0x200 + HEADER + 4] != b"SEGA"


def read_dump(path: Path) -> bytes:
    if path.suffix.lower() == ".zip":
        with zipfile.ZipFile(path) as archive:
            members = [m for m in archive.infolist() if not m.is_dir()]
            if len(members) != 1:
                raise RomError(f"{path.name}: expected exactly one file in the archive, found {len(members)}")
            raw = archive.read(members[0])
    else:
        raw = path.read_bytes()
    if looks_interleaved(raw):
        raise RomError("SMD-interleaved dump; supply a plain (.md/.bin) dump")
    if raw[HEADER:HEADER + 4] != b"SEGA":
        raise RomError("no SEGA header at 0x100; not a plain Genesis dump")
    return raw


def identify(raw: bytes) -> str:
    digest = hashlib.sha256(raw).hexdigest()
    for name, version in title()["versions"].items():
        if version["sha256"] == digest:
            return name
    raise RomError(f"unknown dump (sha256 {digest}); supported versions are pinned in config/title.json")


def orig_path(version: str) -> Path:
    versions = title()["versions"]
    if version not in versions:
        raise RomError(f"unknown version {version!r}; choose from {sorted(versions)}")
    return ROOT / versions[version]["orig"]


def load(version: str | None = None) -> bytes:
    """The verified original ROM for `version` (default: the title's default version)."""
    version = version or title()["default_version"]
    path = orig_path(version)
    if not path.is_file():
        raise RomError(f"{path.relative_to(ROOT)} is missing; run tools/rom.py import")
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != title()["versions"][version]["sha256"]:
        raise RomError(f"{path.relative_to(ROOT)} does not match its pinned sha256")
    return raw


def do_import(source: Path) -> str:
    raw = read_dump(source)
    version = identify(raw)
    spec = title()["versions"][version]
    header = parse_header(raw)
    if header["serial"] != spec["serial"] or f"{header['checksum']:04x}" != spec["header_checksum"]:
        raise RomError("header fields disagree with the pinned identity")
    if compute_checksum(raw) != header["checksum"]:
        raise RomError("header checksum does not match the computed checksum")
    dest = orig_path(version)
    if dest.is_file():
        if dest.read_bytes() != raw:
            raise RomError(f"{dest.relative_to(ROOT)} exists with different bytes")
        return version
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(".part")
    tmp.write_bytes(raw)
    os.chmod(tmp, 0o444)
    tmp.replace(dest)
    return version


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    imp = sub.add_parser("import")
    imp.add_argument("--rom", type=Path, required=True)
    for name in ("verify", "info"):
        p = sub.add_parser(name)
        p.add_argument("--version")
    args = parser.parse_args()
    try:
        if args.command == "import":
            version = do_import(args.rom)
            print(f"imported {version} -> {orig_path(version).relative_to(ROOT)}")
        elif args.command == "verify":
            version = args.version or title()["default_version"]
            load(version)
            print(f"{version}: OK")
        else:
            raw = load(args.version)
            info = parse_header(raw)
            info["computed_checksum"] = compute_checksum(raw)
            print(json.dumps({k: (f"0x{v:08x}" if isinstance(v, int) else v) for k, v in info.items()}, indent=1))
    except RomError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
