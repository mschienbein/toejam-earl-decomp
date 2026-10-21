"""Paths and identity of the pinned toolchain (see toolchain.lock.json and tools/setup.py)."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "toolchain.lock.json").read_text())
INSTALL = ROOT / LOCK["install_directory"]
INSTALLED = INSTALL / "installed.json"


class ToolchainError(RuntimeError):
    pass


def binutils_prefix(target: str) -> Path:
    return INSTALL / f"binutils-{LOCK['binutils']['version']}-{target}"


def tool(target: str, name: str) -> Path:
    """Absolute path of a pinned binutils program, e.g. tool("m68k-elf", "as")."""
    path = binutils_prefix(target) / "bin" / f"{target}-{name}"
    if not path.is_file():
        raise ToolchainError(f"{path} is missing; run tools/setup.py")
    return path


def file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1 << 22):
            h.update(chunk)
    return h.hexdigest()


def identity() -> dict:
    """The installed-toolchain record written by tools/setup.py (binary hashes, versions)."""
    if not INSTALLED.is_file():
        raise ToolchainError(f"{INSTALLED} is missing; run tools/setup.py")
    return json.loads(INSTALLED.read_text())
