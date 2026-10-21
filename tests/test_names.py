import unittest

import support  # noqa: F401  (puts tools/ on sys.path)

import names


def doc(*symbols):
    return {"schema_version": 1, "version": "rev02", "symbols": list(symbols)}


def sym(name, address, kind="function", **extra):
    return {"name": name, "address": address, "kind": kind, "source": "test", **extra}


class NamesTest(unittest.TestCase):
    def test_valid_names_index_by_address(self):
        parsed = names.parse(doc(sym("GameProgram", "0x0002fc"), sym("PlayerState", "0xff8000", "ram", size=64)), "rev02")
        self.assertEqual(parsed[0x2FC].name, "GameProgram")
        self.assertEqual((parsed[0xFF8000].kind, parsed[0xFF8000].size), ("ram", 64))

    def test_generated_prefixes_are_reserved(self):
        for bad in ("func_0002fc", "loc_001234", "dat_00abcd", "jtbl_009e9c"):
            with self.assertRaisesRegex(names.NamesError, "reserved"):
                names.parse(doc(sym(bad, "0x0002fc")))

    def test_one_name_per_address_and_one_address_per_name(self):
        with self.assertRaisesRegex(names.NamesError, "two names"):
            names.parse(doc(sym("A", "0x000300"), sym("B", "0x000300")))
        with self.assertRaisesRegex(names.NamesError, "names two addresses"):
            names.parse(doc(sym("A", "0x000300"), sym("A", "0x000302")))
        with self.assertRaisesRegex(names.NamesError, "names two addresses"):
            names.parse(doc(sym("A", "0x000300", aliases=["B"]), sym("B", "0x000302")))

    def test_ranges_follow_the_kind(self):
        with self.assertRaisesRegex(names.NamesError, "RAM name outside"):
            names.parse(doc(sym("Counter", "0x000300", "ram")))
        with self.assertRaisesRegex(names.NamesError, "ROM name outside"):
            names.parse(doc(sym("Table", "0xff8000", "rom_data")))

    def test_file_for_another_version_is_refused(self):
        with self.assertRaisesRegex(names.NamesError, "not 'rev00'"):
            names.parse(doc(sym("A", "0x000300")), "rev00")


if __name__ == "__main__":
    unittest.main()
