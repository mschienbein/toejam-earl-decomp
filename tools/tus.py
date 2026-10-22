"""Original translation-unit (C file) spans, recovered from PC-relative references.

    python tools/tus.py [--version rev02]           # writes config/<version>/tus.json
    python tools/tus.py [--version rev02] --check   # fails if the committed file is stale

Microtec's assembler turns a call or address reference into PC-relative form only when the target
is in the same section of the same module, i.e. the same C file. Every PC-relative reference from
one catalogued function to another therefore joins them into one translation unit, and a module is
linked as one contiguous block, so each joined group spans a contiguous address range (overlapping
groups merge). Functions with no such references are not assigned: their file boundaries do not
change any byte and stay open until matching shows them.

The C pipeline uses the spans so that a C unit covering part of a translation unit can still call
the rest of that unit the way the original assembler did (tools/mcc.py `near`).
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import m68k  # noqa: E402
import rom as romtool  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
PC_RELATIVE = 0x3A   # EA mode 7, register 2: (d16,PC)


def path(version: str) -> Path:
    return ROOT / "config" / version / "tus.json"


def derive(rom: bytes, catalogue: dict) -> list[dict]:
    tables = {t["address"]: t["count"] for t in catalogue["jump_tables"]}
    functions = sorted(f["address"] for f in catalogue["functions"])
    size = {f["address"]: f["size"] for f in catalogue["functions"]}

    def owner(address: int) -> int | None:
        k = bisect.bisect_right(functions, address) - 1
        return functions[k] if k >= 0 and address < functions[k] + size[functions[k]] else None

    parent = {f: f for f in functions}

    def find(x: int) -> int:
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for f in functions:
        pc, end = f, f + size[f]
        while pc < end:
            if pc in tables:
                pc += 2 * tables[pc]
                continue
            insn = m68k.decode_one(rom, pc)
            if insn.base in ("jsr", "jmp", "lea", "pea") and insn.raw[1] & 0x3F == PC_RELATIVE and insn.targets:
                target = owner(insn.targets[0])
                if target is not None and target != f:
                    parent[find(target)] = find(f)
            pc += insn.size
    groups: dict[int, list[int]] = {}
    for f in functions:
        groups.setdefault(find(f), []).append(f)
    spans: list[list[int]] = []
    for lo, hi in sorted((min(g), max(g) + size[max(g)]) for g in groups.values() if len(g) > 1):
        if spans and lo < spans[-1][1]:
            spans[-1][1] = max(spans[-1][1], hi)
        else:
            spans.append([lo, hi])
    return [{"start": f"0x{lo:06x}", "end": f"0x{hi:06x}",
             "functions": [f"0x{f:06x}" for f in functions if lo <= f < hi]} for lo, hi in spans]


def load(version: str) -> list[tuple[int, int]]:
    file = path(version)
    if not file.is_file():
        return []
    return [(int(s["start"], 16), int(s["end"], 16)) for s in json.loads(file.read_text())["spans"]]


def span_of(spans: list[tuple[int, int]], address: int) -> tuple[int, int] | None:
    for lo, hi in spans:
        if lo <= address < hi:
            return lo, hi
    return None


def render(version: str, rom: bytes, catalogue: dict) -> str:
    spans = derive(rom, catalogue)
    document = {"schema_version": 1, "version": version, "rom_sha256": hashlib.sha256(rom).hexdigest(),
                "note": "Contiguous spans joined by PC-relative references between catalogued functions (tools/tus.py).",
                "spans": spans}
    lines = ["{"] + [f' "{k}": {json.dumps(v)},' for k, v in document.items() if k != "spans"] + [' "spans": [']
    lines += [f"  {json.dumps(s)}{',' if k + 1 < len(spans) else ''}" for k, s in enumerate(spans)]
    return "\n".join(lines + [" ]", "}"]) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="rev02")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    rom = romtool.load(args.version)
    catalogue = json.loads((ROOT / "config" / args.version / "catalogue.json").read_text())
    text = render(args.version, rom, catalogue)
    if args.check:
        ok = path(args.version).is_file() and path(args.version).read_text() == text
        print(f"{path(args.version).relative_to(ROOT)} {'OK' if ok else 'is stale'}")
        return 0 if ok else 1
    path(args.version).write_text(text)
    spans = json.loads(text)["spans"]
    print(f"{args.version}: {len(spans)} translation-unit spans, {sum(len(s['functions']) for s in spans)} functions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
