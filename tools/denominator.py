"""Hash-chained history of the progress denominator (standard library only; CI-safe).

    python tools/denominator.py verify                              # chain intact, files unchanged
    python tools/denominator.py record --note "why this revision"   # integrator only

The denominator is what 100% means: per version, the function catalogue
(config/<version>/catalogue.json) and, once they exist, the function classification
(classification.json) and the typed data units (data_units.json). Those files may change only
together with a new entry here. Each entry records their sha256 and headline counts and links to
the previous entry's hash, so a silent edit to the denominator fails verification.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HISTORY = ROOT / "docs/denominator-history.jsonl"
VERSIONS = ("rev02", "rev00")
FILES = ("catalogue.json", "classification.json", "data_units.json")


def canonical(entry: dict) -> bytes:
    return json.dumps(entry, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def entry_hash(entry: dict) -> str:
    return hashlib.sha256(canonical({k: v for k, v in entry.items() if k != "entry_sha256"})).hexdigest()


def read_history(path: Path = HISTORY) -> list[dict]:
    if not path.is_file():
        return []
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def snapshot(root: Path = ROOT) -> dict:
    """Hashes and headline counts of every denominator file that exists, per version."""
    result = {}
    for version in VERSIONS:
        files = {}
        for name in FILES:
            path = root / "config" / version / name
            if path.is_file():
                files[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        catalogue = json.loads((root / "config" / version / "catalogue.json").read_text(encoding="utf-8"))
        stats = catalogue.get("stats", {})
        result[version] = {
            "files": files,
            "functions": stats.get("functions"),
            "fragments": stats.get("fragments"),
            "code_bytes": stats.get("code_bytes"),
            "function_bytes": sum(f["size"] for f in catalogue["functions"] if f["kind"] == "function"),
        }
    return result


def verify(root: Path = ROOT, history: Path = HISTORY) -> list[dict]:
    entries = read_history(history)
    if not entries:
        raise SystemExit("denominator: no recorded revision")
    prev = None
    for number, entry in enumerate(entries, 1):
        if entry.get("revision") != number or entry.get("prev") != prev or entry.get("entry_sha256") != entry_hash(entry):
            raise SystemExit(f"denominator: history broken at revision {number}")
        prev = entry["entry_sha256"]
    current = snapshot(root)
    recorded = {v: entries[-1]["versions"][v]["files"] for v in VERSIONS}
    actual = {v: current[v]["files"] for v in VERSIONS}
    if actual != recorded:
        changed = sorted(f"{v}/{n}" for v in VERSIONS for n in set(actual[v]) | set(recorded[v])
                         if actual[v].get(n) != recorded[v].get(n))
        raise SystemExit(f"denominator: {', '.join(changed)} changed without a recorded revision "
                         f"(latest is r{len(entries)}); run tools/denominator.py record --note ...")
    return entries


def record(note: str, root: Path = ROOT, history: Path = HISTORY) -> dict:
    entries = read_history(history)
    entry = {
        "revision": len(entries) + 1,
        "recorded_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "note": note,
        "versions": snapshot(root),
        "prev": entries[-1]["entry_sha256"] if entries else None,
    }
    entry["entry_sha256"] = entry_hash(entry)
    history.parent.mkdir(parents=True, exist_ok=True)
    with history.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(entry, ensure_ascii=False, sort_keys=True) + "\n")
    return entry


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("verify")
    rec = sub.add_parser("record")
    rec.add_argument("--note", required=True)
    args = parser.parse_args()
    if args.command == "verify":
        entries = verify()
        latest = entries[-1]["versions"]["rev02"]
        print(f"denominator r{len(entries)} OK: rev02 {latest['functions']} functions, {latest['function_bytes']} bytes")
        return 0
    entry = record(args.note)
    print(f"recorded denominator r{entry['revision']} ({entry['entry_sha256'][:12]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
