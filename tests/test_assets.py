"""Synthetic codecs and a private-ROM, every-unit data round trip (unittest)."""
from __future__ import annotations

import importlib.util
import copy
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

TOOL = Path(__file__).resolve().parents[1] / "tools/assets.py"
SPEC = importlib.util.spec_from_file_location("assets", TOOL)
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class AssetCodecTests(unittest.TestCase):
    def setUp(self) -> None:
        self.config = {"symbols": {"Reset": 0x202, "StackTop": 0xFFFF00, "Graphic": 0x3000}}

    def roundtrip(self, kind: str, data: bytes, params: dict | None = None) -> bytes:
        unit = {"name": "Synthetic", "address": 0x2000, "kind": kind, "size": len(data), "params": params or {}}
        result = assets.extract_unit(unit, data, self.config)
        self.assertEqual(assets.pack_unit(unit, result, self.config), data)
        return result

    def test_palette_all_colours_and_unused_bits(self) -> None:
        data = b"".join(struct.pack(">H", value) for value in range(65536))
        self.assertEqual(assets.encode_palette(assets.decode_palette(data)), data)
        self.assertEqual(assets.decode_palette(b"\x0e\xee")["colours"][0], {"r": 7, "g": 7, "b": 7, "unused_bits": 0})

    def test_palette_invalid_colour(self) -> None:
        with self.assertRaises(assets.AssetError):
            assets.encode_palette({"colours": [{"r": 8, "g": 0, "b": 0, "unused_bits": 0}]})

    def test_simple_rle_all_operations_and_boundaries(self) -> None:
        stream = bytes([0x80, 0xC0, 0x33, 63]) + bytes(range(63)) + bytes([0x7F, 0xBF, 0xFF, 0x44, 1, 0xAA])
        expected = bytes(64) + b"\x33" * 64 + bytes(range(63)) + b"\xff" * 63 + b"\x00\x44\xaa"
        decoded, plan, used = assets.decode_rle(stream, len(expected), "simple")
        self.assertEqual(decoded, expected)
        self.assertEqual(used, len(stream))
        self.assertEqual(assets.encode_rle(decoded, plan, "simple"), stream)

    def test_interleaved_rle_all_operations(self) -> None:
        lane = bytes([0xBF, 0xFF, 0xAB, 1, 0xCD, 0x41, 0x63, 0x12, 0x81])
        decoded, plan, used = assets.decode_rle(lane * 4, 280, "interleaved")
        self.assertEqual(used, len(lane) * 4)
        self.assertEqual(decoded[::4][:7], b"\x00\xab\xcd\xff\x12\x21\x12")
        self.assertEqual(assets.encode_rle(decoded, plan, "interleaved"), lane * 4)

    def test_rle_preserves_noncanonical_encoder_choices(self) -> None:
        for stream in (bytes([0xFC, 0]), bytes([4, 0, 0, 0, 0]), bytes([0xBC])):
            decoded, plan, used = assets.decode_rle(stream, 4, "simple")
            self.assertEqual(decoded, bytes(4))
            self.assertEqual(assets.encode_rle(decoded, plan, "simple"), stream)
            self.assertEqual(used, len(stream))

    def test_rle_rejects_malformed_streams(self) -> None:
        for stream in (b"\x00", b"\x04\x11", b"\x80", b"\xff"):
            with self.subTest(stream=stream), self.assertRaises(assets.AssetError):
                assets.decode_rle(stream, 4, "simple")
        with self.assertRaises(assets.AssetError):
            assets.encode_rle(b"\x01\x02", [[["repeat", 2]]], "simple")

    def test_original_dbra_underflow_counts_are_rejected(self) -> None:
        # Entered FF DBRA bodies: 025AE4..025AF0 / 025B76..025B80.
        # Alternating pair body: 025AC2..025ADA, then odd-byte remainder.
        def entered_dbra_iterations(counter: int) -> int:
            return (counter & 0xFFFF) + 1
        self.assertEqual(entered_dbra_iterations(0 - 1), 65536)
        self.assertEqual(2 * entered_dbra_iterations((0 >> 1) - 1), 131072)
        self.assertEqual(2 * entered_dbra_iterations((1 >> 1) - 1) + 1, 131073)
        self.assertEqual(2 * entered_dbra_iterations((2 >> 1) - 1), 2)
        self.assertEqual(2 * entered_dbra_iterations((3 >> 1) - 1) + 1, 3)
        for stream, mode in ((b"\x00", "simple"), (b"\x40", "simple"),
                             (b"\x40", "interleaved"), (b"\x60\xa5", "interleaved"),
                             (b"\x61\xa5", "interleaved")):
            with self.subTest(mode=mode, token=stream[0]), self.assertRaisesRegex(assets.AssetError, "DBRA"):
                assets.decode_rle(stream, 4, mode)
        for op, count in (("ones", 0), ("alternate", 0), ("alternate", 1)):
            with self.subTest(op=op, count=count), self.assertRaises(assets.AssetError):
                assets.encode_rle_token(op, count, "interleaved")

    def test_deterministic_maxrun_and_short_merge_rule(self) -> None:
        data = bytes([0x12, 0x34, 0x56, 0x56, 0x78])
        self.assertEqual(assets.encode_deterministic_rle(data, "simple"), bytes([5]) + data)
        self.assertEqual(assets.encode_deterministic_rle(bytes(64), "simple"), bytes([0x81, 0xBF]))
        self.assertEqual(assets.encode_deterministic_rle(b"\xff" * 4, "simple"), b"\x44")
        alternating = b"\x12" * 4 + b"\x21" * 4 + b"\x12" * 4
        self.assertEqual(assets.encode_deterministic_rle(alternating, "interleaved"), b"\x63\x12" * 4)
        long_ff = bytes([255, 0, 0, 0]) * 50
        self.assertEqual(assets.encode_deterministic_rle(long_ff, "interleaved"),
                         bytes([206, 255, 142, 142, 142]))

    def test_compact_private_plans_have_no_pixel_operands(self) -> None:
        plan = [[["zero", 64], ["literal", 63], ["repeat", 64], ["ones", 63], ["literal", 1]]]
        payload = assets.encode_compact_plan(plan, "simple")
        self.assertEqual(len(payload), 6)  # Selector plus five opcode/count bytes.
        self.assertEqual(assets.decode_compact_plan(payload, 255, "simple"), plan)
        lanes = [[["zero", 64], ["repeat", 64], ["literal", 63], ["ones", 31], ["alternate", 31]]] * 4
        payload = assets.encode_compact_plan(lanes, "interleaved")
        self.assertLess(len(payload), 21)
        self.assertEqual(assets.decode_compact_plan(payload, 1012, "interleaved"), lanes)
        for payload in (b"", b"\x02", b"\x00\x40", b"\x00\x60", b"\x00\x61"):
            with self.subTest(payload=payload), self.assertRaises(assets.AssetError):
                assets.decode_compact_plan(payload, 4, "interleaved")

    def test_private_plan_png_and_pixel_edit_literal(self) -> None:
        unit = {"name": "LiteralTile", "kind": "tiles", "size": 34, "palette": "Palette",
                "params": {"compression": "simple", "decoded_size": 32, "width_tiles": 1, "height_tiles": 1, "order": "column"}}
        original = bytes([32]) + bytes(32) + b"\xa5"  # Noncanonical literal of zeros.
        image = assets.extract_unit(unit, original, self.config, assets.decode_palette(bytes(32)))
        self.assertIn(b"tjRP", image)
        rows, metadata = assets.decode_png(image)
        self.assertEqual(metadata["encoder"], "token-plan-v2")
        self.assertEqual(len(metadata["rle_plan"]), 2)
        self.assertEqual(assets.pack_unit(unit, image, self.config), original)
        changed = [bytearray(row) for row in rows]
        changed[0][:2] = bytes([10, 5])
        edited_image = assets.encode_png(changed, [(0, 0, 0)] * 16, metadata)
        edited = assets.pack_unit(unit, edited_image, self.config)
        self.assertEqual(edited[1], 0xA5)
        self.assertEqual(assets.decode_png(edited_image)[1]["rle_plan"], metadata["rle_plan"])
        self.assertNotEqual(edited, original)

    def test_private_plan_pixel_edit_repeat_and_plan_free_png(self) -> None:
        unit = {"name": "RepeatTile", "kind": "tiles", "size": 4, "palette": "Palette",
                "params": {"compression": "simple", "decoded_size": 32, "width_tiles": 1, "height_tiles": 1, "order": "column"}}
        original = bytes([0xF0, 0xAA, 0xF0, 0xAA])
        image = assets.extract_unit(unit, original, self.config, assets.decode_palette(bytes(32)))
        rows, metadata = assets.decode_png(image)
        self.assertEqual(metadata["encoder"], "token-plan-v2")
        changed = [bytes([11] * 8) for _ in rows]
        edited = assets.pack_unit(unit, assets.encode_png(changed, [(0, 0, 0)] * 16, metadata), self.config)
        self.assertEqual(edited, bytes([0xF0, 0xBB, 0xF0, 0xBB]))
        unit["size"] = 1
        image = assets.extract_unit(unit, b"\xa0", self.config, assets.decode_palette(bytes(32)))
        self.assertNotIn(b"tjRP", image)
        self.assertNotIn("rle_plan", assets.decode_png(image)[1])
        self.assertEqual(assets.pack_unit(unit, image, self.config), b"\xa0")

    def test_tile_orders_and_nibble_order(self) -> None:
        data = bytes((i * 31) & 255 for i in range(192))
        for order in ("column", "row"):
            rows = assets.tiles_to_pixels(data, 3, 2, order)
            self.assertEqual(assets.pixels_to_tiles(rows, 3, 2, order), data)
        self.assertEqual(assets.tiles_to_pixels(b"\xab" * 32, 1, 1, "column")[0][:2], b"\x0a\x0b")

    def test_indexed_png_and_all_filters(self) -> None:
        rows = [bytes((x + y) & 15 for x in range(16)) for y in range(5)]
        metadata = {"test": "synthetic"}
        image = assets.encode_png(rows, [(i * 17,) * 3 for i in range(16)], metadata)
        self.assertEqual(assets.decode_png(image), (rows, metadata))
        # Exercise PNG filters 0..4 with independently constructed filtered scanlines.
        packed_rows = [bytes((r[i] << 4) | r[i + 1] for i in range(0, len(r), 2)) for r in rows]
        filtered = bytearray()
        for y, row in enumerate(packed_rows):
            filtered.append(y)
            for x, value in enumerate(row):
                left, up, corner = row[x - 1] if x else 0, packed_rows[y - 1][x] if y else 0, packed_rows[y - 1][x - 1] if y and x else 0
                p = left + up - corner
                ds = (abs(p - left), abs(p - up), abs(p - corner))
                prediction = (0, left, up, (left + up) // 2, (left, up, corner)[ds.index(min(ds))])[y]
                filtered.append((value - prediction) & 255)
        replacement = (assets.PNG_SIGNATURE + assets.png_chunk(b"IHDR", struct.pack(">IIBBBBB", 16, 5, 4, 3, 0, 0, 0))
                       + assets.png_chunk(b"PLTE", bytes(c for i in range(16) for c in (i * 17,) * 3))
                       + assets.png_chunk(b"tEXt", b"tje-layout\x00" + json.dumps(metadata).encode())
                       + assets.png_chunk(b"IDAT", zlib.compress(bytes(filtered))) + assets.png_chunk(b"IEND", b""))
        self.assertEqual(assets.decode_png(replacement), (rows, metadata))

    def test_png_crc_rejected(self) -> None:
        image = bytearray(assets.encode_png([bytes(8)] * 8, [(0, 0, 0)] * 16, {}))
        image[20] ^= 1
        with self.assertRaises(assets.AssetError):
            assets.decode_png(bytes(image))

    def test_tile_asset_rle_padding(self) -> None:
        unit = {"name": "Tile", "kind": "tiles", "size": 3, "palette": "Palette",
                "params": {"compression": "simple", "decoded_size": 32, "width_tiles": 1, "height_tiles": 1, "order": "column"}}
        palette = assets.decode_palette(bytes(32))
        # Explicit nonzero alignment byte must survive; it cannot be inferred as zero.
        data = bytes([0xE0, 0xAA, 0xA5])
        image = assets.extract_unit(unit, data, self.config, palette)
        self.assertEqual(assets.pack_unit(unit, image, self.config), data)

    def test_raw_tile_asset(self) -> None:
        data = bytes(range(32))
        unit = {"name": "Tile", "kind": "tiles", "size": 32, "palette": "Palette",
                "params": {"compression": "none", "decoded_size": 32, "width_tiles": 1, "height_tiles": 1, "order": "row"}}
        image = assets.extract_unit(unit, data, self.config, assets.decode_palette(bytes(32)))
        self.assertEqual(assets.pack_unit(unit, image, self.config), data)

    def test_wav_unsigned_signed_and_odd_length(self) -> None:
        for signed in (False, True):
            self.roundtrip("pcm", b"\x00\x07\x00\x7f\x80\xff\x55", {"header_bytes": 2, "sample_rate": 8000, "signed": signed,
                           "rate_evidence": "synthetic, not a playback-rate claim"})
        wave = assets.encode_wav(b"\x01\x02\x03", 8000, {"x": 1})
        self.assertEqual(assets.decode_wav(wave), (b"\x01\x02\x03", 8000, {"x": 1}))

    def test_vectors_and_pointer_tables_are_symbolic(self) -> None:
        data = struct.pack(">4I", 0xFFFF00, 0x202, 0, 0x3000)
        for kind in ("vectors", "pointer_table"):
            encoded = self.roundtrip(kind, data)
            self.assertEqual(json.loads(encoded)["entries"][1], {"symbol": "Reset"})
        with self.assertRaises(assets.AssetError):
            assets.resolve_pointer({"symbol": "Unknown"}, {})

    def test_cartridge_header_preserves_fixed_width_fields(self) -> None:
        data = bytes(range(256))
        self.roundtrip("cartridge_header", data)

    def test_sprite_headers_and_nested_metasprite(self) -> None:
        sprite = struct.pack(">BBbbbBI", 2, 3, -128, 127, -1, 0x82, 0x3000)
        self.roundtrip("sprite_header", sprite)
        self.roundtrip("metasprite", b"\x02\x01\x02\x03" + sprite * 2)

    def test_text_values_records_padding_binary(self) -> None:
        self.roundtrip("text", b"Hello\x00World\x00\x00")
        self.roundtrip("values", b"\xff\xff\x00\x01", {"width": 2, "signed": True})
        self.roundtrip("records", b"\x01\xff\xfe\x02\x00\x03", {"fields": [{"name": "id", "width": 1}, {"name": "x", "width": 2, "signed": True}]})
        self.roundtrip("padding", b"\xff" * 31)
        self.roundtrip("z80_binary", b"\x01\x02\x03")
        self.roundtrip("raw", b"\x01\x02\x03")

    def unreferenced_fixture(self, version: str = "rev02") -> tuple[dict, dict, bytes]:
        address = 0x03C34A if version == "rev02" else 0x03C2DA
        unit = {"name": "FMPatchAuxiliary", "address": address, "size": 6, "kind": "unreferenced",
                "asset": "Unknown.json", "provenance": ["synthetic"], "params": {
                    "referenced": False, "evidence": {
                        "scope": "catalogued-instructions-typed-pointers-rom-longwords",
                        "code_references": 0, "typed_pointer_references": 0, "rom_longword_references": 0,
                        "previous_unit": "FMPatches", "next_unit": "InstrumentSelectionMaps",
                        "consumer_analysis": "synthetic boundary analysis",
                        "limitation": "No observed root; computed use is not excluded."},
                    "observed_pattern": {"count": 6, "minimum": 0, "maximum": 4, "distinct": 5}}}
        previous = {"name": "FMPatches", "address": address - 1862, "size": 1862, "kind": "records",
                    "asset": "Previous.json", "provenance": ["synthetic"],
                    "params": {"fields": [{"name": f"field_{i}", "width": 1} for i in range(38)]}}
        following = {"name": "InstrumentSelectionMaps", "address": address + 6, "size": 224, "kind": "values",
                     "asset": "Following.json", "provenance": ["synthetic"], "params": {
                         "width": 1, "byte_order": "big", "shape": [14, 16]}}
        config = {"schema_version": 1, "version": version, "rom_size": address + 230,
                  "units": [previous, unit, following]}
        return config, unit, bytes([0, 1, 2, 3, 4, 4])

    def trailing_fixture(self, version: str = "rev02") -> tuple[dict, dict, bytes]:
        address = 0x026446 if version == "rev02" else 0x0263CE
        unit = {"name": "LevelEarthlingChoices", "address": address, "size": 521, "kind": "values",
                "asset": "Choices.json", "provenance": ["synthetic"], "params": {
                    "width": 1, "byte_order": "big", "shape": [26, 20], "trailing_elements": 1,
                    "trailing_evidence": {"basis": "same-element-format", "consumer": "func_017ac8",
                                          "source": "synthetic reader bounds",
                                          "qualification": "[INFERENCE] One unindexed element."}}}
        config = {"schema_version": 1, "version": version, "rom_size": address + 521, "units": [unit]}
        return config, unit, bytes(i % 25 for i in range(520)) + b"\x10"

    def test_values_separate_only_the_reviewed_whole_trailing_element(self) -> None:
        for version in ("rev02", "rev00"):
            config, unit, data = self.trailing_fixture(version)
            assets.validate_config(config)
            asset = assets.extract_unit(unit, data, config)
            decoded = json.loads(asset)
            self.assertEqual(decoded, {"values": list(data[:-1]), "trailing_values": [16]})
            self.assertEqual(assets.pack_unit(unit, asset, config), data)
            for main, trailing in ((list(data[:-2]), [0, 16]), (list(data), [])):
                with self.subTest(version=version, main=len(main)), self.assertRaises(assets.AssetError):
                    assets.pack_unit(unit, assets.json_bytes({"values": main, "trailing_values": trailing}), config)
            for value in (-1, 256, True, "16", [16]):
                decoded["trailing_values"] = [value]
                with self.subTest(value=value), self.assertRaises(assets.AssetError):
                    assets.pack_unit(unit, assets.json_bytes(decoded), config)

    def test_trailing_elements_cannot_hide_large_or_unreviewed_payloads(self) -> None:
        config, unit, _ = self.trailing_fixture()
        payload = bytes((i * 73 + 19) % 256 for i in range(4096))
        unit["size"], config["rom_size"] = len(payload), unit["address"] + len(payload)
        unit["params"].update(shape=[1], trailing_elements=4095)
        for operation in (lambda: assets.validate_config(config),
                          lambda: assets.extract_unit(unit, payload, config),
                          lambda: assets.pack_unit(unit, assets.json_bytes({
                              "values": [payload[0]], "trailing_values": list(payload[1:])}), config)):
            with self.assertRaises(assets.AssetError):
                operation()
        config, unit, data = self.trailing_fixture()
        unit["name"] = "UnreviewedTable"
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config)
        config, unit, data = self.trailing_fixture()
        unit["params"]["width"] = 2
        unit["size"] *= 2
        config["rom_size"] = unit["address"] + unit["size"]
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config)
        with self.assertRaises(assets.AssetError):
            self.roundtrip("values", b"\x00\x01\xff", {"width": 2})

    def test_trailing_elements_require_structured_ownership_evidence(self) -> None:
        for evidence in (True, "unverified", {}, {"basis": "bytes"}, {
                "basis": "same-element-format", "consumer": "func_017ac8", "source": True, "qualification": "x"}):
            config, unit, _ = self.trailing_fixture()
            unit["params"]["trailing_evidence"] = evidence
            with self.subTest(evidence=evidence), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
        config, unit, data = self.trailing_fixture()
        asset = assets.extract_unit(unit, data, config)
        del unit["params"]["trailing_elements"]
        with self.assertRaises(assets.AssetError):
            assets.pack_unit(unit, asset, config)

    def test_unreferenced_bytes_preserve_unknown_semantics_with_reviewed_bounds(self) -> None:
        for version in ("rev02", "rev00"):
            config, unit, data = self.unreferenced_fixture(version)
            assets.validate_config(config)
            asset = assets.extract_unit(unit, data, config)
            self.assertEqual(json.loads(asset), {"format": "unreferenced-bytes-v1", "bytes": list(data)})
            self.assertEqual(assets.pack_unit(unit, asset, config), data)
            for bad in ({"format": "invented-fields", "bytes": list(data)},
                        {"format": "unreferenced-bytes-v1", "bytes": [0] * 6},
                        {"format": "unreferenced-bytes-v1", "bytes": list(data) + [0]}):
                with self.subTest(bad=bad), self.assertRaises(assets.AssetError):
                    assets.pack_unit(unit, assets.json_bytes(bad), config)

    def test_unreferenced_rejects_large_payloads_at_load_extract_and_pack(self) -> None:
        for size in (9, 4096):
            config, unit, _ = self.unreferenced_fixture()
            unit["size"] = size
            config["rom_size"] = unit["address"] + size + 224
            payload = bytes((i * 73 + 19) % 256 for i in range(size))
            encoded = assets.json_bytes({"format": "unreferenced-bytes-v1", "bytes": list(payload)})
            for operation in (lambda: assets.validate_config(config),
                              lambda: assets.extract_unit(unit, payload, config),
                              lambda: assets.pack_unit(unit, encoded, config)):
                with self.subTest(size=size), self.assertRaises(assets.AssetError):
                    operation()
        config, unit, data = self.unreferenced_fixture()
        unit["name"] = "ArbitrarySmallPayload"
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config)
        config, unit, data = self.unreferenced_fixture()
        unit["address"] += 1
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config)

    def test_unreferenced_requires_structured_typed_evidence_and_pattern(self) -> None:
        for key in ("referenced", "evidence", "observed_pattern"):
            config, unit, _ = self.unreferenced_fixture()
            del unit["params"][key]
            with self.subTest(missing=key), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
        for evidence in (True, "not checked", [], {}):
            config, unit, _ = self.unreferenced_fixture()
            unit["params"]["evidence"] = evidence
            with self.subTest(evidence=evidence), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
        for key, value in (("code_references", True), ("typed_pointer_references", 1),
                           ("rom_longword_references", -1), ("consumer_analysis", True), ("limitation", "")):
            config, unit, _ = self.unreferenced_fixture()
            unit["params"]["evidence"][key] = value
            with self.subTest(key=key), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
        for pattern in (True, [1], {}, {"count": True, "minimum": 0, "maximum": 4, "distinct": 5}):
            config, unit, _ = self.unreferenced_fixture()
            unit["params"]["observed_pattern"] = pattern
            with self.subTest(pattern=pattern), self.assertRaises(assets.AssetError):
                assets.validate_config(config)

    def test_unreferenced_requires_two_contiguous_reviewed_typed_neighbours(self) -> None:
        original, _, _ = self.unreferenced_fixture()
        for index in (0, 2):
            config = copy.deepcopy(original)
            del config["units"][index]
            with self.subTest(missing=index), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
            config = copy.deepcopy(original)
            config["units"][index].update(kind="raw", raw_reason="synthetic negative control")
            with self.subTest(raw=index), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
            config = copy.deepcopy(original)
            config["units"][index]["name"] = "DifferentTypedNeighbour"
            with self.subTest(renamed=index), self.assertRaises(assets.AssetError):
                assets.validate_config(config)
        config = copy.deepcopy(original)
        config["units"][0]["size"] -= 1
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config)

    def test_padding_fill_is_a_checked_layout_parameter(self) -> None:
        unit = {"name": "Alignment", "kind": "padding", "size": 1, "params": {"alignment": 2, "fill": 255}}
        self.roundtrip("padding", b"\xff", unit["params"])
        with self.assertRaises(assets.AssetError):
            assets.extract_unit(unit, b"\x00", self.config)
        with self.assertRaises(assets.AssetError):
            assets.pack_unit(unit, assets.json_bytes({"fill": 0, "count": 1}), self.config)

    def test_progress_excludes_unreferenced_bytes_from_typed_credit(self) -> None:
        spec = importlib.util.spec_from_file_location("progress_assets_test", TOOL.parent.parent / "ci/progress_report.py")
        progress = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(progress)
        for version in ("rev02", "rev00"):
            catalogue = json.loads((assets.CONFIG_ROOT / version / "catalogue.json").read_text())
            counts = progress.data_counts(version, catalogue)
            total = sum(span["size"] for span in catalogue["data"])
            self.assertEqual(counts, {"typed": total - 6, "unreferenced": 6, "raw": 0})
            measures = progress.measures([], set(), total, counts["typed"])
            self.assertEqual(int(measures["matched_data"]), total - 6)
            self.assertLess(measures["matched_data_percent"], 100)

    def test_record_pointer_fields(self) -> None:
        self.roundtrip("records", b"\x00\x00\x30\x00\xff",
                       {"fields": [{"name": "graphic", "width": 4, "type": "pointer"}, {"name": "offset", "width": 1, "signed": True}]})

    def test_animation_commands(self) -> None:
        self.roundtrip("animation", bytes(range(256)))

    def test_music_commands_and_absolute_callback_alignment(self) -> None:
        # Include every opcode, both callback alignment cases and signed jumps.
        data = (b"\x00\x01\x12\x34\x02\x03\x56\x78\x04\x09\x05\x00\x03"
                b"\x06\xff\xfe\x07\xa5\x00\x00\x02\x02\x08\xff\xf0\x09\x0a\x0b\x0c\x80\xff")
        self.roundtrip("music", data)
        for address in (0x2000, 0x2001):
            stream = b"\x07" + (b"\xa5" if address % 2 == 0 else b"") + b"\x00\x00\x02\x02"
            commands = assets.decode_music(stream, address, self.config["symbols"])
            self.assertEqual(assets.encode_music(commands, address, self.config["symbols"]), stream)
        for stream in (b"\x7f", b"\x01\x00", b"\x07"):
            with self.assertRaises(assets.AssetError):
                assets.decode_music(stream, 0x2000, {})

    def test_partition_and_path_validation(self) -> None:
        catalogue = {"version": "test", "rom_sha256": "hash", "header": {"address": 0, "size": 4}, "data": [{"address": 8, "size": 4}]}
        config = {"schema_version": 1, "version": "test", "rom_sha256": "hash", "rom_size": 12, "units": [
            {"name": "A", "address": 0, "size": 4, "kind": "padding", "asset": "A.json", "provenance": ["derived"]},
            {"name": "B", "address": 8, "size": 4, "kind": "padding", "asset": "B.json", "provenance": ["derived"]}]}
        assets.validate_config(config, catalogue)
        config["units"][1]["address"] = 9
        with self.assertRaises(assets.AssetError):
            assets.validate_config(config, catalogue)
        with self.assertRaises(assets.AssetError):
            assets.asset_path(Path("."), "../outside.bin")


