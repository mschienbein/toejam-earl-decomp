import unittest

from support import synthetic_rom

import analyze


class AnalyzeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rom = synthetic_rom()
        cls.catalogue = analyze.build_catalogue(cls.rom, "synthetic", {})

    def test_vector_and_call_targets_become_functions(self):
        functions = {f["address"]: f for f in self.catalogue["functions"]}
        self.assertEqual(functions[0x200]["reached_by"], "vector")
        self.assertEqual(functions[0x300]["reached_by"], "call")
        self.assertEqual(functions[0x200]["size"], 0xC)

    def test_switch_table_is_data_inside_the_function(self):
        self.assertEqual(self.catalogue["jump_tables"],
                         [{"address": 0x310, "count": 3, "jump": 0x30C, "index_base": 0x310}])
        for case in (0x316, 0x31A, 0x31E, 0x322):
            self.assertIn(case, self.catalogue["labels"])
        # the function runs through its table to the last case: one contiguous unit
        self.assertIn({"address": 0x300, "size": 0x26}, self.catalogue["code_ranges"])

    def test_unreached_bytes_stay_data(self):
        data = {(d["address"], d["size"]) for d in self.catalogue["data"]}
        self.assertIn((0x20C, 0x300 - 0x20C), data)
        self.assertIn((0x326, len(self.rom) - 0x326), data)
        self.assertEqual(self.catalogue["stats"]["conflicts"], 0)

    def test_seed_adds_code_only_reachable_through_pointers(self):
        rom = bytearray(self.rom)
        rom[0x400:0x404] = bytes.fromhex("70074e75")  # moveq #7,d0 / rts, referenced by nothing
        seeded = analyze.build_catalogue(bytes(rom), "synthetic", {0x400: "seed"})
        entry = next(f for f in seeded["functions"] if f["address"] == 0x400)
        self.assertEqual((entry["size"], entry["reached_by"]), (4, "seed"))

    def test_one_based_switch_indexes_from_two_bytes_before_the_table(self):
        rom = bytearray(self.rom)
        rom[0x300:0x310] = bytes.fromhex("0c400003" "6e1c" "d040" "303b0004" "4efb0002")  # cases 1..3
        catalogue = analyze.build_catalogue(bytes(rom), "synthetic", {})
        self.assertEqual(catalogue["jump_tables"],
                         [{"address": 0x310, "count": 3, "jump": 0x30C, "index_base": 0x30E}])

    def test_table_overlapping_its_own_jump_is_rejected(self):
        rom = bytearray(self.rom)
        rom[0x308:0x310] = bytes.fromhex("303b0002" "4efb0000")  # both index from 0x30e: inside the jmp
        catalogue = analyze.build_catalogue(bytes(rom), "synthetic", {})
        self.assertEqual(catalogue["jump_tables"], [])
        self.assertEqual(catalogue["diagnostics"]["computed_jumps"][0]["address"], 0x30C)


if __name__ == "__main__":
    unittest.main()
