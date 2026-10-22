import json
import tempfile
import unittest
from pathlib import Path

import support  # noqa: F401  (puts tools/ on sys.path)

import denominator


class DenominatorTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        for version in denominator.VERSIONS:
            folder = self.root / "config" / version
            folder.mkdir(parents=True)
            catalogue = {"functions": [{"address": 0x200, "size": 4, "kind": "function"}],
                         "stats": {"functions": 1, "fragments": 0, "code_bytes": 4}}
            (folder / "catalogue.json").write_text(json.dumps(catalogue))
        self.history = self.root / "docs" / "denominator-history.jsonl"

    def test_recorded_revision_verifies_until_a_file_changes(self):
        denominator.record("first", self.root, self.history)
        self.assertEqual(len(denominator.verify(self.root, self.history)), 1)
        (self.root / "config/rev02/classification.json").write_text("{}")
        with self.assertRaisesRegex(SystemExit, "rev02/classification.json changed"):
            denominator.verify(self.root, self.history)
        denominator.record("classification added", self.root, self.history)
        self.assertEqual(len(denominator.verify(self.root, self.history)), 2)

    def test_edited_history_breaks_the_chain(self):
        denominator.record("first", self.root, self.history)
        denominator.record("second", self.root, self.history)
        lines = self.history.read_text().splitlines()
        first = json.loads(lines[0])
        first["note"] = "rewritten"
        self.history.write_text(json.dumps(first) + "\n" + lines[1] + "\n")
        with self.assertRaisesRegex(SystemExit, "history broken at revision 1"):
            denominator.verify(self.root, self.history)

    def test_record_refuses_to_extend_a_damaged_history(self):
        denominator.record("first", self.root, self.history)
        entry = json.loads(self.history.read_text())
        entry["note"] = "rewritten"
        self.history.write_text(json.dumps(entry) + "\n")
        with self.assertRaisesRegex(SystemExit, "history broken at revision 1"):
            denominator.record("second", self.root, self.history)

    def test_dropping_a_tail_revision_is_caught_against_an_earlier_copy(self):
        denominator.record("first", self.root, self.history)
        denominator.record("same files again", self.root, self.history)
        earlier = self.root / "base.jsonl"
        earlier.write_text(self.history.read_text())
        self.history.write_text(self.history.read_text().splitlines()[0] + "\n")
        self.assertEqual(len(denominator.verify(self.root, self.history)), 1)  # the chain alone cannot tell
        with self.assertRaisesRegex(SystemExit, "does not extend"):
            denominator.verify(self.root, self.history, base=earlier)


if __name__ == "__main__":
    unittest.main()
