import io
import tempfile
import unittest
import zipfile
from pathlib import Path

from support import synthetic_rom

import rom as romtool


class HeaderTest(unittest.TestCase):
    def test_checksum_matches_header_and_detects_a_flipped_byte(self):
        rom = synthetic_rom()
        header = romtool.parse_header(rom)
        self.assertEqual(header["reset_pc"], 0x200)
        self.assertEqual(romtool.compute_checksum(rom), header["checksum"])
        damaged = bytearray(rom)
        damaged[0x400] ^= 0x01
        self.assertNotEqual(romtool.compute_checksum(bytes(damaged)), header["checksum"])

    def test_checksum_ignores_the_vector_table_and_header(self):
        rom = bytearray(synthetic_rom())
        before = romtool.compute_checksum(bytes(rom))
        rom[0x120] ^= 0xFF  # inside the header (0x100-0x1FF): not summed
        self.assertEqual(romtool.compute_checksum(bytes(rom)), before)


class DumpTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_zip_must_hold_exactly_one_file(self):
        path = self.dir / "two.zip"
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr("a.md", synthetic_rom())
            archive.writestr("b.md", synthetic_rom())
        with self.assertRaisesRegex(romtool.RomError, "exactly one"):
            romtool.read_dump(path)

    def test_zip_with_one_dump_reads_its_bytes(self):
        path = self.dir / "one.zip"
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr("game.md", synthetic_rom())
        self.assertEqual(romtool.read_dump(path), synthetic_rom())

    def test_smd_interleaved_dump_is_refused(self):
        path = self.dir / "game.smd"
        path.write_bytes(b"\0" * 0x200 + bytes(0x4000))
        with self.assertRaisesRegex(romtool.RomError, "interleaved"):
            romtool.read_dump(path)

    def test_unknown_dump_is_not_identified(self):
        with self.assertRaisesRegex(romtool.RomError, "unknown dump"):
            romtool.identify(synthetic_rom())


if __name__ == "__main__":
    unittest.main()
