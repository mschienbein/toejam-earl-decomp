#!/usr/bin/env python3
"""Write an objdiff-format progress report (report.json) for ByteMatch (bytematch.dev).

    python3 ci/progress_report.py --version rev02 --output report.json [--summary $GITHUB_STEP_SUMMARY]

ByteMatch ingests an objdiff `Report` (objdiff-core/protos/report.proto, report version 2; protobuf
JSON mapping, uint64 as strings) uploaded as a GitHub Actions artifact named `<version>_report`.
No ROM is needed:
- the denominator is the committed function catalogue config/<version>/catalogue.json (every
  function the analyzer found; fragments are code reached only by jumps and are not counted), less
  the functions config/<version>/classification.json classifies with high confidence as not C
  (hand-written assembly, compiler runtime, Sega library code);
- the numerator is every function credited in config/matches.json, which changes only when the
  integrator accepts a C build that is byte-identical to the original (docs/ACCEPTANCE.md).
Disassembled-but-not-decompiled code earns nothing: the byte-identical assembly baseline is the
starting point, not progress.

Units are 64 KiB address blocks until functions are grouped into translation units. Standard library
only, so CI needs nothing but Python 3.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT_VERSION = 2
BLOCK = 0x10000


def measures(fns: list[dict], matched: set[int], data_total: int = 0, data_done: int = 0) -> dict:
    total = sum(f["size"] for f in fns)
    done = sum(f["size"] for f in fns if f["address"] in matched)
    nf = len(fns)
    nm = sum(1 for f in fns if f["address"] in matched)
    pct = 100.0 * done / total if total else 0.0
    data_pct = 100.0 * data_done / data_total if data_total else 0.0
    return {
        "fuzzy_match_percent": pct,
        "total_code": str(total),
        "matched_code": str(done),
        "matched_code_percent": pct,
        "total_data": str(data_total),
        "matched_data": str(data_done),
        "matched_data_percent": data_pct,
        "total_functions": nf,
        "matched_functions": nm,
        "matched_functions_percent": 100.0 * nm / nf if nf else 0.0,
        "complete_code": str(done),
        "complete_code_percent": pct,
        "complete_data": str(data_done),
        "complete_data_percent": data_pct,
    }


def typed_data(version: str, catalogue: dict) -> int:
    """Bytes of the catalogue's data ranges owned by typed data units (config/<version>/data_units.json).

    A unit counts when its kind is not `raw`: its bytes are rebuilt from an extracted, typed asset.
    The units must tile the catalogue's header and data ranges exactly (tools/assets.py).
    """
    path = ROOT / "config" / version / "data_units.json"
    if not path.is_file():
        return 0
    sys.path.insert(0, str(ROOT / "tools"))
    import assets   # noqa: E402  (standard library only)
    config = json.loads(path.read_text(encoding="utf-8"))
    assets.validate_config(config, catalogue)
    ranges = [(d["address"], d["address"] + d["size"]) for d in catalogue["data"]]
    typed = 0
    for unit in config["units"]:
        start = int(unit["address"], 16)
        if unit["kind"] != "raw" and any(lo <= start < hi for lo, hi in ranges):
            typed += unit["size"]
    return typed


NON_C = ("handwritten_asm", "runtime_helper", "sega_library")


def excluded(version: str) -> set[int]:
    """Functions positively classified as not C (high confidence): never in the C denominator."""
    path = ROOT / "config" / version / "classification.json"
    if not path.is_file():
        return set()
    entries = json.loads(path.read_text(encoding="utf-8"))["functions"]
    return {int(e["address"], 16) for e in entries if e["class"] in NON_C and e["confidence"] == "high"}


def load(version: str) -> tuple[list[dict], set[int], int, int]:
    catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text(encoding="utf-8"))
    not_c = excluded(version)
    functions = sorted((f for f in catalogue["functions"] if f["kind"] == "function" and f["address"] not in not_c),
                       key=lambda f: f["address"])
    known = {f["address"] for f in functions}
    matched: set[int] = set()
    ledger = json.loads((ROOT / "config/matches.json").read_text(encoding="utf-8"))
    if ledger.get("schema_version") != 2:
        raise SystemExit("config/matches.json must be schema_version 2")
    for entry in ledger["functions"]:
        source = ROOT / entry["source"]
        if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != entry["source_sha256"]:
            raise SystemExit(f"matches.json credits {entry['name']} ({entry['version']}) to {entry['source']} as accepted, "
                             f"but the file differs or is gone: take the change through a checkpoint")
        if entry["version"] != version:
            continue
        address = int(entry["address"], 16)
        if address not in known:
            raise SystemExit(f"matches.json credits {entry['name']} at {entry['address']}, which is not a catalogued "
                             f"{version} C function")
        matched.add(address)
    data_total = sum(d["size"] for d in catalogue["data"])
    return functions, matched, data_total, typed_data(version, catalogue)


def display_names(version: str) -> dict[int, str]:
    """Function names from config/<version>/names.json (validated by tools/names.py at build time)."""
    path = ROOT / "config" / version / "names.json"
    if not path.is_file():
        return {}
    symbols = json.loads(path.read_text(encoding="utf-8"))["symbols"]
    return {int(s["address"], 16): s["name"] for s in symbols if s["kind"] in ("function", "label")}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="rev02", help="catalogue version (artifact name is <version>_report)")
    parser.add_argument("--output", type=Path, default=Path("report.json"))
    parser.add_argument("--summary", type=Path, help="also append a Markdown summary (e.g. $GITHUB_STEP_SUMMARY)")
    args = parser.parse_args()
    functions, matched, data_total, data_done = load(args.version)
    display = display_names(args.version)

    blocks: dict[int, list[dict]] = {}
    for f in functions:
        blocks.setdefault(f["address"] // BLOCK * BLOCK, []).append(f)
    units = []
    for base, fns in sorted(blocks.items()):
        m = measures(fns, matched)
        m["total_units"] = 1
        m["complete_units"] = int(m["matched_functions"] == m["total_functions"])
        units.append({
            "name": f"code_{base:06x}",
            "measures": m,
            "sections": [{"name": ".text", "size": m["total_code"], "fuzzy_match_percent": m["fuzzy_match_percent"]}],
            "functions": [
                {
                    "name": display.get(f["address"], f"func_{f['address']:06x}"),
                    "size": str(f["size"]),
                    "fuzzy_match_percent": 100.0 if f["address"] in matched else 0.0,
                    "metadata": {"virtual_address": str(f["address"])},
                }
                for f in fns
            ],
            "metadata": {"complete": False, "module_name": "rom", "progress_categories": ["game"], "auto_generated": True},
        })
    total = measures(functions, matched, data_total, data_done)
    total["total_units"] = len(units)
    total["complete_units"] = sum(u["measures"]["complete_units"] for u in units)
    report = {
        "measures": total,
        "units": units,
        "version": REPORT_VERSION,
        "categories": [{"id": "game", "name": "rom", "measures": total}],
    }
    args.output.write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8", newline="\n")
    line = (f"{args.version}: {total['matched_functions']}/{total['total_functions']} functions, "
            f"{total['matched_code']}/{total['total_code']} code bytes ({total['matched_code_percent']:.3f}%); "
            f"data {total['matched_data']}/{total['total_data']} bytes typed ({total['matched_data_percent']:.3f}%)")
    print(line)
    if args.summary:
        with args.summary.open("a") as f:
            f.write(f"### Progress\n\n{line}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