@unittest.skipUnless((assets.ROOT / "orig/rev02/baserom.bin").is_file(), "private REV02 ROM not imported")
class RealRomAssetTests(unittest.TestCase):
    def test_every_unit_roundtrips_and_covers_noncode(self) -> None:
        config = json.loads((assets.CONFIG_ROOT / "rev02/data_units.json").read_text())
        catalogue = json.loads((assets.ROOT / "config/rev02/catalogue.json").read_text())
        assets.validate_config(config, catalogue)
        rom = (assets.ROOT / "orig/rev02/baserom.bin").read_bytes()
        parent = Path(os.environ.get("TJ_ASSET_TEST_TMPDIR", str(assets.ROOT / "build/asset-tests")))
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent) as temp:
            base = Path(temp)
            extraction = assets.extract(config, rom, base / "assets")
            storage = extraction["rle_storage"]
            self.assertGreater(storage["deterministic_units"], 0)
            self.assertEqual(storage["deterministic_units"] + storage["planned_units"], storage["compressed_units"])
            self.assertLess(storage["plan_bytes"], storage["original_bytes"])
            self.assertLess(storage["plan_original_ratio"], 0.05)
            self.assertEqual(storage["plan_bytes"], storage["plan_chunk_bytes"] + storage["plan_encoder_tag_bytes"])
            for unit in config["units"]:
                self.assertNotIn("rle_plan", unit)
                self.assertNotIn("rle_plan", unit.get("params", {}))
            report = assets.pack(config, base / "assets", base / "packed")
            self.assertTrue(report["all_exact"])
            for unit in report["units"]:
                with self.subTest(name=unit["name"]):
                    start = assets.integer(unit["address"])
                    self.assertEqual((base / "packed" / unit["path"]).read_bytes(), rom[start:start + unit["size"]])
            self.assertEqual(sum(unit["size"] for unit in report["units"]), 512 + sum(d["size"] for d in catalogue["data"]))


if __name__ == "__main__":
    unittest.main()
