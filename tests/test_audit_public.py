import contextlib
import bz2
import base64
import gzip
import binascii
import hashlib
import io
import json
import lzma
import os
import subprocess
import quopri
import struct
import sys
import tempfile
import unittest
from unittest import mock
import zlib
from pathlib import Path
from unittest.mock import patch
from urllib.parse import quote_from_bytes

ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/title.json").is_file() and (p / "tools/m68k.py").is_file())
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import audit_public

LONG_TEXT = b"A completely synthetic cartridge message used only for tests: collect every missing piece before returning home."


class AuditTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.git("init", "-q")
        self.git("config", "user.email", "synthetic@example.invalid")
        self.git("config", "user.name", "Synthetic Audit Test")
        self.put("README.md", b"# Synthetic repository\n")
        self.git("add", "README.md")
        self.git("commit", "-qm", "clean synthetic tree")
        self.image = b"".join(hashlib.sha256(str(n).encode()).digest() for n in range(32)) + b"PRIVATE PRINTABLE WINDOW!!" + LONG_TEXT
        self.put("orig/rev02/baserom.bin", self.image)
        self.put("orig/rev00/baserom.bin", self.image[::-1])
        title = {"versions": {version: {"orig": f"orig/{version}/baserom.bin", "size": len(image),
                                       "sha256": hashlib.sha256(image).hexdigest()}
                              for version, image in (("rev02", self.image), ("rev00", self.image[::-1]))}}
        self.put("config/title.json", json.dumps(title).encode())

    def git(self, *args):
        return subprocess.run(["git", "-C", str(self.root), *args], check=True, capture_output=True).stdout

    def put(self, path, raw):
        dest = self.root / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(raw)

    def reasons(self, **kwargs):
        findings, summary = audit_public.audit(self.root, **kwargs)
        return "\n".join(f"{f.path}: {f.reason} [{f.source}]" for f in findings), summary

    def stage(self, path, raw):
        self.put(path, raw)
        self.git("add", "-f", "--", path)

    def test_clean_tree_and_untracked_private_files_are_not_audited(self):
        self.put("scratch/private.bin", self.image)
        reasons, summary = self.reasons(history=True)
        self.assertEqual(reasons, "")
        self.assertEqual(summary["roms"], 2)

    def test_staged_hex_and_worktree_cleaned_are_still_caught(self):
        self.stage("notes.txt", self.image[50:62].hex().encode())
        self.put("notes.txt", b"clean worktree\n")
        reasons, _ = self.reasons()
        self.assertIn("hex-encoded", reasons)
        self.assertIn("ROM window", reasons)
        self.assertIn("index:0", reasons)

    def test_tracked_unstaged_long_text_extract_is_caught(self):
        self.stage("notes.txt", b"clean\n")
        self.git("commit", "-qm", "track notes")
        self.put("notes.txt", LONG_TEXT)
        reasons, _ = self.reasons()
        self.assertIn("raw ROM window", reasons)
        self.assertIn("worktree", reasons)

    def test_hex_formats_and_twelve_byte_threshold(self):
        index = audit_public.RomIndex([self.image])
        raw = self.image[11:23]
        formats = [raw.hex().encode(), raw.hex(" ").upper().encode(),
                   b", ".join(f"0x{b:02x}".encode() for b in raw),
                   b", ".join(f"${int.from_bytes(raw[n:n+2], 'big'):04x}".encode() for n in range(0, len(raw), 2)),
                   b'"' + raw[:6].hex().encode() + b'"\n"' + raw[6:].hex().encode() + b'"']
        for encoded in formats:
            with self.subTest(encoded=encoded):
                self.assertTrue(any("ROM window" in reason for reason in audit_public.content_reasons("x.txt", encoded, index)))
        self.assertFalse(audit_public.content_reasons("x.txt", raw[:11].hex().encode(), index))
        escaped = b"".join(b"\\x" + f"{b:02x}".encode() for b in raw)
        self.assertIn("hex-escaped", " ".join(audit_public.content_reasons("x.py", escaped, index)))

    def test_history_catches_deleted_leak_and_paths_for_reused_blob(self):
        self.stage("leak.txt", self.image[100:112].hex().encode())
        self.stage("scratch/copy.txt", b"# Synthetic repository\n")
        self.git("commit", "-qm", "plant historical leaks")
        self.git("rm", "-q", "leak.txt", "scratch/copy.txt")
        self.git("commit", "-qm", "remove leaks")
        self.assertEqual(self.reasons()[0], "")
        reasons, _ = self.reasons(history=True)
        self.assertIn("hex-encoded", reasons)
        self.assertIn("scratch/copy.txt: file under", reasons)

    def test_all_refs_including_non_head_branch_are_audited(self):
        self.git("branch", "clean")
        self.stage("branch-leak.txt", self.image[10:22].hex().encode())
        self.git("commit", "-qm", "side branch leak")
        self.git("checkout", "-q", "clean")
        self.assertEqual(self.reasons()[0], "")
        self.assertIn("branch-leak.txt", self.reasons(history=True)[0])

    def test_history_refuses_a_shallow_clone(self):
        clone = self.root / "shallow-clone"
        subprocess.run(["git", "clone", "-q", "--depth", "1", self.root.as_uri(), str(clone)], check=True, capture_output=True)
        with self.assertRaisesRegex(audit_public.AuditError, "full history"):
            audit_public.audit(clone, history=True, no_rom=True)

    def test_generated_assembly_listing_incbin_and_private_paths(self):
        cases = {"marker.s": b".L" + b"i_001234:\tmove.w d0,d1\n",
                 "inc.s": b'.inc' + b'bin "orig/rev02/baserom.bin",0,2\n',
                 "listing.txt": b"00000200: 4e75 rts\n",
                 "capstone.txt": b"0x000200  4e 75  rts\n",
                 "packet.txt": b"000200:  moveq #1,d0\n"}
        for path, raw in cases.items():
            self.stage(path, raw)
        for directory in ("build", "asm", "orig", ".cache", "scratch"):
            self.stage(f"nested/{directory}/innocent.txt", b"ordinary text\n")
        reasons, _ = self.reasons(no_rom=True)
        for text in ("instruction marker", "includes a private baserom", "disassembly listing", "private/generated directory"):
            self.assertIn(text, reasons)

    def test_only_reviewed_assembly_sources_leave_the_private_asm_rule(self):
        source = b"| clear a word\n\t.text\n\t.globl Clear\nClear:\n\tclr.w %d0\n\trts\n"
        self.stage("src/asm/clear.s", source)                     # ruling R007: allowed
        self.stage("src/asm/deeper/clear.s", source)              # only directly under src/asm
        self.stage("src/asm/notes.txt", b"ordinary text\n")       # only .s files
        self.stage("tools/asm/clear.s", source)                   # only src/asm
        self.stage("src/asm/marker.s", b".L" + b"i_001234:\tmove.w d0,d1\n")   # content rules still apply
        reasons, _ = self.reasons(no_rom=True)
        self.assertNotIn("src/asm/clear.s:", reasons)
        for path in ("src/asm/deeper/clear.s", "src/asm/notes.txt", "tools/asm/clear.s"):
            self.assertIn(f"{path}: file under a private/generated directory", reasons)
        self.assertIn("src/asm/marker.s", reasons)

    def test_binary_assets_extensions_markdown_and_size(self):
        cases = {"binary.txt": bytes([0, 255, 1]), "dump.GEN": b"looks like text", "photo.png": b"not really a PNG",
                 "dump.md": b"this is not markdown", "large.txt": b"x" * (audit_public.MAX_BYTES + 1),
                 "valid.md": b"# Valid Markdown\nprose\n"}
        for path, data in cases.items():
            self.stage(path, data)
        reasons, _ = self.reasons(no_rom=True)
        for text in ("non-text blob", "asset filename", "possible ROM", "exceeds 1 MB"):
            self.assertIn(text, reasons)
        self.assertNotIn("valid.md:", reasons)

    def test_no_rom_skips_only_byte_comparison(self):
        self.stage("ascii.txt", self.image[48:80].hex().encode())
        self.assertIn("ROM window", self.reasons()[0])
        self.assertEqual(self.reasons(no_rom=True)[0], "")
        self.stage("blob.txt", bytes([0, 1]))
        self.assertIn("non-text", self.reasons(no_rom=True)[0])

    def test_public_headers_and_short_plaintext_are_permitted(self):
        console = b"SEGA GENESIS" + b" " * 4
        index = audit_public.RomIndex([console + b"(C)SEGA 1991.OCT" + b" " * 32])
        self.assertEqual(audit_public.content_reasons("tests/support.py", console, index), [])
        self.assertEqual(audit_public.content_reasons("elsewhere.py", console, index), [])
        self.assertEqual(audit_public.content_reasons("docs/header.md", b"# " + b"(C)SEGA 1991.OCT", index), [])
        self.assertEqual(audit_public.content_reasons("x.py", b" " * 32, index), [])

    def test_cli_exit_status_and_report_does_not_print_leaked_bytes(self):
        self.stage("secret.txt", LONG_TEXT)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = audit_public.main(["--repo", str(self.root), "--history"])
        self.assertEqual(status, 1)
        self.assertIn("audit_public: FAIL", output.getvalue())
        self.assertNotIn(LONG_TEXT.decode(), output.getvalue())

    def test_base64_json_escapes_and_split_payloads_are_caught(self):
        raw = self.image[17:112]
        encoded = base64.b64encode(raw).decode()
        url = base64.urlsafe_b64encode(raw).decode()
        cases = {
            "base64": {"code": encoded},
            "url-safe": {"code": url},
            "url-unpadded": {"code": url.rstrip("=")},
            "base64-lines": {"code": "\n".join(encoded[n:n + 12] for n in range(0, len(encoded), 12))},
            "json-escaped": {"code": raw[:32].decode("latin1")},
            "split-hex": {"code": [raw[n:n + 4].hex() for n in range(0, 32, 4)]},
            "split-base64": {"code": [encoded[n:n + 6] for n in range(0, len(encoded), 6)]},
            "split-bytes": {"code": [raw[n:n + 4].decode("latin1") for n in range(0, 32, 4)]},
        }
        for name, value in cases.items():
            self.stage(f"{name}.json", json.dumps(value, ensure_ascii=True).encode())
        reasons, _ = self.reasons()
        for name in cases:
            with self.subTest(name=name):
                self.assertIn(f"{name}.json:", reasons)
        self.assertIn("base64.json: opaque", self.reasons(no_rom=True)[0])

    def test_encoded_decoding_limits_fail_closed(self):
        index = audit_public.RomIndex([self.image])
        data = json.dumps({"code": ["ordinary text"] * 8}).encode()
        with patch.object(audit_public, "MAX_DECODED_BYTES", 32):
            self.assertIn("byte budget", " ".join(audit_public.content_reasons("x.json", data, index)))
        with patch.object(audit_public, "MAX_JSON_NODES", 2):
            self.assertIn("node/depth budget", " ".join(audit_public.content_reasons("x.json", data, index)))
        deep = ("[" * 70 + "0" + "]" * 70).encode()
        self.assertIn("node/depth budget", " ".join(audit_public.content_reasons("x.json", deep, index)))

    def test_encoded_short_header_text_is_not_rom_evidence(self):
        console = b"SEGA GENESIS" + b" " * 4
        header = b"(C)SEGA 1991.OCT"
        index = audit_public.RomIndex([console + header])
        allowed = json.dumps({"text": base64.b64encode(header).decode()}).encode()
        self.assertEqual(audit_public.content_reasons("metadata.json", allowed, index), [])
        allowed = json.dumps({"text": base64.b64encode(console).decode()}).encode()
        self.assertEqual(audit_public.content_reasons("tests/support.py", allowed, index), [])
        self.assertEqual(audit_public.content_reasons("other.py", allowed, index), [])

    def test_normal_mode_requires_both_pinned_roms(self):
        self.stage("encoded.json", json.dumps({"code": base64.b64encode(self.image[32:64]).decode()}).encode())
        path = self.root / "orig/rev00/baserom.bin"
        for invalid in (None, b"", self.image[:-1], bytes(len(self.image))):
            with self.subTest(invalid_size=None if invalid is None else len(invalid)):
                if invalid is None:
                    path.unlink()
                else:
                    path.write_bytes(invalid)
                stdout, stderr = io.StringIO(), io.StringIO()
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    self.assertEqual(audit_public.main(["--repo", str(self.root), "--history"]), 1)
                self.assertIn("reference ROM", stderr.getvalue())
                self.assertNotIn("PASS", stdout.getvalue())
                self.assertEqual(self.reasons(no_rom=True)[0], "")
                path.write_bytes(self.image[::-1])
        (self.root / "config/title.json").unlink()
        with self.assertRaises(audit_public.AuditError):
            self.reasons()

    def test_short_prose_matches_are_not_binary_evidence(self):
        prose = b"an incoming parameter specifically"
        index = audit_public.RomIndex([prose + LONG_TEXT])
        cases = [prose, prose + b"; this explanation continues for many characters without quoting the cartridge.",
                 json.dumps({"note": prose.decode()}).encode(),
                 json.dumps({"note": (prose + b" and a normal longer explanation.").decode()}).encode()]
        for data in cases:
            with self.subTest(data=data):
                self.assertEqual(audit_public.content_reasons("notes.txt", data, index), [])
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("notes.txt", LONG_TEXT, index)))
        encoded = json.dumps({"note": base64.b64encode(prose).decode()}).encode()
        self.assertIn("opaque", " ".join(audit_public.content_reasons("notes.txt", encoded, index)))

    def test_real_classification_prose_is_allowed(self):
        path = ROOT / "config/rev02/classification.json"
        if not path.is_file() or not (ROOT / "orig/rev02/baserom.bin").is_file():
            self.skipTest("classification/retail references absent")
        index = audit_public.RomIndex(audit_public.reference_images(ROOT))
        self.assertEqual(audit_public.content_reasons("config/rev02/classification.json", path.read_bytes(), index), [])

    def test_python_repr_adjacent_and_plus_literals_are_decoded_safely(self):
        raw = self.image[17:113]
        chunks = [repr(raw[n:n + 8]).encode() for n in range(0, len(raw), 8)]
        cases = [b"code = " + repr(raw).encode(), b"code = (\n" + b"\n".join(chunks) + b"\n)",
                 b"code = " + b" + ".join(chunks)]
        for data in cases:
            self.stage("repr.txt", data)
            self.assertIn("Python bytes literal", self.reasons()[0])
        sentinel = self.root / "not-executed"
        source = f"__import__('pathlib').Path({str(sentinel)!r}).touch()\n".encode() + b"value = " + repr(raw).encode()
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("snippet.txt", source, audit_public.RomIndex([self.image]))))
        self.assertFalse(sentinel.exists())

    def test_commented_c_arrays_preserve_order_across_numeric_rows(self):
        raw = self.image[17:113]
        rows = [b", ".join(f"0x{value:02x}".encode() for value in raw[n:n + 8]) + f", /* offset {n} */".encode()
                for n in range(0, len(raw), 8)]
        source = b"const unsigned char code[] = {\n" + b"\n".join(rows) + b"\n};\n"
        self.stage("array.txt", source)
        self.assertIn("numeric run", self.reasons()[0])
        index = audit_public.RomIndex([self.image])
        halves = [b", ".join(f"0x{byte:02x}".encode() for byte in raw[n:n + 6]) for n in (0, 6)]
        separate = b"int a[]={" + halves[0] + b"}; int b[]={" + halves[1] + b"};"
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("expressions.txt", separate, index)))
        expression = b"int a[]={" + halves[0] + b" + 1," + halves[1] + b"};"
        self.assertEqual(audit_public.content_reasons("expressions.txt", expression, index), [])

    def test_gzip_base64_is_archive_policy_without_inflation(self):
        for payload in (self.image[17:113], b"x" * (audit_public.MAX_DECODED_BYTES + 1)):
            encoded = base64.b64encode(gzip.compress(payload, mtime=0)).decode()
            self.stage("compressed.json", json.dumps({"code": encoded}).encode())
            self.assertIn("gzip-encoded archive", self.reasons()[0])
            self.assertIn("gzip-encoded archive", self.reasons(no_rom=True)[0])

    def test_low_address_capstone_and_visual_differ_outputs(self):
        fixtures = [b"0x0:\trts\n", b"202: tstl 0xa10008\n", b"a: bnes 20\n",
                    b"0x202\tmove.w\td0, d1\n", b"0: 4e75 rts\n",
                    b"TARGET                  CURRENT (5)\n202: tstl 0xa10008      i 202: tstl 0xa1000a\n",
                    b"TARGET                  CURRENT (0)\n0: jsr func_000300        0: jsr func_000300\n"]
        for data in fixtures:
            with self.subTest(data=data):
                self.assertIn("disassembly listing", " ".join(audit_public.content_reasons("output.txt", data, None)))
        self.assertEqual(audit_public.content_reasons("notes.txt", b"2026: This is a normal note, not a listing.\n", None), [])
        explanation = b"0x200 jsr $300.l / bsr.w $300 / bra.s self (synthetic reset handler)\n"
        self.assertEqual(audit_public.content_reasons("notes.txt", explanation, None), [])

    def test_c_literals_only_are_allowed_under_source_and_include(self):
        index = audit_public.RomIndex([self.image])
        literal = json.dumps(LONG_TEXT.decode()).encode()
        source = b"const char *message = " + literal + b";\n"
        for path in ("src/text.c", "src/game/text.C", "include/text.h"):
            with self.subTest(path=path):
                self.assertEqual(audit_public.content_reasons(path, source, index), [])
                self.assertEqual(audit_public.content_reasons(path, literal, index), [])
        for path in ("text.c", "docs/text.h", "src/text.txt"):
            self.assertIn("ROM window", " ".join(audit_public.content_reasons(path, source, index)))
        comments = b"/* " + LONG_TEXT + b" */"
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("src/text.c", comments, index)))
        array = b"unsigned char data[]={" + b",".join(f"0x{value:02x}".encode() for value in self.image[17:49]) + b"};"
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("src/data.c", array, index)))

    def test_config_json_has_explicit_four_megabyte_limit(self):
        for path in ("config/names.json", "config/rev00/names.json"):
            self.assertEqual(audit_public.path_reasons(path, audit_public.CONFIG_MAX_BYTES, "100644"), [])
            self.assertIn("exceeds 4 MB", " ".join(audit_public.path_reasons(path, audit_public.CONFIG_MAX_BYTES + 1, "100644")))
        for path in ("other/names.json", "config/names.txt"):
            self.assertIn("exceeds 1 MB", " ".join(audit_public.path_reasons(path, audit_public.MAX_BYTES + 1, "100644")))
        self.stage("config/large.json", json.dumps({"note": "ordinary prose words " * (audit_public.MAX_BYTES // 20 + 1)}).encode())
        self.assertEqual(self.reasons()[0], "")

    def test_large_config_content_is_scanned_not_size_exempted(self):
        payload = {"note": "ordinary prose words " * (audit_public.MAX_BYTES // 20 + 1), "leak": self.image[17:113].hex()}
        self.stage("config/large.json", json.dumps(payload).encode())
        self.assertIn("ROM window", self.reasons()[0])

    def test_c_binary_literals_decode_hex_octal_mixed_and_adjacency(self):
        raw = self.image[17:217]
        index = audit_public.RomIndex([self.image, self.image[::-1]])
        encodings = [
            b"".join(f"\\x{byte:02x}".encode() for byte in raw),
            b"".join(f"\\{byte:03o}".encode() for byte in raw),
            b"".join(bytes([byte]) if 32 <= byte < 127 and byte not in (34, 92) else f"\\{byte:03o}".encode() for byte in raw),
        ]
        for encoded in encodings:
            source = b'const unsigned char data[] = "' + encoded + b'";\n'
            for path in ("src/data.c", "include/data.h"):
                with self.subTest(path=path, encoding=encoded[:8]):
                    groups = list(audit_public.c_literals(source, audit_public.DecodeBudget()))
                    self.assertEqual(groups[0].raw, raw)
                    self.assertIn("ROM window", " ".join(audit_public.content_reasons(path, source, index)))
                    self.assertIn("suspicious C literal", " ".join(audit_public.content_reasons(path, source, None)))
        pieces = [b'"' + b"".join(f"\\x{byte:02x}".encode() for byte in raw[n:n + 8]) + b'"' for n in range(0, len(raw), 8)]
        adjacent = b"const char data[] = " + b" /* join */\n".join(pieces) + b";\n"
        self.assertEqual(list(audit_public.c_literals(adjacent, audit_public.DecodeBudget()))[0].raw, raw)
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("src/adjacent.c", adjacent, index)))
        self.assertIn("suspicious C literal", " ".join(audit_public.content_reasons("include/commented.h", b"/*\n" + adjacent + b"*/", None)))

    def test_only_decoded_printable_c_text_is_exempt(self):
        index = audit_public.RomIndex([self.image])
        escaped = b"".join(f"\\{byte:03o}".encode() for byte in LONG_TEXT)
        source = b'const char *message = "' + escaped + b'";\n'
        self.assertEqual(audit_public.content_reasons("src/text.c", source, index), [])
        self.assertEqual(audit_public.content_reasons("include/text.h", source, None), [])
        decoded, _ = audit_public.decode_c_literal(b'"A\\101\\x42\\n"')
        self.assertEqual(decoded, b"AAB\n")
        for count in (audit_public.MAX_NON_TEXT_ESCAPES, audit_public.MAX_NON_TEXT_ESCAPES + 1):
            data = b'const char *data = "' + b"\\000" * count + b'";'
            reasons = audit_public.content_reasons("src/control.c", data, None)
            self.assertEqual(any("suspicious C literal" in reason for reason in reasons), count > audit_public.MAX_NON_TEXT_ESCAPES)

    def test_numeric_runs_include_comments_labels_decimal_and_dollar_hex(self):
        raw = self.image[17:273]
        index = audit_public.RomIndex([self.image])
        for notation in ("hex", "decimal", "dollar", "bare"):
            def spell(byte):
                return (f"0x{byte:02x}" if notation == "hex" else f"${byte:02x}" if notation == "dollar"
                        else str(byte) if notation == "decimal" else f"{byte:02x}").encode()
            rows = []
            for n in range(0, len(raw), 8):
                rows.append(f"{0x80000+n:08x}: ".encode() + b", ".join(spell(byte) for byte in raw[n:n + 8])
                            + f", // row {n//8} offset 0x{0x80000+n:06x}".encode())
            source = b"/* Saved experiment\nconst unsigned char data[] = {\n" + b"\n".join(rows) + b"\n};\n*/\n"
            with self.subTest(notation=notation):
                variants = [value for _, candidates in audit_public.numeric_blocks(source, audit_public.DecodeBudget()) for value in candidates]
                self.assertIn(raw, variants)
                self.assertIn("ROM window", " ".join(audit_public.content_reasons("src/disabled.c", source, index)))
                self.assertIn("numeric run", " ".join(audit_public.content_reasons("src/disabled.c", source, None)))
        small = b"\n".join(f"row_{n}:".encode() + b"\n// " + b", ".join(f"0x{byte:02x}".encode() for byte in raw[n:n + 8])
                            for n in (0, 8))
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("notes.txt", small, index)))

    def test_long_numeric_blocks_are_rom_free_policy_not_metadata_fields(self):
        self.assertNotIn("NUMERIC_ALLOWLIST", vars(audit_public))
        single_values = b"\n".join(b"// ff" for _ in range(audit_public.MAX_NUMERIC_RUN))
        self.assertIn("numeric run", " ".join(audit_public.content_reasons("notes.txt", single_values, None)))
        padded = b", ".join(f"0x{value:04x}".encode() for value in range(audit_public.MAX_NUMERIC_RUN))
        self.assertIn("numeric run", " ".join(audit_public.content_reasons("notes.txt", padded, None)))
        for size in (audit_public.MAX_NUMERIC_RUN - 1, audit_public.MAX_NUMERIC_RUN):
            data = b"\n".join(b"// " + b", ".join(str(value).encode() for value in range(n, min(n + 8, size)))
                              for n in range(0, size, 8))
            self.assertEqual(any("numeric run" in reason for reason in audit_public.content_reasons("notes.txt", data, None)),
                             size >= audit_public.MAX_NUMERIC_RUN)
        fields = {f"field{n}": n % 256 for n in range(300)}
        self.assertEqual(audit_public.content_reasons("config/facts.json", json.dumps(fields, indent=2).encode(), None), [])
        first = b", ".join(str(n).encode() for n in range(32))
        self.assertIn("numeric run", " ".join(audit_public.content_reasons("notes.txt", first + b"\n\n" + first, None)))

    def test_embedded_binary_signatures_across_all_decoders_without_roms(self):
        prefixes = {
            "PNG": b"\x89PNG\r\n\x1a\n", "GIF": b"GIF89a", "JPEG": b"\xff\xd8\xff",
            "RIFF": b"RIFF", "ZIP": b"PK\x03\x04", "7z": b"7z\xbc\xaf'\x1c",
            "xz": b"\xfd7zXZ\0", "bzip2": b"BZh9", "ELF": b"\x7fELF", "Ogg": b"OggS",
            "FLAC": b"fLaC", "MIDI": b"MThd", "zstd": b"\x28\xb5\x2f\xfd",
        }
        payloads = {name: prefix + bytes(range(32)) for name, prefix in prefixes.items()}
        payloads["zlib"] = zlib.compress(b"synthetic object data" * 16)
        payloads["empty-zlib"] = zlib.compress(b"")
        payloads["gzip"] = gzip.compress(b"synthetic data", mtime=0)
        payloads["actual-xz"] = lzma.compress(b"synthetic data")
        payloads["actual-bz2"] = bz2.compress(b"synthetic data")
        payloads["BMP"] = b"BM" + struct.pack("<IHHI", 54, 0, 0, 54) + bytes(40)
        processor, module = b"M68000", b"demo"
        payloads["IEEE"] = bytes([0xE0, len(processor)]) + processor + bytes([len(module)]) + module + bytes([0xEC, 8, 4, 0xCC, 0xE1])
        payloads["IEEE-anonymous"] = bytes([0xE0, len(processor)]) + processor + bytes([0, 0xEC, 8, 4, 0xCC, 0xE1])
        for name, raw in payloads.items():
            forms = [
                json.dumps({"data": base64.b64encode(raw).decode()}).encode(),
                json.dumps({"uri": "data:application/octet-stream;base64," + base64.b64encode(raw).decode()}).encode(),
                json.dumps({"uri": "data:application/octet-stream," + quote_from_bytes(raw)}).encode(),
                raw.hex().encode(), b"value = " + repr(raw).encode(),
                b"unsigned char data[]={" + b", ".join(f"0x{byte:02x}".encode() for byte in raw) + b"};",
            ]
            for form in forms:
                with self.subTest(kind=name, form=form[:20]):
                    reasons = audit_public.content_reasons("payload.txt", form, None)
                    self.assertIn("embedded", " ".join(reasons))
        for prefix in prefixes.values():
            self.assertEqual(audit_public.content_reasons("signature.py", b"MAGIC = " + repr(prefix).encode(), None), [])

    def test_valid_png_data_uri_and_encoded_c_text_are_media_policy(self):
        def chunk(kind, body):
            return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
        signature = bytes([137]) + b"PNG" + bytes([13, 10, 26, 10])
        png = signature + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
        png += chunk(b"IDAT", zlib.compress(bytes(4))) + chunk(b"IEND", b"")
        uri = "data:image/png;base64," + base64.b64encode(png).decode()
        for path, source in (("config/image.json", json.dumps({"image": uri}).encode()),
                             ("src/image.c", b"const char *image = " + json.dumps(uri).encode() + b";")):
            self.assertIn("embedded PNG", " ".join(audit_public.content_reasons(path, source, None)))

    def test_ci_command_needs_no_rom_or_third_party_python_packages(self):
        script = self.root / "tools/audit_public.py"
        script.parent.mkdir()
        script.write_bytes((Path(__file__).resolve().parents[1] / "tools/audit_public.py").read_bytes())
        self.git("add", "tools/audit_public.py", "config/title.json")
        self.git("commit", "-qm", "install standalone synthetic audit")
        for version in ("rev02", "rev00"):
            (self.root / f"orig/{version}/baserom.bin").unlink()
        env = {**os.environ, "PYTHONPATH": "", "PYTHONNOUSERSITE": "1", "PYTHONDONTWRITEBYTECODE": "1"}
        result = subprocess.run([sys.executable, "-S", "tools/audit_public.py", "--history", "--no-rom"],
                                cwd=self.root, env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("ROM comparison SKIPPED", result.stdout)

    def test_revision_exclusive_binary_c_literal_uses_the_second_rom(self):
        if not all((ROOT / f"orig/{version}/baserom.bin").is_file() for version in ("rev02", "rev00")):
            self.skipTest("private reference ROMs absent")
        primary = (ROOT / "orig/rev02/baserom.bin").read_bytes()
        secondary = (ROOT / "orig/rev00/baserom.bin").read_bytes()
        raw = secondary[0x478:0x488]
        source = b'const char data[]="' + b"".join(f"\\{byte:03o}".encode() for byte in raw) + b'";'
        index02, index00 = audit_public.RomIndex([primary]), audit_public.RomIndex([secondary])
        self.assertTrue(all(raw[n:n + 12] not in index02.windows[12] for n in range(5)))
        self.assertFalse(any("ROM window" in reason for reason in audit_public.content_reasons("src/exclusive.c", source, index02)))
        self.assertTrue(any("ROM window" in reason for reason in audit_public.content_reasons("src/exclusive.c", source, index00)))

    def normalization_cases(self, raw):
        encoded = base64.b64encode(raw)
        return {
            "octal": ("src/octal.c", b"unsigned char data[]={" + b", ".join(f"0{byte:03o}".encode() for byte in raw) + b"};\n"),
            "signed-c": ("src/signed.c", b"char data[]={" + b", ".join(str(byte if byte < 128 else byte - 256).encode() for byte in raw) + b"};\n"),
            "signed-json": ("config/samples.json", json.dumps({"samples": [byte if byte < 128 else byte - 256 for byte in raw]}).encode()),
            "characters": ("src/characters.c", b"unsigned char data[]={" + b", ".join(f"'\\{byte:03o}'".encode() for byte in raw) + b"};\n"),
            "wrapped-base64": ("tests/frame.yml", b"expected: |\n" + b"\n".join(b"  " + encoded[n:n + 8] for n in range(0, len(encoded), 8)) + b"\n"),
        }

    def assert_normalized_window(self, kind):
        raw = self.image[17:33]
        path, source = self.normalization_cases(raw)[kind]
        self.stage(path, source)
        reasons, _ = self.reasons()
        self.assertIn(path, reasons)
        self.assertIn("ROM window", reasons)
        if kind == "wrapped-base64":
            self.assertIn(base64.b64encode(raw), audit_public.base64_candidates(source))
        else:
            variants = [value for _, candidates in audit_public.numeric_blocks(source, audit_public.DecodeBudget()) for value in candidates]
            self.assertIn(raw, variants)

    def test_c_octal_initializer_window(self):
        self.assert_normalized_window("octal")

    def test_c_signed_decimal_initializer_window(self):
        self.assert_normalized_window("signed-c")

    def test_json_signed_decimal_window(self):
        self.assert_normalized_window("signed-json")

    def test_c_escaped_character_initializer_window(self):
        self.assert_normalized_window("characters")

    def test_yaml_wrapped_base64_window(self):
        self.assert_normalized_window("wrapped-base64")

    def test_byte_atom_decoder_covers_all_values_bases_signs_and_escapes(self):
        for byte in range(256):
            signed = byte if byte < 128 else byte - 256
            forms = [str(byte), f"+{byte}U", str(signed), f"0x{byte:02x}", f"0x_{byte:02X}",
                     f"${byte:02x}", f"0o{byte:o}", f"0{byte:03o}", f"0b{byte:08b}",
                     f"'\\{byte:03o}'", f"'\\x{byte:02x}'"]
            for form in forms:
                with self.subTest(byte=byte, form=form):
                    token = form.encode()
                    self.assertIsNotNone(audit_public.NUMBER.fullmatch(token))
                    values, _ = audit_public.literal_values(token)
                    self.assertIsNotNone(values[0])
                    self.assertTrue(-128 <= values[0] <= 255)
                    self.assertEqual(values[0] & 255, byte)
        fields = audit_public.numeric_fields(b"-129, 256, -256, 0x100, -1, +255")
        self.assertEqual(fields[0], bytes([255, 255]))
        for token, expected in ((b"'x'", 120), (b"'\\n'", 10), (b"'\\t'", 9), (b"-'x'", 136)):
            self.assertEqual(audit_public.literal_values(token)[0][0] & 255, expected)
        self.assertEqual(audit_public.literal_values(b"'\\x{value}'")[0], (None, None, None, None))

    def test_audit_regression_source_is_not_embedded_material(self):
        source = Path(__file__).read_bytes()
        self.assertEqual(audit_public.content_reasons("tests/test_audit_public.py", source, None), [])

    def test_base64_whitespace_normalization_is_container_independent(self):
        raw = self.image[17:33]
        encoded = base64.b64encode(raw)
        index = audit_public.RomIndex([self.image])
        for width in (4, 7, 8, 11, 76):
            lines = [encoded[n:n + width] for n in range(0, len(encoded), width)]
            wrapped = b"\n".join(b"    " + line for line in lines)
            fixtures = [b"expected: >-\n" + wrapped, b"expected: |+\n" + wrapped,
                        b"expected: !!binary |-\n" + wrapped, b"expected: |2-\n" + wrapped,
                        b"```base64\n" + wrapped + b"\n```",
                        b"> ```base64\n" + b"\n".join(b"> " + line for line in lines) + b"\n> ```",
                        b"expected: " + lines[0] + b"\n" + b"\n".join(b"  " + line for line in lines[1:]),
                        b"data:application/octet-stream;base64," + lines[0] + b"\n" + b"\n".join(b"  " + line for line in lines[1:]),
                        b" ".join(lines)]
            for source in fixtures:
                with self.subTest(width=width, source=source[:25]):
                    self.assertIn(encoded, audit_public.base64_candidates(source))
                    self.assertIn("ROM window", " ".join(audit_public.content_reasons("fixture.txt", source, index)))
        scalars = b"\n".join(f"field_{n}: {n % 256}".encode() for n in range(300))
        self.assertEqual(audit_public.content_reasons("config/facts.yml", scalars, None), [])

    def test_five_normalizations_match_a_real_secondary_only_window(self):
        if not all((ROOT / f"orig/{version}/baserom.bin").is_file() for version in ("rev02", "rev00")):
            self.skipTest("private reference ROMs absent")
        primary = (ROOT / "orig/rev02/baserom.bin").read_bytes()
        secondary = (ROOT / "orig/rev00/baserom.bin").read_bytes()
        raw = secondary[0x126C:0x127C]
        index02 = audit_public.RomIndex([primary])
        index00 = audit_public.RomIndex([secondary])
        self.assertTrue(all(raw[n:n + 12] not in index02.windows[12] for n in range(5)))
        for kind, (path, source) in self.normalization_cases(raw).items():
            with self.subTest(kind=kind):
                self.assertTrue(any("ROM window" in reason for reason in audit_public.content_reasons(path, source, index00)))
                self.assertFalse(any("ROM window" in reason for reason in audit_public.content_reasons(path, source, index02)))

    def test_opaque_entropy_mixing_length_and_standing_hash_boundaries(self):
        encoded = base64.b64encode(hashlib.shake_256(b"opaque length boundary").digest(36))
        for size, value in ((47, encoded[:47]), (48, encoded), (49, encoded + encoded[:1])):
            reasons = audit_public.content_reasons("payload.txt", value, None)
            self.assertEqual(any("opaque" in reason for reason in reasons), size >= 48)
        for size in (40, 64):
            digest = (b"0123456789abcdef" * 4)[:size]
            for source in (digest, b"digest: " + digest, json.dumps({"sha": digest.decode()}).encode()):
                self.assertIsNone(audit_public.opaque_reason("facts.txt", source))
            self.assertFalse(audit_public.structured_text_view(digest).strip())
        balanced_hex = b"0123456789abcdef" * 3
        self.assertAlmostEqual(audit_public.opaque_metrics(balanced_hex)[0], 4.0)
        self.assertIn("opaque", audit_public.opaque_reason("payload.txt", balanced_hex))
        self.assertIn("opaque", audit_public.opaque_reason("payload.txt", b"prefix" + b"0123456789abcdef" * 4))
        for value in (b"g" * 128, b"x" * 1_000_000, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ" * 4):
            self.assertIsNone(audit_public.opaque_reason("padding.txt", value))
        plain = b"an independently authored fixture with several ordinary short words"
        for encoded in (base64.b32encode(plain), base64.b85encode(plain), quopri.encodestring(plain, quotetabs=True)):
            self.assertTrue(audit_public.content_reasons("encoded.txt", encoded, None))
        hash_like = b"0b0cc" + b"1" * 59
        self.assertIsNone(audit_public.opaque_reason("facts.txt", hash_like))
        self.assertEqual(audit_public.numeric_fields(hash_like)[0], b"")

    def test_structured_facts_and_source_syntax_are_not_opaque_payloads(self):
        references = [
            b"https://github.com/mschienbein/toejam-earl-decomp/actions/workflows/progress.yml",
            b"tje-disasm:include/variables.h:gMusicChannel6Ptr2",
            b"tje-disasm:src/main.S:.YM2612UpdateFrequency$WaitForYM2612Ready_02",
            b"derived:code-referenced-metasprite-pointer-graph",
            b"(`/manage/mschienbein/toejam-earl-decomp?added=1`).",
            b".cache/toolchain/binutils-2.47-m68k-elf/bin/m68k-elf-objdump",
            b"Sub-boundaries98024,98044,98064,98070,98074,98080,9808C;",
        ]
        for text in references:
            self.assertIsNone(audit_public.opaque_reason("facts.txt", text))
        source = b"def " + b"long_identifier_" * 6 + b"():\n    pass\n"
        source += b"\n# " + b"-" * 90 + b"\n"
        pattern = b"^B(?:" + b"|".join(b"RA SR HI LS CC HS CS LO NE EQ VC VS PL MI GE LT GT LE".split()) + b")$"
        source += b'import re\nBRANCH = re.compile(' + repr(pattern).encode() + b")\n"
        self.assertIsNone(audit_public.opaque_reason("syntax.py", source))
        source += b"CONDITION = " + repr(pattern).encode() + b"\nALIAS = CONDITION\nMATCH = re.compile(b'^' + ALIAS)\n"
        self.assertIsNone(audit_public.opaque_reason("syntax.py", source))
        addresses = {f"target{n}": f"0x{0x30000 + n * 16:06X}" for n in range(16)}
        self.assertIsNone(audit_public.opaque_reason("facts.json", json.dumps(addresses).encode()))
        payload_source = b'VALUE = "' + base64.b64encode(self.image[17:113]) + b'"\n'
        self.assertIn("opaque", audit_public.opaque_reason("syntax.py", payload_source))

    def test_historical_regex_exceptions_are_exact_spans_only(self):
        spans = (
            b're.compile(rb"' + audit_public.DATA_URI.pattern + b'",',
            b're.compile(rb"' + audit_public.MARKER.pattern.split(b"{6,8}")[0],
        )
        self.assertEqual({hashlib.sha256(value).hexdigest() for value in spans},
                         audit_public.LEGACY_OPAQUE_SHA256)
        secret = base64.b64encode(self.image[17:113])
        for value in spans:
            self.assertIsNone(audit_public.opaque_reason("unrelated.txt", value))
            self.assertIn("opaque", audit_public.opaque_reason("unrelated.txt", value + b"1"))
            self.assertIn("opaque", audit_public.opaque_reason("unrelated.txt", value + b"\n" + secret))
            raw = value + b" readable fixture continuation " * 4
            index = audit_public.RomIndex([raw])
            self.assertIn("ROM window", " ".join(audit_public.content_reasons("unrelated.txt", raw, index)))

    def test_low_entropy_instruction_literals_still_receive_rom_checks(self):
        programs = (
            (0x0C40, 0x0002, 0x6E1C, 0xD040, 0x303B, 0x0006, 0x4EFB, 0x0002, 0x0006,
             0x000A, 0x000E, 0x7001, 0x4E75, 0x7002, 0x4E75, 0x70FF, 0x4E75, 0x7000, 0x4E75),
            (0x4E56, 0xFFF8, 0x48E7, 0x3000, 0x302E, 0x0008, 0x222F, 0x0018,
             0x4CDF, 0x000C, 0x4E5E, 0x4E75),
        )
        for words in programs:
            raw = b"".join(word.to_bytes(2, "big") for word in words)
            encoded = raw.hex().encode()
            self.assertIsNone(audit_public.opaque_reason("fixture.txt", encoded))
            self.assertLess(audit_public.opaque_metrics(encoded)[0], 4.0)
            self.assertIsNone(audit_public.opaque_reason("fixture.txt", encoded + b"1"))
            index = audit_public.RomIndex([raw + self.image])
            self.assertIn("ROM window", " ".join(audit_public.content_reasons("fixture.txt", encoded, index)))

    def test_long_independent_numeric_data_is_intentionally_forbidden(self):
        vector = hashlib.shake_256(b"independent synthetic fixture").digest(80)
        source = b"EXPECTED = [\n" + b", ".join(str(value).encode() for value in vector) + b"\n]\n"
        self.assertIn("numeric", " ".join(audit_public.content_reasons("tests/vector.py", source, None)))
        calibration = b"sample\n" + b"".join(f"{n % 47}\n".encode() for n in range(80))
        self.assertIn("numeric", " ".join(audit_public.content_reasons("tests/calibration.csv", calibration, None)))
        generated = b"import hashlib\nEXPECTED = hashlib.shake_256(b'independent fixture').digest(80)\n"
        self.assertEqual(audit_public.content_reasons("tests/generated.py", generated, None), [])

    def format_cases(self, raw):
        def intel_records(width):
            rows = []
            for start in range(0, len(raw), width):
                payload = raw[start:start + width]
                record = bytes([len(payload)]) + start.to_bytes(2, "big") + bytes([0]) + payload
                rows.append(b":" + (record + bytes([-sum(record) & 255])).hex().upper().encode())
            return b"\n".join(rows)
        srows = []
        for start in range(0, len(raw), 16):
            payload = raw[start:start + 16]
            record = bytes([len(payload) + 3]) + start.to_bytes(2, "big") + payload
            srows.append(b"S1" + (record + bytes([~sum(record) & 255])).hex().upper().encode())
        uu = b"begin 644 captured.dat\n" + b"".join(binascii.b2a_uu(raw[n:n + 45]) for n in range(0, len(raw), 45)) + b"`\nend\n"
        z85 = bytearray()
        alphabet = audit_public.OPAQUE_ALPHABETS["Z85"]
        for start in range(0, len(raw), 4):
            value = int.from_bytes(raw[start:start + 4], "big")
            digits = bytearray(5)
            for index in range(4, -1, -1):
                value, digit = divmod(value, 85)
                digits[index] = alphabet[digit]
            z85.extend(digits)
        return {
            "rust": f"const FRAME: [u8; {len(raw)}] = [".encode() + b", ".join(f"{value}u8".encode() for value in raw) + b"];\n",
            "javascript": b"const frame = new Uint8Array([" + b", ".join(str(value).encode() for value in raw) + b"]);\n",
            "go": b"package fixture\nvar Frame = []byte{" + b", ".join(str(value).encode() for value in raw) + b"}\n",
            "tsv": b"address\tvalue\n" + b"".join(f"{0x277a+n:06x}\t{value}\n".encode() for n, value in enumerate(raw)),
            "csv": b"index,value\n" + b"".join(f"{n},{value}\n".encode() for n, value in enumerate(raw)),
            "od": b"\n".join(f"{n:07o} ".encode() + b" ".join(f"{value:03o}".encode() for value in raw[n:n + 16]) for n in range(0, len(raw), 16)),
            "intel": intel_records(8), "intel16": intel_records(16), "srecord": b"\n".join(srows), "uu": uu,
            "qp": quopri.encodestring(raw, quotetabs=True),
            "base32": base64.b32encode(raw), "base85": base64.b85encode(raw), "z85": bytes(z85),
            "ascii85": base64.a85encode(raw, adobe=True, wrapcol=60),
            "chunks": json.dumps({f"part{n//8}": raw[n:n+8].hex() for n in range(0, len(raw), 8)}).encode(),
        }

    def test_all_conventional_reproductions_reject_small_and_long_windows(self):
        index = audit_public.RomIndex([self.image])
        for size in (16, 32, 96):
            raw = self.image[17:17 + size]
            for name, source in self.format_cases(raw).items():
                path = "tests/payload.json" if name == "chunks" else "tests/payload.txt"
                with self.subTest(size=size, format=name):
                    self.assertTrue(audit_public.content_reasons(path, source, index))
                    # Record shapes/long opaque runs are independent of the reference ROM.
                    if name in ("intel", "intel16", "srecord") or size == 96 and name in ("uu", "base32", "base85", "z85", "ascii85", "qp"):
                        self.assertTrue(audit_public.content_reasons(path, source, None))

    def test_short_numeric_wrappers_are_decoded_not_hidden_by_thresholds(self):
        raw = self.image[17:33]
        index = audit_public.RomIndex([self.image])
        for name, source in self.format_cases(raw).items():
            if name not in ("rust", "javascript", "go", "tsv", "csv", "od", "base32", "base85", "z85", "ascii85"):
                continue
            with self.subTest(format=name):
                reasons = audit_public.content_reasons("tests/data.txt", source, index)
                self.assertIn("ROM window", " ".join(reasons))

    def test_record_shapes_and_numbered_chunks_are_structural(self):
        raw = self.image[17:113]
        cases = self.format_cases(raw)
        for name in ("intel", "srecord", "uu", "qp"):
            with self.subTest(format=name):
                self.assertIsNotNone(audit_public.opaque_reason("records.txt", cases[name]))
        parts = json.dumps({f"part{n}": self.image[17 + 8*n:25 + 8*n].hex() for n in range(4)}).encode()
        index = audit_public.RomIndex([self.image])
        self.assertIn("ROM window", " ".join(audit_public.content_reasons("parts.json", parts, index)))
        hashes = json.dumps({f"sha{n}": hashlib.sha256(str(n).encode()).hexdigest() for n in range(4)}).encode()
        self.assertIsNone(audit_public.opaque_reason("hashes.json", hashes))

    def lexical_cases(self, raw):
        numbers = lambda value: b", ".join(str(byte).encode() for byte in value)
        small, long = raw[:32], raw[:80]
        encoded = base64.b85encode(raw)
        return (
            ("tests/single.js", b"const expected = '" + encoded + b"';\n", True),
            ("tests/template.js", b"const expected = `" + encoded + b"`;\n", True),
            ("tests/raw.go", b"package fixture\nvar expected = `" + base64.b64encode(raw) + b"`\n", True),
            ("tests/tuple.py", b"EXPECTED = (" + numbers(small) + b")\n", False),
            ("tests/tuple_long.py", b"EXPECTED = (" + numbers(long) + b")\n", True),
            ("tests/return.py", b"def expected():\n    return bytes([" + numbers(long) + b"])\n", True),
            ("tests/assert.py", b"self.assertEqual(list(self.frame), [" + numbers(long) + b"])\n", True),
            ("src/compound.c", b"void f() { consume((unsigned char[]){" + numbers(long) + b"}, 80); }\n", True),
            ("tests/braces.cpp", b"std::array<unsigned char, 80> expected{{" + numbers(long) + b"}};\n", True),
            ("tests/fixture.ini", b"[test]\nexpected = " + numbers(small) + b"\n", False),
        )

    def test_ar6_quoting_and_numeric_reproductions_are_path_independent(self):
        raw = self.image[17:113]
        index = audit_public.RomIndex([self.image])
        for path, source, structural in self.lexical_cases(raw):
            for spelling in (path, "artifact.txt"):
                with self.subTest(path=spelling, shape=path):
                    self.assertTrue(audit_public.content_reasons(spelling, source, index))
                    self.assertEqual(bool(audit_public.content_reasons(spelling, source, None)), structural)

    def test_ar6_shapes_with_private_retail_bytes(self):
        references = [ROOT / f"orig/{version}/baserom.bin" for version in ("rev02", "rev00")]
        if not all(path.is_file() for path in references):
            self.skipTest("private reference ROMs absent")
        images = [path.read_bytes() for path in references]
        index = audit_public.RomIndex(images)
        for path, source, structural in self.lexical_cases(images[1][0x17600:0x17660]):
            with self.subTest(path=path):
                self.assertTrue(audit_public.content_reasons(path, source, index))
                self.assertEqual(bool(audit_public.content_reasons(path, source, None)), structural)

    def test_numeric_sequences_are_context_free_and_do_not_mask_string_contents(self):
        raw = self.image[17:97]
        sequence = b", ".join(str(value).encode() for value in raw)
        wrappers = (
            (b"VALUE = '", b"'"), (b'VALUE = "', b'"'), (b"VALUE = `", b"`"),
            (b"VALUE = r'''", b"'''"), (b"values = (", b")"),
            (b"key: [", b"]"), (b"[section]\nkey=", b"\n"),
            (b"# stored values = ", b"\n"), (b"/* stored values = (", b") */"),
            (b"f(", b")"), (b"return {", b"}"),
        )
        for prefix, suffix in wrappers:
            with self.subTest(prefix=prefix):
                self.assertIn("numeric", " ".join(audit_public.content_reasons("artifact.txt", prefix + sequence + suffix, None)))
        values = b", ".join(str(n).encode() for n in range(32))
        self.assertIn("numeric", " ".join(audit_public.content_reasons("artifact.txt", values + b"\n\n" + values, None)))
        facts = {f"field_{n}": n % 97 for n in range(256)}
        self.assertEqual(audit_public.content_reasons("artifact.txt", json.dumps(facts).encode(), None), [])

    def test_adjacent_text_literals_do_not_invent_an_opaque_source_run(self):
        value = bytes(range(33, 118))
        self.assertIsNotNone(audit_public.opaque_reason("joined.txt", value))
        pieces = [json.dumps(value[n:n + 32].decode()).encode() for n in range(0, len(value), 32)]
        source = b"char *message = " + b" /* continuation */ ".join(pieces) + b";\n"
        index = audit_public.RomIndex([value])
        self.assertEqual(audit_public.content_reasons("src/message.c", source, index), [])

    def test_json_name_chunks_are_decoding_candidates_not_literal_opaque_runs(self):
        values = []
        for n in range(8):
            middle = "".join(chr(65 + byte % 26) for byte in hashlib.shake_256(str(n).encode()).digest(30))
            values.append("z" + middle + "z")
        joined = "".join(values).encode()
        self.assertIsNotNone(audit_public.opaque_reason("joined.txt", joined))
        source = json.dumps({"aliases": values}).encode()
        self.assertEqual(audit_public.content_reasons("config/names.json", source, None), [])

    def test_reference_exceptions_do_not_hide_high_entropy_components(self):
        secret = base64.b64encode(self.image[17:113])
        for source in (
                b"https://example.invalid/file?payload=" + secret,
                b"https://example.invalid/data/" + secret,
                b"source:src/main.c:" + secret,
                b'{"name":"' + secret + b'"}'):
            with self.subTest(source=source[:40]):
                self.assertIn("opaque", audit_public.opaque_reason("facts.txt", source))
        digest = b"0123456789abcdef" * 4
        package = b"https://files.pythonhosted.org/packages/" + digest[:2] + b"/" + digest[2:4] + b"/" + digest[4:] + b"/package.whl"
        self.assertIsNone(audit_public.opaque_reason("uv.lock", package))
        self.assertIsNone(audit_public.opaque_reason("facts.txt", b"https://example.invalid/" + digest + b"/file"))

    def test_a_reviewed_exception_covers_one_exact_blob(self):
        run = b"x = " + base64.b85encode(hashlib.sha256(b"reviewed").digest() * 2)[:56] + b"\n"
        self.stage("src/flagged.c", run)
        self.assertIn("opaque", self.reasons(no_rom=True)[0])
        oid = self.git("rev-parse", ":src/flagged.c").decode().strip()
        exception = {(oid, "src/flagged.c"): ("opaque",)}
        with mock.patch.object(audit_public, "REVIEWED_FALSE_POSITIVES", exception):
            self.assertNotIn("src/flagged.c", self.reasons(no_rom=True)[0])
            self.stage("src/other.c", run)                       # same blob, other path: still reported
            self.assertIn("src/other.c", self.reasons(no_rom=True)[0])
            self.stage("src/flagged.c", run + b"y = 1;\n")      # changed blob: still reported
            self.assertIn("src/flagged.c", self.reasons(no_rom=True)[0])

    def test_genesis_header_needs_more_than_sega_at_offset_256(self):
        text = b"/* " + b"x" * 237 + b" */\nextern void SEGAAndIntroVideoRoutine();\n"
        self.assertEqual(text[256:260], b"SEGA")
        self.assertIsNone(audit_public.embedded_kind(text, audit_public.DecodeBudget()))
        header = bytes(256) + b"SEGA GENESIS    (C)SEGA 1991.OCT" + bytes(64)
        self.assertEqual(audit_public.embedded_kind(header, audit_public.DecodeBudget()), "Genesis ROM")
        texty = b"x" * 256 + b"SEGA GENESIS    (C)SEGA 1991.OCT" + b"y" * 64   # text vectors, real fields
        self.assertEqual(audit_public.embedded_kind(texty, audit_public.DecodeBudget()), "Genesis ROM")
        vectors = bytes(range(256)) + b"SEGA GENESIS    "   # a 272-byte prefix: binary vectors, no copyright yet
        self.assertEqual(audit_public.embedded_kind(vectors, audit_public.DecodeBudget()), "Genesis ROM")


class PinnedObjdumpAuditTest(unittest.TestCase):
    def test_genuine_objdump_listing_of_generated_style_assembly(self):
        binutils = ROOT / ".cache/toolchain/binutils-2.47-m68k-elf/bin"
        if not (binutils / "m68k-elf-objdump").is_file():
            self.skipTest("pinned binutils not installed")
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "func_000400.s"   # synthetic, written like the generated baseline
            source.write_text('\t.section .text.func_000400,"ax"\n\t.globl func_000400\nfunc_000400:\n'
                              ".Li_000400:\tmove.w #0x1234,%d0\n.Li_000404:\tmove.l %d0,(0xff0010).l\n"
                              ".Li_00040a:\tbeq.s .Li_000400\n.Li_00040c:\trts\n")
            obj = Path(tmp) / "listing.o"
            subprocess.run([str(binutils / "m68k-elf-as"), "-m68000", "--bitwise-or", "--pcrel", "-L",
                            "-o", str(obj), str(source)], check=True, capture_output=True)
            listing = subprocess.run([str(binutils / "m68k-elf-objdump"), "-dr", str(obj)],
                                     check=True, capture_output=True).stdout
            self.assertTrue(audit_public.LISTING.search(listing), "real variable-width opcode rows")
            self.assertTrue(audit_public.OBJDUMP_HEADER.search(listing), "real symbol/section headers")
            self.assertTrue(audit_public.MARKER.search(listing), "real generated labels inside objdump")
            self.assertIn("disassembly listing", " ".join(audit_public.content_reasons("listing.txt", listing, None)))


if __name__ == "__main__":
    unittest.main()
