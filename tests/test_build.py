import json
import struct
import tempfile
import unittest
from pathlib import Path

from elftools.elf.elffile import ELFFile
from support import ROOT, synthetic_rom

import analyze
import build
import names as namestool
import rom as romtool
import toolchain


def have_toolchain() -> bool:
    try:
        toolchain.tool(build.TARGET, "as")
        return True
    except toolchain.ToolchainError:
        return False


# Encodings whose GNU as spelling is easy to get wrong (signedness, implied sizes, special
# registers, PC-relative and absolute-short operands, a short branch into another unit far from
# its own section start).
TRICKY = [
    "70ff", "4e56fff8", "4e5e", "cf88", "4e66", "4e6e", "0111", "08010003", "46fc2700", "027cf8ff",
    "003c0001", "48e73f3e", "4cdf7cfc", "31c0fff0", "223900ff0000", "14301004", "5240", "518f",
    "4218", "4aa1", "81c1", "c0fc000a", "e440", "48c0", "4840", "4440", "4600", "50c0", "b308",
    "d380", "c300", "4e40", "51c9fffe",
]


def tricky_rom() -> bytes:
    rom = bytearray(synthetic_rom())
    code = bytearray(bytes.fromhex("4eb900000740"))      # jsr $740.l makes 0x740 a function
    for word in TRICKY:
        code += bytes.fromhex(word)
    here = 0x600 + len(code)
    code += struct.pack(">HH", 0x41FA, 0x734 - (here + 2))  # lea $734(pc),a0 (data)
    code += struct.pack(">HH", 0x4EBA, 0x740 - (here + 6))  # jsr $740(pc)
    while 0x600 + len(code) < 0x730:
        code += bytes.fromhex("4e71")
    code += bytes.fromhex("660e")                       # 0x730: bne.s $740, 0x130 bytes into its unit
    code += bytes.fromhex("4e75")                       # 0x732: rts
    rom[0x600:0x600 + len(code)] = code
    rom[0x734:0x740] = b"TABLEDATA..."
    rom[0x740:0x744] = bytes.fromhex("70ff4e75")        # moveq #-1,d0 / rts
    struct.pack_into(">I", rom, 4, 0x600)                # reset vector -> the tricky function
    return bytes(rom)


@unittest.skipUnless(have_toolchain(), "pinned binutils not installed (python tools/setup.py)")
class SyntheticBuildTest(unittest.TestCase):
    def build(self, rom: bytes, names: dict | None = None) -> tuple[dict, Path]:
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        base = Path(tmp.name) / "baserom.bin"
        base.write_bytes(rom)
        catalogue = analyze.build_catalogue(rom, "synthetic", {})
        return build.build_image(rom, base, catalogue, Path(tmp.name) / "out", jobs=2, names=names), Path(tmp.name) / "out"

    def test_imported_names_resolve_to_their_addresses(self):
        names = namestool.parse({"schema_version": 1, "version": "synthetic", "symbols": [
            {"name": "Callee", "address": "0x000740", "kind": "function"},
            {"name": "TableData", "address": "0x000734", "kind": "rom_data"},
            {"name": "RamBase", "address": "0xff0000", "kind": "ram"},
        ]})
        receipt, out = self.build(tricky_rom(), names)
        self.assertEqual(receipt["result"], "PASS")
        self.assertEqual(receipt["instructions"]["raw_words"], 0)
        with (out / "toejam.elf").open("rb") as stream:
            table = {s.name: s["st_value"] for s in ELFFile(stream).get_section_by_name(".symtab").iter_symbols()}
        self.assertEqual((table["Callee"], table["TableData"], table["RamBase"]), (0x740, 0x734, 0xFF0000))

    def test_switch_function_rebuilds_byte_identically_as_assembly(self):
        receipt, _ = self.build(synthetic_rom())
        self.assertEqual(receipt["result"], "PASS")
        self.assertEqual(receipt["instructions"]["raw_words"], 0)
        self.assertEqual((receipt["instructions"]["jump_tables"], receipt["instructions"]["raw_jump_tables"]), (1, 0))

    def test_tricky_encodings_round_trip_without_raw_fallback(self):
        receipt, _ = self.build(tricky_rom())
        self.assertEqual(receipt["result"], "PASS")
        self.assertEqual(receipt["instructions"]["raw_by_mnemonic"], {})

    def test_mismatch_falls_back_to_raw_words_and_still_matches(self):
        rom = tricky_rom()
        catalogue = analyze.build_catalogue(rom, "synthetic", {})
        original = build.m68k.render
        build.m68k.render = lambda insn, symbol: "nop" if insn.base == "exg" else original(insn, symbol)
        try:
            tmp = tempfile.TemporaryDirectory()
            self.addCleanup(tmp.cleanup)
            base = Path(tmp.name) / "baserom.bin"
            base.write_bytes(rom)
            receipt = build.build_image(rom, base, catalogue, Path(tmp.name) / "out", jobs=2)
        finally:
            build.m68k.render = original
        self.assertEqual(receipt["result"], "PASS")
        self.assertEqual(receipt["instructions"]["raw_by_mnemonic"], {"exg.l": 1})


@unittest.skipUnless(have_toolchain(), "pinned binutils not installed (python tools/setup.py)")
class RealRomTest(unittest.TestCase):
    """End to end against the private dumps; skipped where orig/ is absent (CI)."""

    def check(self, version: str) -> None:
        try:
            rom = romtool.load(version)
        except romtool.RomError as error:
            self.skipTest(str(error))
        committed = (ROOT / "config" / version / "catalogue.json").read_text()
        fresh = analyze.render_json(analyze.build_catalogue(rom, version, analyze.load_seeds(version)))
        self.assertEqual(committed, fresh, "committed catalogue is stale: run tools/analyze.py")
        receipt = build.build(version, jobs=8)
        self.assertEqual(receipt["result"], "PASS")
        self.assertEqual(receipt["actual"]["sha256"], romtool.title()["versions"][version]["sha256"])

    def test_rev02(self):
        self.check("rev02")

    def test_rev00(self):
        self.check("rev00")


if __name__ == "__main__":
    unittest.main()
