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


def emu2_path() -> Path:
    """The pinned emu2 DOS emulator binary (built by tools/setup.py)."""
    return INSTALL / f"emu2-{LOCK['emu2']['commit'][:12]}" / "emu2"


def mcc68k_dir() -> Path:
    """The extracted Microtec tools: MCC68K/ (compiler, headers, libraries) and ASM68K/."""
    return INSTALL / f"mcc68k-{LOCK['mcc68k']['version']}"


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


def verified_compiler() -> dict:
    """Hash the compiler programs that are about to run and check them before use.

    MCC68K.EXE must equal its immutable pin in toolchain.lock.json; emu2 must be the binary that
    tools/setup.py built from the pinned commit (its installation record). Returns the hashes
    actually checked, which build receipts record.
    """
    compiler = mcc68k_dir() / "MCC68K" / "MCC68K.EXE"
    emu2 = emu2_path()
    for path in (emu2, compiler):
        if not path.is_file():
            raise ToolchainError(f"{path} is missing; run tools/setup.py")
    actual = {"mcc68k": file_sha256(compiler), "emu2": file_sha256(emu2)}
    if actual["mcc68k"] != LOCK["mcc68k"]["sha256"]["MCC68K/MCC68K.EXE"]:
        raise ToolchainError(f"{compiler} does not match its pinned sha256; run tools/setup.py")
    recorded = identity().get("emu2") or {}
    if recorded.get("commit") != LOCK["emu2"]["commit"] or recorded.get("sha256") != actual["emu2"]:
        raise ToolchainError(f"{emu2} is not the binary tools/setup.py built from the pinned commit; run tools/setup.py")
    return actual


def verified_binutils(target: str, names: tuple[str, ...] = ("as", "ld", "objcopy")) -> dict:
    """Hash the binutils programs that are about to run and check them against the installation record."""
    recorded = identity()["binutils"]["targets"][target]
    actual = {}
    for name in names:
        actual[name] = file_sha256(tool(target, name))
        if actual[name] != recorded[name]["sha256"]:
            raise ToolchainError(f"{target}-{name} differs from the installed toolchain record; run tools/setup.py")
    return actual
