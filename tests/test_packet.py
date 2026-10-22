import contextlib
import hashlib
import io
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from toolsmith_support import ROOT, analyze, packet, program_for, synthetic_repo, synthetic_rom


class PacketTest(unittest.TestCase):
    def setUp(self):
        self.image = synthetic_rom()
        self.catalogue = analyze.build_catalogue(self.image, "synthetic", {})
        self.names = {"schema_version": 1, "version": "synthetic", "symbols": [
            {"name": "ChooseCase", "address": "0x300", "kind": "function", "size": 38,
             "source": "synthetic fixture", "evidence": "test switch"}]}
        self.program = packet.Program(self.image, self.catalogue, packet.Symbols(self.catalogue, self.names), packet.decoder(ROOT))

    def test_symbolized_calls_and_complete_switch_body(self):
        caller = packet.make_packet(self.program, self.program.select("func_000200"), "synthetic")
        self.assertEqual(len(caller["callees"]), 2)
        self.assertTrue(all(edge["target_name"] == "ChooseCase" for edge in caller["callees"]))
        self.assertIn("ChooseCase", caller["assembly"][0]["text"])
        target = packet.make_packet(self.program, self.program.select("func_000300"), "synthetic")
        self.assertEqual(len(target["callers"]), 2)
        self.assertEqual(len(target["jump_tables"]), 1)
        self.assertEqual(target["jump_tables"][0]["targets"], [0x316, 0x31A, 0x31E])
        self.assertEqual([row["address"] for row in target["assembly"] if row["text"].startswith(".short")], [0x310])
        self.assertFalse(any(row["address"] in (0x312, 0x314) for row in target["assembly"]))
        self.assertEqual(len([e for e in target["call_graph_edges"] if e["kind"] == "switch"]), 3)
        self.assertIn("synthetic fixture", packet.markdown(target))

    def test_cross_unit_switch_targets_are_graph_edges_not_fake_calls(self):
        image = bytearray(self.image)
        image[0x314:0x316] = (0x350 - 0x310).to_bytes(2, "big", signed=True)
        image[0x350:0x352] = bytes.fromhex("4e75")
        catalogue = {**self.catalogue, "functions": self.catalogue["functions"] + [{"address": 0x350, "size": 2, "kind": "function"}]}
        program = packet.Program(bytes(image), catalogue, packet.Symbols(catalogue), packet.decoder(ROOT))
        data = packet.make_packet(program, program.select("func_000350"), "synthetic")
        self.assertEqual(data["callers"][0]["kind"], "switch")
        self.assertEqual(data["callers"][0]["from"], 0x300)

    def test_named_ram_short_long_and_rom_data_references(self):
        code = bytes.fromhex("223900ff0010" "41f900000400" "3038fff0" "4e75")
        names = {"symbols": [{"name": "Score", "address": 0xFF0010, "kind": "ram", "size": 4},
                             {"name": "LastWord", "address": 0xFFFFF0, "kind": "ram", "size": 2},
                             {"name": "ItemTable", "address": 0x400, "kind": "data", "size": 16}]}
        program = program_for(code, names=names)
        data = packet.make_packet(program, program.select("func_000200"), "synthetic")
        self.assertEqual({r["name"] for r in data["ram_references"]}, {"Score", "LastWord"})
        self.assertEqual(data["rom_data_references"][0]["name"], "ItemTable")
        assembly = "\n".join(row["text"] for row in data["assembly"])
        for name in ("Score", "LastWord", "ItemTable"):
            self.assertIn(name, assembly)

    def test_header_is_rom_data_not_unclassified_memory(self):
        program = program_for(bytes.fromhex("41f900000100" "4e75"))
        catalogue = {**program.catalogue, "header": {"address": 0, "size": 0x200}}
        program = packet.Program(program.rom, catalogue, packet.Symbols(catalogue), packet.decoder(ROOT))
        data = packet.make_packet(program, program.select("func_000200"), "synthetic")
        self.assertEqual(data["rom_data_references"][0]["name"], "header_000000+0x100")

    def test_frame_and_saved_registers_normalize_entry_stack_offsets(self):
        code = bytes.fromhex("4e56fff8" "48e73000" "302e0008" "222f0018" "4cdf000c" "4e5e" "4e75")
        program = program_for(code)
        data = packet.make_packet(program, program.select("func_000200"), "synthetic")
        accesses = data["stack_usage"]["accesses"]
        self.assertEqual([a["entry_offset"] for a in accesses], [4, 4])
        self.assertEqual(data["stack_usage"]["argument_hints"], [{"entry_offset": 4, "width": 4}])
        self.assertIn("uint32_t stack_at_4", data["c_skeleton"])
        self.assertIn("[INFERENCE]", data["c_skeleton"])

    def test_frameless_arguments_correct_for_movem(self):
        program = program_for(bytes.fromhex("48e73000" "302f000c" "4cdf000c" "4e75"))
        usage = packet.stack_usage(program, program.select("func_000200"))
        self.assertEqual(usage["accesses"][0]["entry_offset"], 4)

    def test_cfg_join_does_not_invent_an_argument_offset(self):
        program = program_for(bytes.fromhex("4a40" "6704" "2f00" "6002" "4e71" "302f0004" "4e75"))
        usage = packet.stack_usage(program, program.select("func_000200"))
        self.assertIsNone(usage["accesses"][0]["entry_offset"])
        self.assertEqual(usage["argument_hints"], [])

    def test_overwritten_frame_pointer_becomes_unknown(self):
        program = program_for(bytes.fromhex("4e56fff8" "4dd0" "302e0008" "4e75"))
        usage = packet.stack_usage(program, program.select("func_000200"))
        self.assertIsNone(usage["accesses"][0]["entry_offset"])

    def test_no_parameter_claim_from_no_stack_accesses(self):
        program = program_for(bytes.fromhex("4e75"))
        data = packet.make_packet(program, program.select("func_000200"), "synthetic")
        self.assertIn("does not prove a no-argument", packet.markdown(data))
        self.assertEqual(data["stack_usage"]["argument_hints"], [])

    def test_unknown_indirect_calls_are_explicit(self):
        program = program_for(bytes.fromhex("4e90" "4e75"))
        data = packet.make_packet(program, program.select("func_000200"), "synthetic")
        self.assertEqual(data["unresolved_control_flow"][0]["kind"], "computed_call")
        self.assertEqual(data["callees"], [])

    def test_invalid_unit_and_conflicting_names_fail(self):
        for name in ("func_000301", "func_000999", "ChooseCase"):
            with self.subTest(name=name), self.assertRaises(packet.PacketError):
                self.program.select(name)
        with self.assertRaises(packet.PacketError):
            packet.Symbols(self.catalogue, {"symbols": [{"name": "func_000200", "address": 0x300}]})

    def test_symbol_offsets_and_aliases(self):
        self.assertEqual(self.program.symbols.name(0x302), "ChooseCase+0x2")
        self.assertEqual(self.program.symbols.by_name["func_000300"], 0x300)

    def test_cli_markdown_json_and_absent_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            synthetic_repo(root)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(packet.main(["func_000300", "--repo", str(root)]), 0)
            markdown_path = root / "scratch/packets/rev02/func_000300.md"
            self.assertTrue(markdown_path.is_file())
            self.assertIn("PRIVATE", markdown_path.read_text())
            output = io.StringIO()
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(packet.main(["func_000300", "--repo", str(root), "--json", "--out", str(root / "scratch/custom")]), 0)
            data = json.loads(output.getvalue())
            self.assertTrue(data["private"])
            self.assertTrue((root / "scratch/custom/func_000300.json").is_file())

    def test_public_and_force_tracked_outputs_are_refused_without_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            synthetic_repo(root)
            public = root / "public-packets"
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(packet.main(["func_000300", "--repo", str(root), "--out", str(public)]), 1)
            self.assertFalse(public.exists())
            tracked = root / "scratch/tracked/func_000300.md"
            tracked.parent.mkdir(parents=True)
            tracked.write_text("existing tracked contents")
            subprocess.run(["git", "-C", str(root), "add", "-f", str(tracked)], check=True, capture_output=True)
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(packet.main(["func_000300", "--repo", str(root), "--out", str(tracked.parent)]), 1)
            self.assertEqual(tracked.read_text(), "existing tracked contents")

    def test_symlink_resolutions_must_also_be_ignored_and_untracked(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as outside:
            root = Path(tmp)
            synthetic_repo(root)
            (root / "scratch").mkdir()
            (root / "public").mkdir()
            (root / "scratch/public-link").symlink_to(root / "public", target_is_directory=True)
            (root / "scratch/outside-link").symlink_to(outside, target_is_directory=True)
            public_file = root / "public/existing.md"
            public_file.write_text("tracked target")
            subprocess.run(["git", "-C", str(root), "add", str(public_file)], check=True, capture_output=True)
            (root / "scratch/func_000300.md").symlink_to(public_file)
            for output in (root / "scratch/public-link", root / "scratch/outside-link", root / "scratch"):
                with self.subTest(output=output), contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(packet.main(["func_000300", "--repo", str(root), "--out", str(output)]), 1)
            self.assertEqual(public_file.read_text(), "tracked target")
            self.assertFalse((root / "public/func_000300.md").exists())
            self.assertFalse((Path(outside) / "func_000300.md").exists())

    def test_ignored_internal_symlink_destination_is_permitted(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            synthetic_repo(root)
            (root / "scratch/actual").mkdir(parents=True)
            (root / "scratch/alias").symlink_to(root / "scratch/actual", target_is_directory=True)
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(packet.main(["func_000300", "--repo", str(root), "--out", str(root / "scratch/alias")]), 0)
            self.assertTrue((root / "scratch/actual/func_000300.md").is_file())

    def test_wrong_rom_and_names_schema_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            synthetic_repo(root, names={"schema_version": 2, "version": "rev02", "symbols": []})
            with self.assertRaisesRegex(packet.PacketError, "names schema"):
                packet.load_inputs(root, "rev02")
            (root / "orig/rev02/baserom.bin").write_bytes(b"wrong")
            with self.assertRaisesRegex(packet.PacketError, "SHA-256"):
                packet.load_inputs(root, "rev02")


class PrivateRomPacketTest(unittest.TestCase):
    def test_every_catalogued_function_decodes_in_both_revisions(self):
        for version in ("rev02", "rev00"):
            if not (ROOT / "orig" / version / "baserom.bin").is_file():
                self.skipTest("private retail ROMs not installed")
            rom, catalogue, symbols = packet.load_inputs(ROOT, version)
            program = packet.Program(rom, catalogue, symbols, packet.decoder(ROOT))
            self.assertEqual(len(program.units), len(catalogue["functions"]))
            switches = [unit for unit in program.units if any(unit["address"] <= a < unit["address"] + unit["size"] for a in program.tables)]
            for unit in switches:
                data = packet.make_packet(program, unit, version)
                self.assertTrue(data["jump_tables"])


if __name__ == "__main__":
    unittest.main()
