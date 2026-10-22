"""Optional asm-differ settings. Numeric start/end addresses avoid map parser assumptions.

Binary mode is supported. Upstream m68k object relocation handling currently rejects R_68K_PC*;
use tools/diff.py for relocation-aware object comparisons and exact-match exit status.
"""
import os
from pathlib import Path


def apply(config: dict, args) -> None:
    root = Path(os.environ.get("TJE_DIFF_ROOT", Path(__file__).resolve().parent))
    version = os.environ.get("TJE_DIFF_VERSION", "rev02")
    config["arch"] = "m68k"
    config["baseimg"] = str(root / "orig" / version / "baserom.bin")
    config["myimg"] = os.environ.get("TJE_DIFF_CANDIDATE", str(root / "build" / version / "toejam.bin"))
    config["mapfile"] = str(root / "build" / version / "toejam.map")
    config["map_format"] = "gnu"
    config["source_directories"] = [str(root / "src")]
    config["objdump_executable"] = str(root / ".cache/toolchain/binutils-2.47-m68k-elf/bin/m68k-elf-objdump")
