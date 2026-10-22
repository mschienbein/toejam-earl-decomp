"""config/sources.json validation and Z80 source registration (tools/cunits.py, tools/build.py)."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build  # noqa: E402
import cunits  # noqa: E402


def registry(tmp: Path, units: list[dict]) -> Path:
    path = tmp / "sources.json"
    path.write_text(json.dumps({"schema_version": 1, "units": units}))
    return path


class SourcesTest(unittest.TestCase):
    def load(self, units):
        with tempfile.TemporaryDirectory() as tmp:
            return cunits.load_sources("rev02", registry(Path(tmp), units))

    def test_a_source_listed_twice_is_refused(self):
        entry = {"source": "src/core/a.c", "versions": ["rev02"]}
        with self.assertRaisesRegex(cunits.CUnitError, "more than once"):
            self.load([entry, dict(entry, versions=["rev00"])])

    def test_two_sources_with_one_object_name_are_refused(self):
        with self.assertRaisesRegex(cunits.CUnitError, "share a file name"):
            self.load([{"source": "src/ui/hud.c", "versions": ["rev02"]},
                       {"source": "src/map/hud.c", "versions": ["rev02"]}])

    def test_c_and_assembly_with_one_stem_do_not_collide(self):
        loaded = self.load([{"source": "src/sound/sound.c", "versions": ["rev02"]},
                            {"source": "src/asm/sound.s", "versions": ["rev02"]}])
        self.assertEqual(len(loaded), 2)

    def test_kinds_are_checked(self):
        with self.assertRaisesRegex(cunits.CUnitError, "kind"):
            self.load([{"source": "src/z80/driver.s", "kind": "z80", "versions": ["rev02"]}])   # no unit
        with self.assertRaisesRegex(cunits.CUnitError, "kind"):
            self.load([{"source": "src/x.c", "kind": "asm", "versions": ["rev02"]}])
        with self.assertRaisesRegex(cunits.CUnitError, ".c or .s"):
            self.load([{"source": "src/x.cpp", "versions": ["rev02"]}])


class Z80SourcesTest(unittest.TestCase):
    def test_one_unit_cannot_be_replaced_twice(self):
        data = {"units": [(0x2C6, 38, "InitialZ80Bootstrap", Path("unused.bin"))]}
        entries = [{"source": "src/z80/a.s", "kind": "z80", "unit": "InitialZ80Bootstrap", "versions": ["rev02"]},
                   {"source": "src/z80/b.s", "kind": "z80", "unit": "InitialZ80Bootstrap", "versions": ["rev02"]}]
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(build.BuildError, "more than once"):
                build.z80_sources("rev02", data, Path(tmp), entries)


if __name__ == "__main__":
    unittest.main()
