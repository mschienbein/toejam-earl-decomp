import contextlib
import io
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from elftools.elf.elffile import ELFFile

from toolsmith_support import ROOT, elf_object, load_diff, packet, program_for, synthetic_repo, synthetic_rom

author_diff = load_diff()


class DiffTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def candidate(self, raw, relocs=(), **kwargs):
        path = self.root / "candidate.o"
        path.write_bytes(elf_object(raw, relocs, **kwargs))
        return author_diff.read_candidate(path, kwargs.get("name", "candidate"))

    def compare(self, program, candidate):
        return author_diff.compare(program, program.select("func_000200"), candidate)

    def test_exact_match_and_instruction_difference(self):
        program = program_for(bytes.fromhex("7001" "4e75"))
        result = self.compare(program, self.candidate(bytes.fromhex("7001" "4e75")))
        self.assertTrue(result["match"])
        result = self.compare(program, self.candidate(bytes.fromhex("7002" "4e75")))
        self.assertFalse(result["match"])
        display = author_diff.side_by_side(program, program.select("func_000200"), result)
        self.assertIn("moveq", display)
        self.assertIn("DIFFER", display)
        self.assertIn("!", display)

    def test_long_absolute_relocation_matches_by_symbol_not_zero_fill(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}])
        candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0)])
        self.assertNotEqual(candidate.data, program.rom[0x200:0x206])
        result = self.compare(program, candidate)
        self.assertTrue(result["match"])
        self.assertEqual(result["relocations"][0]["actual"], {"symbol": "func_000300", "addend": 0})

    def test_wrong_target_and_wrong_addend_are_not_masked(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}, {"address": 0x320}])
        for target, addend in (("func_000320", 0), ("func_000300", 2), ("unknown_function", 0)):
            with self.subTest(target=target, addend=addend):
                result = self.compare(program, self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, target, addend)]))
                self.assertFalse(result["match"])

    def test_pc8_branch_addend_bias_is_normalized(self):
        program = program_for(bytes.fromhex("613e"), [{"address": 0x240}])
        candidate = self.candidate(bytes.fromhex("6100"), [(1, 6, "func_000240", -1)])
        result = self.compare(program, candidate)
        self.assertTrue(result["match"])
        self.assertEqual(result["relocations"][0]["actual"], {"symbol": "func_000240", "addend": 0})
        self.assertFalse(self.compare(program, self.candidate(bytes.fromhex("6100"), [(1, 6, "func_000240", 0)]))["match"])

    def test_pc16_branch_and_pc32_data_operand(self):
        program = program_for(bytes.fromhex("610000fe"), [{"address": 0x300}])
        self.assertTrue(self.compare(program, self.candidate(bytes.fromhex("61000000"), [(2, 5, "func_000300", 0)]))["match"])
        # R_68K_PC32 is legal ELF data even though the 68000 has no 32-bit branch operand.
        program = program_for(bytes.fromhex("203c000001fe" "4e75"))
        candidate = self.candidate(bytes.fromhex("203c00000000" "4e75"), [(2, 4, "data_000400", 0)])
        # PC32 uses instruction+2: 0x1fe+0x202 = data_000400, with no compensating addend.
        self.assertTrue(self.compare(program, candidate)["match"])

    def test_absolute_short_sign_extension_uses_ram_address(self):
        program = program_for(bytes.fromhex("3038fff0" "4e75"), names={"symbols": [
            {"name": "LastWord", "address": 0xFFFFF0, "kind": "ram", "size": 2}]})
        candidate = self.candidate(bytes.fromhex("30380000" "4e75"), [(2, 2, "LastWord", 0)])
        self.assertTrue(self.compare(program, candidate)["match"])

    def test_absolute_eight_bit_immediate_relocation(self):
        program = program_for(bytes.fromhex("707f" "4e75"), names={"symbols": [{"name": "Limit", "address": 0x7F, "kind": "constant"}]})
        self.assertTrue(self.compare(program, self.candidate(bytes.fromhex("7000" "4e75"), [(1, 3, "Limit", 0)]))["match"])

    def test_rel_implicit_addend_and_named_alias(self):
        program = program_for(bytes.fromhex("4eb900000304"), [{"address": 0x300, "raw": bytes.fromhex("4e71" "4e71" "4e75")}],
                              names={"symbols": [{"name": "Worker", "address": 0x300, "kind": "function", "size": 6}]})
        candidate = self.candidate(bytes.fromhex("4eb900000004"), [(2, 1, "Worker", 4)], rela=False)
        result = self.compare(program, candidate)
        self.assertTrue(result["match"])
        self.assertEqual(result["relocations"][0]["actual"], {"symbol": "Worker", "addend": 4})

    def test_local_section_anchor_maps_current_candidate_name(self):
        # A relocation to the current defined function's local offset needs no names.json entry.
        program = program_for(bytes.fromhex("41f900000208" "4e71" "4e75"))
        candidate = self.candidate(bytes.fromhex("41f900000000" "4e71" "4e75"), [(2, 1, "candidate", 8)])
        self.assertTrue(self.compare(program, candidate)["match"])

    def test_compensating_different_function_or_data_symbol_is_not_an_alias(self):
        program = program_for(bytes.fromhex("4eb900000320"), [{"address": 0x300}, {"address": 0x320}])
        result = self.compare(program, self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0x20)]))
        self.assertFalse(result["match"])
        self.assertEqual(result["relocations"][0]["actual"], {"symbol": "func_000300", "addend": 0x20})
        program = program_for(bytes.fromhex("41f900000400"), names={"symbols": [
            {"name": "FirstData", "address": 0x400, "kind": "data", "size": 16},
            {"name": "LaterData", "address": 0x420, "kind": "data", "size": 16}]})
        result = self.compare(program, self.candidate(bytes.fromhex("41f900000000"), [(2, 1, "LaterData", -0x20)]))
        self.assertFalse(result["match"])

    def test_genuine_alias_metadata_and_section_offsets_normalize(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}], names={"symbols": [
            {"name": "Worker", "address": 0x300, "kind": "function", "aliases": [{"name": "WorkerAlias", "source": "synthetic"}]}]})
        result = self.compare(program, self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "WorkerAlias", 0)]))
        self.assertTrue(result["match"], result)
        program = program_for(bytes.fromhex("41f900000208" "4e71" "4e75"))
        result = self.compare(program, self.candidate(bytes.fromhex("41f900000000" "4e71" "4e75"), [(2, 1, ".text", 10)],
                                                    prefix=bytes.fromhex("4e71"), extra_symbols=[(".text", 0, 0, 1, 3)]))
        self.assertTrue(result["match"], result)

    def test_section_type_cannot_hide_a_compensating_absolute_symbol(self):
        program = program_for(bytes.fromhex("4eb900000320"), [{"address": 0x300}, {"address": 0x320}])
        candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "forged_section", 0x20)],
                                   extra_symbols=[("forged_section", 0x300, 0, 0xFFF1, 3)])
        result = self.compare(program, candidate)
        self.assertFalse(result["match"])
        self.assertIn("genuine zero-offset", " ".join(result["errors"]))

    def test_known_absolute_and_section_definitions_cannot_contradict_metadata(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}])
        for value, section in ((0x320, 0xFFF1), (0, 1), (0, 2)):
            with self.subTest(value=value, section=section):
                candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0)],
                                           extra_symbols=[("func_000300", value, 0, section, 0x10)])
                result = self.compare(program, candidate)
                self.assertFalse(result["match"])
                self.assertTrue(result["errors"])
        candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0)],
                                   extra_symbols=[("func_000300", 0x300, 0, 0xFFF1, 0x10)])
        self.assertTrue(self.compare(program, candidate)["match"])

    def test_unused_contradictory_known_definition_is_also_rejected(self):
        program = program_for(bytes.fromhex("4e75"), [{"address": 0x300}])
        candidate = self.candidate(bytes.fromhex("4e75"), extra_symbols=[("func_000300", 0x320, 0, 0xFFF1, 0x10)])
        self.assertFalse(self.compare(program, candidate)["match"])

    def test_missing_size_uses_next_function_and_not_local_label(self):
        raw = bytes.fromhex("7001" "4e75" "7002" "4e75")
        candidate = self.candidate(raw, size=0, extra_symbols=[("inside", 2, 0, 1, 0), ("next_function", 4, 4, 1, 0x12)])
        self.assertEqual(candidate.data, raw[:4])
        candidate = self.candidate(raw[:4], size=0, extra_symbols=[("inside", 2, 0, 1, 0)])
        self.assertEqual(candidate.data, raw[:4])

    def test_nonzero_symbol_section_offset(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}])
        candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0)], prefix=bytes.fromhex("4e71"))
        self.assertEqual(candidate.offset, 2)
        self.assertTrue(self.compare(program, candidate)["match"])

    def test_unknown_relocation_wrong_machine_and_missing_symbol_fail(self):
        with self.assertRaisesRegex(author_diff.DiffError, "unsupported m68k relocation"):
            self.candidate(bytes.fromhex("4eb900000000"), [(2, 7, "func_000300", 0)])
        with self.assertRaisesRegex(author_diff.DiffError, "big-endian ELF32 m68k"):
            self.candidate(bytes.fromhex("4e75"), machine=3)
        path = self.root / "candidate.o"
        path.write_bytes(elf_object(bytes.fromhex("4e75")))
        with self.assertRaisesRegex(author_diff.DiffError, "--symbol"):
            author_diff.read_candidate(path, "missing")

    def test_overlapping_relocations_fail_closed(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}])
        candidate = self.candidate(bytes.fromhex("4eb900000000"), [(2, 1, "func_000300", 0), (2, 1, "func_000300", 0)])
        with self.assertRaisesRegex(author_diff.DiffError, "overlapping"):
            self.compare(program, candidate)

    def test_size_and_nonrelocated_opcode_must_match(self):
        program = program_for(bytes.fromhex("4eb900000300"), [{"address": 0x300}])
        self.assertFalse(self.compare(program, self.candidate(bytes.fromhex("4eb900000000" "4e75"), [(2, 1, "func_000300", 0)]))["match"])
        self.assertFalse(self.compare(program, self.candidate(bytes.fromhex("4ef900000000"), [(2, 1, "func_000300", 0)]))["match"])

    def test_cli_exact_and_mismatch_exit_status(self):
        synthetic_repo(self.root)
        # Use the complete switch function to test table-aware presentation as well.
        code = synthetic_rom()[0x300:0x326]
        path = self.root / "candidate.o"
        path.write_bytes(elf_object(code))
        with contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(author_diff.main(["func_000300", str(path), "--symbol", "candidate", "--repo", str(self.root)]), 0)
        self.assertIn(".short", out.getvalue())
        path.write_bytes(elf_object(code[:-2]))
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(author_diff.main(["func_000300", str(path), "--symbol", "candidate", "--repo", str(self.root)]), 1)


