import json
import tempfile
import unittest
from pathlib import Path

import support  # noqa: F401  (puts tools/ on sys.path)

import names
import revmap


def catalogue(version, functions, data):
    return {"version": version, "functions": [{"address": a, "size": n} for a, n in functions],
            "data": [{"address": a, "size": n} for a, n in data], "jump_tables": []}


class RevmapTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp())
        for version, functions, data in (
                ("rev02", [(0x200, 0x40), (0x240, 0x20)], [(0x260, 0x100)]),
                ("rev00", [(0x200, 0x30), (0x230, 0x20)], [(0x250, 0x110)])):
            folder = self.root / "config" / version
            folder.mkdir(parents=True)
            (folder / "catalogue.json").write_text(json.dumps(catalogue(version, functions, data)))
        (self.root / "config" / "rev02" / "names.json").write_text(json.dumps({
            "schema_version": 1, "version": "rev02",
            "symbols": [{"name": "Main", "address": "0x000240", "kind": "function"},
                        {"name": "Table", "address": "0x000280", "kind": "rom_data", "size": 16}]}))
        (self.root / "config" / "rev00" / "names.json").write_text(json.dumps({
            "schema_version": 1, "version": "rev00",
            "symbols": [{"name": "Table", "address": "0x000274", "kind": "rom_data", "size": 16}]}))
        (self.root / "config" / "rev00" / "correspondence.json").write_text(json.dumps({
            "schema_version": 1, "primary": "rev02", "version": "rev00",
            "functions": [{"rev02": "0x000200", "rev00": "0x000200", "size": [0x40, 0x30], "same": False},
                          {"rev02": "0x000240", "rev00": "0x000230", "size": [0x20, 0x20], "same": True}]}))

    def test_other_revision_takes_the_primary_identities(self):
        loaded = names.load("rev00", self.root)
        self.assertEqual(loaded[0x200].name, "func_000200")
        self.assertEqual(loaded[0x230].name, "Main")

    def test_addresses_keep_their_offset_only_where_the_layout_is_certain(self):
        translate = revmap.translator("rev00", self.root)
        self.assertEqual(translate(0x248), 0x238)          # same-size corresponding functions
        self.assertIsNone(translate(0x234))                # the first function shrank: its interior is uncertain
        self.assertEqual(translate(0x280), 0x274)          # a name both revisions carry
        self.assertEqual(translate(0x284), 0x278)          # inside that name's equal extent
        self.assertIsNone(translate(0x2f0))                # elsewhere the data range changed size
        self.assertEqual(translate(0xFF8000), 0xFF8000)    # RAM does not move

    def test_primary_revision_is_unchanged(self):
        self.assertEqual(revmap.translator("rev02", self.root)(0x248), 0x248)


if __name__ == "__main__":
    unittest.main()
