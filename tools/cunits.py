"""C translation units in the build: config/sources.json, compiled with MCC68K, placed at their original address.

    {"schema_version": 1,
     "units": [{"source": "src/game/timer.c", "versions": ["rev02"], "options": []}]}

Each unit is one C file. Its functions must carry the canonical name of their address (an imported
name, else func_XXXXXX) and appear in ROM order, so the unit replaces one contiguous run of
catalogued units (functions, and any literal data between them) exactly. Code only: a unit that
emits initialised data is refused until data placement is designed.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

from elftools.elf.elffile import ELFFile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mcc  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "config" / "sources.json"


class CUnitError(RuntimeError):
    pass


@dataclass
class CUnit:
    name: str                                   # section/object name, c_<stem>
    source: Path
    obj: Path
    size: int                                   # bytes of code
    functions: list[tuple[str, int, int]]       # (name, offset, size) in emission order
    defined: set[str] = field(default_factory=set)
    undefined: set[str] = field(default_factory=set)
    source_sha256: str = ""
    options: list[str] = field(default_factory=list)
    address: int | None = None


def load_sources(version: str, path: Path = SOURCES) -> list[dict]:
    if not path.is_file():
        return []
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema_version") != 1:
        raise CUnitError("sources.json: schema_version must be 1")
    return [u for u in document["units"] if version in u.get("versions", [])]


def unit_name(source: Path) -> str:
    return "c_" + re.sub(r"[^A-Za-z0-9_]", "_", source.stem).lower()


def compile_unit(entry: dict, out: Path, root: Path = ROOT, address_of=None, spans: list[tuple[int, int]] = (),
                 version: str = "rev02") -> CUnit:
    """Compile one C file with MCC68K and assemble it; out/asm and out/obj receive the outputs.

    `address_of(name)` and the original translation-unit `spans` let calls into the rest of the
    original unit be assembled PC-relative, as the original assembler did (tools/mcc.py `near`).
    """
    source = root / entry["source"]
    if not source.is_file():
        raise CUnitError(f"{entry['source']} does not exist")
    name = unit_name(source)
    options = list(entry.get("options", []))
    try:
        compiled = mcc.compile_to_gas(source, name, [root / "include"], options, address_of, spans, version)
        (out / "asm" / f"{name}.SRC").write_text(compiled.listing, encoding="latin-1")
        (out / "asm" / f"{name}.s").write_text(compiled.gas)
        obj = out / "obj" / f"{name}.o"
        mcc.assemble(compiled.gas, source, obj, name)
    except (mcc.CompileError, mcc.AdaptError) as error:
        raise CUnitError(f"{entry['source']}: {error}") from None
    with obj.open("rb") as stream:
        elf = ELFFile(stream)
        section = elf.get_section_by_name(f".text.{name}")
        if section is None:
            raise CUnitError(f"{entry['source']}: no code")
        for other in elf.iter_sections():
            if other.name in (".data", ".bss") and other["sh_size"]:
                raise CUnitError(f"{entry['source']}: emits {other['sh_size']} bytes of {other.name}; data placement is not designed yet")
        index = elf.get_section_index(f".text.{name}")
        symbols = list(elf.get_section_by_name(".symtab").iter_symbols())
        starts = sorted((s["st_value"], s.name) for s in symbols
                        if s["st_shndx"] == index and s["st_info"]["bind"] == "STB_GLOBAL")
        size = section["sh_size"]
        undefined = {s.name for s in symbols if s.name and s["st_shndx"] == "SHN_UNDEF"}
    if not starts or starts[0][0] != 0:
        raise CUnitError(f"{entry['source']}: code must start with a global function")
    functions = [(n, off, (starts[k + 1][0] if k + 1 < len(starts) else size) - off) for k, (off, n) in enumerate(starts)]
    return CUnit(name, source, obj, size, functions, {n for _, n in starts}, undefined,
                 hashlib.sha256(source.read_bytes()).hexdigest(), options)
