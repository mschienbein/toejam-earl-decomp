"""Function correspondence between the primary revision (REV 02) and another revision.

    python tools/revmap.py [--version rev00]           # writes config/<version>/correspondence.json
    python tools/revmap.py [--version rev00] --check   # fails if the committed file is stale

One C source builds every revision, so a function keeps one identity: its REV 02 name, which for
an unnamed function is the REV 02 spelling `func_XXXXXX`. tools/names.py gives the corresponding
function of the other revision that name too.

The correspondence is accepted only when it is unambiguous: both catalogues list the functions in
the same order with the same count, and aligning their relocation-masked instruction signatures
(absolute ROM addresses and branch targets outside the function masked) produces no insertion or
deletion, only identical functions and equal-length runs of changed ones, so pairing by position
is the only consistent reading. Imported names that both revisions carry must agree with it.
"""
from __future__ import annotations

import argparse
import bisect
import difflib
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import m68k  # noqa: E402
import names as namestool  # noqa: E402
import rom as romtool  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = "rev02"


class RevmapError(RuntimeError):
    pass


def path(version: str, root: Path = ROOT) -> Path:
    return root / "config" / version / "correspondence.json"


def signatures(rom: bytes, catalogue: dict) -> list[tuple[int, int, str]]:
    """(address, size, masked signature hash) per catalogued function, in address order."""
    tables = {t["address"]: t["count"] for t in catalogue["jump_tables"]}
    result = []
    for function in sorted(catalogue["functions"], key=lambda f: f["address"]):
        start, size = function["address"], function["size"]
        parts, pc = [], start
        while pc < start + size:
            if pc in tables:
                parts.append(rom[pc:pc + 2 * tables[pc]].hex())
                pc += 2 * tables[pc]
                continue
            insn = m68k.decode_one(rom, pc)
            text = f"{insn.mnemonic} {insn.op_str}"
            targets = set(insn.targets) | ({insn.branch_target} if insn.branch_target is not None else set())
            for target in sorted(targets, reverse=True):
                if target < namestool.ROM_END:
                    spelled = f"L+{target - start:x}" if start <= target < start + size else "T"
                    text = re.sub(rf"\$0*{target:x}\b", spelled, text, flags=re.I)
            parts.append(text)
            pc += insn.size
        result.append((start, size, hashlib.sha256("\n".join(parts).encode()).hexdigest()))
    return result


def derive(version: str, roms: dict[str, bytes], catalogues: dict[str, dict], names: dict[str, dict]) -> dict:
    primary, other = signatures(roms[PRIMARY], catalogues[PRIMARY]), signatures(roms[version], catalogues[version])
    if len(primary) != len(other):
        raise RevmapError(f"{PRIMARY} has {len(primary)} functions, {version} {len(other)}: no positional correspondence")
    matcher = difflib.SequenceMatcher(None, [s for _, _, s in primary], [s for _, _, s in other], autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag != "equal" and (tag != "replace" or i2 - i1 != j2 - j1 or i1 != j1):
            raise RevmapError(f"signature alignment has a {tag} at {PRIMARY} 0x{primary[i1][0]:06x}: not one-to-one")
    pairs = [{"rev02": f"0x{a:06x}", version: f"0x{b:06x}", "size": [n, m], "same": s == t}
             for (a, n, s), (b, m, t) in zip(primary, other)]
    by_primary = {int(p["rev02"], 16): int(p[version], 16) for p in pairs}
    other_names = {n.name: n.address for n in names[version].values()}
    for name in names[PRIMARY].values():
        if name.kind == "function" and name.address in by_primary and name.name in other_names:
            if other_names[name.name] != by_primary[name.address]:
                raise RevmapError(f"{name.name}: {version} names 0x{other_names[name.name]:06x}, pairing says "
                                  f"0x{by_primary[name.address]:06x}")
    return {"schema_version": 1, "primary": PRIMARY, "version": version,
            "rom_sha256": {PRIMARY: hashlib.sha256(roms[PRIMARY]).hexdigest(),
                           version: hashlib.sha256(roms[version]).hexdigest()},
            "method": "positional pairing of catalogued functions, verified by masked-signature alignment (tools/revmap.py)",
            "identical": sum(p["same"] for p in pairs), "functions": pairs}


def render(document: dict) -> str:
    head = {k: v for k, v in document.items() if k != "functions"}
    lines = ["{"] + [f' "{k}": {json.dumps(v)},' for k, v in head.items()] + [' "functions": [']
    pairs = document["functions"]
    lines += [f"  {json.dumps(p)}{',' if k + 1 < len(pairs) else ''}" for k, p in enumerate(pairs)]
    return "\n".join(lines + [" ]", "}"]) + "\n"


def load(version: str, root: Path = ROOT) -> dict[int, int]:
    """Primary-revision function address -> `version` function address (empty for the primary)."""
    file = path(version, root)
    if version == PRIMARY or not file.is_file():
        return {}
    return {int(p[PRIMARY], 16): int(p[version], 16) for p in json.loads(file.read_text())["functions"]}


def translator(version: str, root: Path = ROOT):
    """Primary-revision address -> the same place in `version`, or None where there is none.

    C names unnamed places by their primary (REV 02) address: `loc_`, `dat_`, `rom_XXXXXX`. Both
    catalogues list the same functions and data ranges in the same order, so an address keeps its
    offset inside the corresponding function or data range; vectors, header and RAM do not move.
    A wrong guess cannot pass: the build compares every byte of the other revision too.
    """
    if version == PRIMARY:
        return lambda address: address
    regions = {}
    for v in (PRIMARY, version):
        catalogue = json.loads((root / "config" / v / "catalogue.json").read_text())
        regions[v] = sorted([(f["address"], f["size"], "function") for f in catalogue["functions"]] +
                            [(d["address"], d["size"], "data") for d in catalogue["data"]])
    ours, theirs = regions[PRIMARY], regions[version]
    if [kind for *_, kind in ours] != [kind for *_, kind in theirs]:
        raise RevmapError(f"{PRIMARY} and {version} catalogues do not list the same functions and data ranges")
    starts = [start for start, *_ in ours]
    first = starts[0]

    def translate(address: int) -> int | None:
        if address >= 0xFF0000 or address < first:
            return address
        k = bisect.bisect_right(starts, address) - 1
        (start, size, _), (their_start, their_size, _) = ours[k], theirs[k]
        offset = address - start
        return their_start + offset if offset < min(size, their_size) else None

    return translate


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="rev00")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    versions = (PRIMARY, args.version)
    roms = {v: romtool.load(v) for v in versions}
    catalogues = {v: json.loads((ROOT / "config" / v / "catalogue.json").read_text()) for v in versions}
    names = {v: namestool.parse(json.loads(namestool.path(v).read_text()), v) for v in versions}
    text = render(derive(args.version, roms, catalogues, names))
    target = path(args.version)
    if args.check:
        ok = target.is_file() and target.read_text() == text
        print(f"{target.relative_to(ROOT)} {'OK' if ok else 'is stale'}")
        return 0 if ok else 1
    target.write_text(text)
    document = json.loads(text)
    print(f"{args.version}: {len(document['functions'])} functions paired with {PRIMARY}, {document['identical']} identical")
    return 0


if __name__ == "__main__":
    sys.exit(main())