class PinnedBinutilsDiffTest(unittest.TestCase):
    def test_real_assembler_relocations(self):
        assembler = ROOT / ".cache/toolchain/binutils-2.47-m68k-elf/bin/m68k-elf-as"
        if not assembler.is_file():
            self.skipTest("pinned m68k binutils not installed")
        with tempfile.TemporaryDirectory() as tmp:
            source, obj = Path(tmp) / "candidate.s", Path(tmp) / "candidate.o"
            source.write_text(".text\n.globl candidate\n.type candidate,@function\ncandidate:\njsr (func_000300).l\nbsr.w func_000300\n.size candidate,.-candidate\n")
            result = subprocess.run([str(assembler), "-m68000", "--pcrel", "-o", str(obj), str(source)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            program = program_for(bytes.fromhex("4eb900000300" "610000f8"), [{"address": 0x300}])
            candidate = author_diff.read_candidate(obj, "candidate")
            result = author_diff.compare(program, program.select("func_000200"), candidate)
            self.assertTrue(result["match"], result)

    def test_defined_symbol_check_agrees_with_a_real_link(self):
        binutils = ROOT / ".cache/toolchain/binutils-2.47-m68k-elf/bin"
        if not (binutils / "m68k-elf-as").is_file():
            self.skipTest("pinned m68k binutils not installed")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, original_obj = root / "candidate.s", root / "undefined.o"
            source.write_text(".text\n.globl candidate\n.type candidate,@function\ncandidate:\njsr (func_000300).l\nrts\n.size candidate,.-candidate\n")
            subprocess.run([str(binutils / "m68k-elf-as"), "-m68000", "--pcrel", "-o", str(original_obj), str(source)], check=True, capture_output=True)
            with original_obj.open("rb") as stream:
                elf = ELFFile(stream)
                symtab = elf.get_section_by_name(".symtab")
                index = next(i for i, symbol in enumerate(symtab.iter_symbols()) if symbol.name == "func_000300")
                position = symtab["sh_offset"] + index * symtab["sh_entsize"]
            script = root / "place.ld"
            script.write_text("SECTIONS { .rom 0x200 : { *(.text) } }\n")
            program = program_for(bytes.fromhex("4eb900000300" "4e75"), [{"address": 0x300}])
            unit = program.select("func_000200")
            for address in (0x300, 0x320):
                raw = bytearray(original_obj.read_bytes())
                struct.pack_into(">I", raw, position + 4, address)
                struct.pack_into(">H", raw, position + 14, 0xFFF1)
                obj, linked, binary = root / f"defined-{address:x}.o", root / "linked.elf", root / "linked.bin"
                obj.write_bytes(raw)
                result = author_diff.compare(program, unit, author_diff.read_candidate(obj, "candidate"))
                subprocess.run([str(binutils / "m68k-elf-ld"), "-T", str(script), "-o", str(linked), str(obj)], check=True, capture_output=True)
                subprocess.run([str(binutils / "m68k-elf-objcopy"), "-O", "binary", str(linked), str(binary)], check=True, capture_output=True)
                linked_matches = binary.read_bytes() == program.rom[0x200:0x208]
                self.assertEqual(linked_matches, address == 0x300)
                self.assertEqual(result["match"], linked_matches, result)


class PrivateRomDiffTest(unittest.TestCase):
    def test_full_catalogue_extent_can_match_without_relocations(self):
        if not (ROOT / "orig/rev02/baserom.bin").is_file():
            self.skipTest("private retail ROM not installed")
        rom, catalogue, symbols = packet.load_inputs(ROOT, "rev02")
        program = packet.Program(rom, catalogue, symbols, packet.decoder(ROOT))
        # A genuine ELF container around a private function, produced only in the temporary dir.
        unit = min((u for u in program.units if u["kind"] == "function" and u["size"] > 16), key=lambda u: u["size"])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "private.o"
            path.write_bytes(elf_object(rom[unit["address"]:unit["address"] + unit["size"]]))
            candidate = author_diff.read_candidate(path, "candidate")
            self.assertTrue(author_diff.compare(program, unit, candidate)["match"])


if __name__ == "__main__":
    unittest.main()
