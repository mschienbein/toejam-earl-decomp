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
\tMOVE.W\t6(SP),D0\t*fr
\tBRA\t_L2
_L1:
\tSUBQ.W\t#1,D0
_L2:
\tTST.W\tD0
\tBNE\t_L1
\tBEQ.S\t_L3
\tJSR\t.helper
\tJSR\t.loop
\tJSR\t.later
\tMOVE.L\t#$FF800E,A0
_L3:
\tRTS
\tXDEF\t.later
.later:
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

    def test_unsized_backward_branch_takes_the_shortest_form(self):
        self.assertIn("\tjbne\t_L1", self.lines)

    def test_calls_in_the_same_file_are_pc_relative_and_forward_ones_keep_six_bytes(self):
        self.assertIn("\tJSR\t(loop,%pc)", self.lines)
        later = self.lines.index("\tJSR\t(later,%pc)")
        self.assertEqual(self.lines[later + 1], "\tnop")
        self.assertIn("\tJSR\t(helper).l", self.lines)

    def test_calls_into_the_rest_of_the_original_unit_are_pc_relative(self):
        near = mcc.adapt(LISTING, "unit", {"helper": "forward"}).splitlines()
        call = near.index("\tJSR\t(helper,%pc)")
        self.assertEqual(near[call + 1], "\tnop")
        before = mcc.adapt(LISTING, "unit", {"helper": "backward"}).splitlines()
        call = before.index("\tJSR\t(helper,%pc)")
        self.assertNotEqual(before[call + 1], "\tnop")

    def test_end_of_line_comments_are_dropped(self):
        self.assertIn("\tMOVE.W\t6(%sp),%d0", self.lines)
        self.assertEqual(mcc.split_fields("RTS\t*x"), ("RTS", ""))
        self.assertEqual(mcc.split_fields("DC.B\t'a b',0\t*text"), ("DC.B", "'a b',0"))

    def test_explicit_short_branch_is_kept(self):
        self.assertIn("\tBEQ.S\t_L3", self.lines)

    def test_registers_and_c_symbols_stay_apart(self):
        listing = "\tSECTION\t9,,C\n.f:\n\tMOVE.W\t.d0,D0\n\tMOVE.L\t.sp,A0\n\tMOVE.W\t(A0,D1.L),D2\n\tRTS\n"
        lines = mcc.adapt(listing, "u").splitlines()
        self.assertIn("\tMOVE.W\td0,%d0", lines)          # the C global d0, then the register
        self.assertIn("\tMOVE.L\tsp,%a0", lines)
        self.assertIn("\tMOVE.W\t(%a0,%d1.L),%d2", lines)

    def test_strings_keep_their_bytes(self):
        listing = "\tSECTION\t9,,C\n.f:\n\tRTS\n\tSECTION\t14,,D\n_S1:\n\tDC.B\t'it''s $1 .x',0\n"
        lines = mcc.adapt(listing, "u").splitlines()
        expected = ",".join(str(b) for b in b"it's $1 .x") + ",0"
        self.assertIn("\t.byte\t" + expected, lines)

    def test_calls_to_data_labels_stay_absolute(self):
        listing = "\tSECTION\t14,,D\n.table:\n\tDC.W\t1\n\tSECTION\t9,,C\n.f:\n\tJSR\t.table\n\tRTS\n"
        self.assertIn("\tJSR\t(table).l", mcc.adapt(listing, "u").splitlines())

    def test_branches_and_dbcc_out_of_the_file_are_refused(self):
        for statement in ("\tBRA\t.elsewhere", "\tDBRA\tD0,.elsewhere"):
            with self.assertRaises(mcc.AdaptError):
                mcc.adapt(f"\tSECTION\t9,,C\n.f:\n{statement}\n\tRTS\n", "u")

    def test_address_loads_of_local_labels_follow_the_call_rule(self):
        listing = ("\tSECTION\t9,,C\n.f:\n\tPEA\t_L2\n\tLEA\t_L2,A0\n\tPEA\t.ext\n\tRTS\n"
                   "_L2:\tDC.B\t'Hi'\n\tDC.B\t0\n\tDCB.B\t1,0\n")
        lines = mcc.adapt(listing, "u").splitlines()
        pea = lines.index("\tPEA\t(_L2,%pc)")
        self.assertEqual(lines[pea + 1], "\tnop")              # forward: 6 bytes, as ASM68K reserved
        lea = lines.index("\tLEA\t(_L2,%pc),%a0")
        self.assertEqual(lines[lea + 1], "\tnop")
        self.assertIn("\tPEA\t(ext).l", lines)                  # another file: absolute
        self.assertIn("\t.fill\t1,1,0", lines)                  # DCB.B count,value

    def test_word_and_long_data_are_aligned_like_asm68k(self):
        listing = "\tSECTION\t9,,C\n.f:\n\tRTS\n_L1:\tDC.B\t1\n_L2:\tDC.W\t2\n\tDS.L\t1\n"
        lines = mcc.adapt(listing, "u").splitlines()
        self.assertEqual(lines[lines.index("_L2:") - 1], "\t.even")   # the label moves with its data
        self.assertEqual(lines[lines.index("\t.space\t(1)*4") - 1], "\t.even")

    def test_microtec_dot_prefix_and_hex_are_respelled(self):
        self.assertIn("loop:", self.lines)
        self.assertIn("\t.globl loop", self.lines)
        self.assertIn("\tMOVE.L\t#0xFF800E,%a0", self.lines)

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
        return self.match.match_file(path, "rev02", [])

    def test_two_known_functions_match_byte_for_byte(self):
        reports = self.run_c("pair.c", "void DelayIterations(n)\nshort n;\n{\n    while (n)\n        --n;\n}\n")
        self.assertEqual([r["status"] for r in reports], ["MATCH"])
        reports = self.run_c("mod.c", "short GetLevelMusicIndex(v)\nshort v;\n{\n    return (v - 1) % 6;\n}\n")
        self.assertEqual([r["status"] for r in reports], ["MATCH"])

    def test_a_wrong_constant_is_reported_as_a_difference(self):
        reports = self.run_c("bad.c", "short GetLevelMusicIndex(v)\nshort v;\n{\n    return (v - 2) % 6;\n}\n")
        self.assertEqual(reports[0]["status"], "DIFF")
        self.assertIn("subq.l #$1", reports[0]["diff"])

    def test_names_cannot_be_moved_or_bypassed(self):
        loop = "void DelayIterations(n)\nshort n;\n{\n    while (n)\n        --n;\n}\n"
        with self.assertRaisesRegex(self.match.MatchError, "authoritative"):
            self.match.match_file(self.dir / "x.c", "rev02", [], defines={"DelayIterations": 0x1262})
        (self.dir / "spelled.c").write_text(loop.replace("DelayIterations", "func_0011ca"))
        with self.assertRaisesRegex(self.match.MatchError, "write DelayIterations"):
            self.match.match_file(self.dir / "spelled.c", "rev02", [])

    def test_a_function_at_the_wrong_place_is_refused(self):
        with self.assertRaisesRegex(self.match.MatchError, "contiguous run"):
            self.run_c("order.c", "short GetLevelMusicIndex(v)\nshort v;\n{\n    return v;\n}\n"
                                  "void DelayIterations(n)\nshort n;\n{\n    while (n) --n;\n}\n")


if __name__ == "__main__":
    unittest.main()
