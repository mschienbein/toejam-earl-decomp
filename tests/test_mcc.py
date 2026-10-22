import tempfile
import unittest
from pathlib import Path

import support  # noqa: F401  (puts tools/ on sys.path)

import mcc
import rom as romtool
import toolchain

LISTING = """*\tMicrotec MCC68K Compiler  Version 3.3K
\tOPT\tNOABSPCADD,E,CASE
UNIT\tIDNT\t
\tSECTION\t9,,C
\tXDEF\t.loop
.loop:
\tMOVE.W\t6(SP),D0
\tBRA\t_L2
_L1:
\tSUBQ.W\t#1,D0
_L2:
\tTST.W\tD0
\tBNE\t_L1
\tBEQ.S\t_L3
\tJSR\t.helper
\tMOVE.L\t#$FF800E,A0
_L3:
\tRTS
\tSECTION\t14,,D
* allocations for .loop
\tXREF\t.helper
\tEND
"""


class AdaptTest(unittest.TestCase):
    def setUp(self):
        self.lines = mcc.adapt(LISTING, "unit").splitlines()

    def test_unsized_forward_branch_gets_a_word_displacement(self):
        self.assertIn("\tBRA.w\t_L2", self.lines)

    def test_unsized_backward_branch_is_left_to_the_assembler(self):
        self.assertIn("\tBNE\t_L1", self.lines)

    def test_explicit_short_branch_is_kept(self):
        self.assertIn("\tBEQ.S\t_L3", self.lines)

    def test_microtec_dot_prefix_and_hex_are_respelled(self):
        self.assertIn("loop:", self.lines)
        self.assertIn("\t.globl loop", self.lines)
        self.assertIn("\tJSR\thelper", self.lines)
        self.assertIn("\tMOVE.L\t#0xFF800E,A0", self.lines)

    def test_sections_map_and_microtec_directives_disappear(self):
        self.assertEqual(self.lines[0], '\t.section .text.unit,"ax"')
        self.assertIn("\t.data", self.lines)
        text = "\n".join(self.lines)
        for gone in ("OPT", "IDNT", "XREF", "END", "allocations"):
            self.assertNotIn(gone, text)


def have_compiler() -> bool:
    try:
        toolchain.tool("m68k-elf", "as")
    except toolchain.ToolchainError:
        return False
    return toolchain.emu2_path().is_file() and (toolchain.mcc68k_dir() / "MCC68K" / "MCC68K.EXE").is_file()


@unittest.skipUnless(have_compiler(), "emu2/MCC68K not provisioned (python tools/setup.py)")
class MatchTest(unittest.TestCase):
    """Compiles C with the identified compiler and checks it against the private ROM."""

    def setUp(self):
        try:
            romtool.load("rev02")
        except romtool.RomError as error:
            self.skipTest(str(error))
        import match
        self.match = match
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.dir = Path(tmp.name)

    def run_c(self, name: str, text: str) -> list[dict]:
        path = self.dir / name
        path.write_text(text)
        return self.match.match_file(path, "rev02", [], defines={"lmodt": 0x027154})

    def test_two_known_functions_match_byte_for_byte(self):
        reports = self.run_c("pair.c", "void func_0011ca(n)\nshort n;\n{\n    while (n)\n        --n;\n}\n")
        self.assertEqual([r["status"] for r in reports], ["MATCH"])
        reports = self.run_c("mod.c", "short func_001262(v)\nshort v;\n{\n    return (v - 1) % 6;\n}\n")
        self.assertEqual([r["status"] for r in reports], ["MATCH"])

    def test_a_wrong_constant_is_reported_as_a_difference(self):
        reports = self.run_c("bad.c", "short func_001262(v)\nshort v;\n{\n    return (v - 2) % 6;\n}\n")
        self.assertEqual(reports[0]["status"], "DIFF")
        self.assertIn("subq.l #$1", reports[0]["diff"])

    def test_a_function_at_the_wrong_place_is_refused(self):
        with self.assertRaisesRegex(self.match.MatchError, "contiguous run"):
            self.run_c("order.c", "short func_001262(v)\nshort v;\n{\n    return v;\n}\n"
                                  "void func_0011ca(n)\nshort n;\n{\n    while (n) --n;\n}\n")


if __name__ == "__main__":
    unittest.main()
