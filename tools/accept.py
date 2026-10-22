"""Checkpoints: the only way C becomes credited progress.

    python tools/accept.py prepare  --checkpoint c001-first-leafs
    python tools/accept.py review   --checkpoint c001-first-leafs --reviewer NAME --verdict PASS --file review.json
    python tools/accept.py finalize --checkpoint c001-first-leafs --note "Main confirmation ..."

prepare: with config/sources.json as it stands, builds every version twice from scratch
  (build/accept/<checkpoint>/{a,b}/<version>), requires PASS and identical images for both builds of
  both versions, runs the full test suite, and writes accept/<checkpoint>/prepare.json (exclusive
  create: a checkpoint name is used once; a failed prepare stays on record, retry under a new name).
review: records an independent reviewer's verdict for the prepared checkpoint (immutable file).
finalize: requires a PASS prepare, at least one PASS review and no FAIL review, and C sources
  unchanged since prepare; then credits the checkpoint's C units in config/matches.json, appends
  the REV 02 totals to docs/progress-history.jsonl and writes accept/<checkpoint>/finalize.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build  # noqa: E402
import cunits  # noqa: E402
import names as namestool  # noqa: E402
import rom as romtool  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ACCEPT = ROOT / "accept"
MATCHES = ROOT / "config" / "matches.json"
HISTORY = ROOT / "docs" / "progress-history.jsonl"
VERSIONS = ("rev02", "rev00")
NAME = re.compile(r"^c\d{3}-[a-z0-9-]+$")


class AcceptError(RuntimeError):
    pass


def now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_once(path: Path, document: dict) -> None:
    """Exclusive create + fsync: records are never overwritten."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "x", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(document, indent=1, sort_keys=True) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def git_head() -> dict:
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True).stdout.strip()
    dirty = subprocess.run(["git", "status", "--porcelain", "--", "src", "config", "tools", "include"],
                           cwd=ROOT, capture_output=True, text=True).stdout.strip()
    return {"head": head, "dirty_paths": dirty.splitlines()}


def sources_digest(version: str) -> list[dict]:
    return [{"source": entry["source"], "sha256": sha256((ROOT / entry["source"]).read_bytes()),
             "options": entry.get("options", [])} for entry in cunits.load_sources(version)]


INPUT_ROOTS = ("src", "include", "config", "tools", "ci", "tests")
INPUT_FILES = ("toolchain.lock.json",)
LEDGER_FILES = ("config/matches.json",)   # written by finalize itself


def inputs_manifest() -> dict[str, str]:
    """sha256 of every file that can change a build or the suite (the checkpoint ledger excluded)."""
    files = [ROOT / name for name in INPUT_FILES]
    for folder in INPUT_ROOTS:
        files += [p for p in (ROOT / folder).rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    manifest = {}
    for path in sorted(files):
        relative = path.relative_to(ROOT).as_posix()
        if relative not in LEDGER_FILES:
            manifest[relative] = sha256(path.read_bytes())
    return manifest


def build_both(checkpoint: str, run: str, jobs: int) -> dict:
    receipts = {}
    for version in VERSIONS:
        rom = romtool.load(version)
        catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text())
        baserom = ROOT / romtool.title()["versions"][version]["orig"]
        out = ROOT / "build" / "accept" / checkpoint / run / version
        shutil.rmtree(out, ignore_errors=True)
        out.mkdir(parents=True)
        receipt = build.build_image(rom, baserom, catalogue, out, jobs, namestool.load(version),
                                    cunits.load_sources(version), build.tus.load(version),
                                    build.revmap.translator(version), build.prepare_data(version, rom, catalogue, out))
        (out / "receipt.json").write_text(json.dumps(receipt, indent=1) + "\n")
        receipts[version] = receipt
    return receipts


def prepare(checkpoint: str, jobs: int) -> dict:
    """Two fresh builds of every version + the suite; the record is written whatever happens."""
    if not NAME.match(checkpoint):
        raise AcceptError("checkpoint names look like c001-short-name")
    folder = ACCEPT / checkpoint
    folder.parent.mkdir(parents=True, exist_ok=True)
    try:
        folder.mkdir()   # claims the name: a checkpoint name is used once, even when prepare fails
    except FileExistsError:
        raise AcceptError(f"{folder.relative_to(ROOT)} exists; use a new checkpoint name") from None
    document = {"checkpoint": checkpoint, "prepared_utc": now(), "result": "FAIL", "git": git_head()}
    try:
        if orphan_sources():
            raise AcceptError(f"C files not listed in config/sources.json: {orphan_sources()}")
        document["inputs"] = inputs_manifest()
        document["sources"] = {v: sources_digest(v) for v in VERSIONS}
        runs = [build_both(checkpoint, run, jobs) for run in ("a", "b")]
        builds, ok = {}, True
        for version in VERSIONS:
            receipts = [r[version] for r in runs]
            same = receipts[0]["actual"]["sha256"] == receipts[1]["actual"]["sha256"]
            passed = all(r["result"] == "PASS" for r in receipts) and same
            ok &= passed
            builds[version] = {"result": "PASS" if passed else "FAIL",
                               "image_sha256": [r["actual"]["sha256"] for r in receipts],
                               "c_units": receipts[0]["c_units"], "raw_words": receipts[0]["instructions"]["raw_words"],
                               "receipt_sha256": [sha256(json.dumps(r, sort_keys=True).encode()) for r in receipts]}
        document["builds"] = builds
        suite = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests"], cwd=ROOT,
                               capture_output=True, text=True)
        document["suite"] = {"returncode": suite.returncode,
                             "summary": (suite.stderr.strip().splitlines() or ["(no output)"])[-1]}
        ok &= suite.returncode == 0
        if document["inputs"] != inputs_manifest():
            document["error"] = "inputs changed while the checkpoint was being prepared"
            ok = False
        document["result"] = "PASS" if ok else "FAIL"
    except Exception as error:   # compiler, assembler, link or tool failure: recorded, never lost
        document["error"] = f"{type(error).__name__}: {error}"
    write_once(folder / "prepare.json", document)
    return document


def review(checkpoint: str, reviewer: str, verdict: str, file: Path) -> dict:
    if verdict not in ("PASS", "PASS_WITH_QUALIFICATIONS", "FAIL"):
        raise AcceptError("verdict is PASS, PASS_WITH_QUALIFICATIONS or FAIL")
    if (ACCEPT / checkpoint / "finalize.json").exists():
        raise AcceptError(f"{checkpoint} is already finalized")
    prepared = json.loads((ACCEPT / checkpoint / "prepare.json").read_text())
    body = file.read_bytes()
    safe = re.sub(r"[^A-Za-z0-9_-]", "_", reviewer)
    document = {"checkpoint": checkpoint, "reviewer": reviewer, "verdict": verdict, "recorded_utc": now(),
                "prepare_sha256": sha256((ACCEPT / checkpoint / "prepare.json").read_bytes()),
                "review_file": file.name, "review_sha256": sha256(body), "prepared_result": prepared["result"]}
    write_once(ACCEPT / checkpoint / f"review-{safe}.json", document)
    shutil.copyfile(file, ACCEPT / checkpoint / f"review-{safe}-{file.name}")
    return document


def credited_functions(c_units: list[dict], version: str, checkpoint: str) -> list[dict]:
    """One record per C function; its size is the catalogued code extent (literal data never counts)."""
    catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text())
    extent = {f["address"]: f["size"] for f in catalogue["functions"] if f["kind"] == "function"}
    records = []
    for unit in c_units:
        for f in unit["functions"]:
            address = int(f["address"], 16)
            if address not in extent:
                raise AcceptError(f"{version}: {f['name']} at {f['address']} is not a catalogued function")
            if f["size"] < extent[address]:
                raise AcceptError(f"{version}: {f['name']} emits {f['size']} bytes, the catalogue says {extent[address]}")
            records.append({"version": version, "address": f["address"], "name": f["name"], "size": extent[address],
                            "source": unit["source"], "source_sha256": unit["sha256"], "checkpoint": checkpoint})
    return records


def orphan_sources() -> list[str]:
    """C files under src/ that config/sources.json does not list (they would never be built or checked)."""
    listed = {entry["source"] for version in VERSIONS for entry in cunits.load_sources(version)}
    return sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / "src").rglob("*.c") if p.relative_to(ROOT).as_posix() not in listed)


def finalize(checkpoint: str, note: str, jobs: int = os.cpu_count() or 4) -> dict:
    """Credit a prepared, reviewed checkpoint after re-verifying it against the current tree.

    Nothing is written unless every check passes: a PASS prepare, a PASS review and no FAIL review,
    C sources and headers byte-identical to the prepared ones, and a fresh build of every version
    that is byte-identical with exactly the prepared C units. Credit is per function (version and
    address): a function moved to another file replaces its old record and never counts twice.
    """
    folder = ACCEPT / checkpoint
    if (folder / "finalize.json").exists():
        raise AcceptError(f"{checkpoint} is already finalized")
    prepared = json.loads((folder / "prepare.json").read_text())
    if prepared["result"] != "PASS":
        raise AcceptError("prepare did not pass")
    reviews = [json.loads(p.read_text()) for p in sorted(folder.glob("review-*.json"))]
    reviews = [r for r in reviews if isinstance(r, dict) and r.get("checkpoint") == checkpoint and "verdict" in r]
    if not any(r["verdict"].startswith("PASS") for r in reviews) or any(r["verdict"] == "FAIL" for r in reviews):
        raise AcceptError("finalize needs at least one PASS review and no FAIL review")
    current = inputs_manifest()
    reviewed = {k: v for k, v in prepared["inputs"].items() if k.startswith(("src/", "include/"))}
    now_c = {k: v for k, v in current.items() if k.startswith(("src/", "include/"))}
    if reviewed != now_c:
        changed = sorted(set(reviewed.items()) ^ set(now_c.items()))
        raise AcceptError(f"C sources or headers changed since prepare: {sorted({k for k, _ in changed})}")
    for version in VERSIONS:
        if sources_digest(version) != prepared["sources"][version]:
            raise AcceptError(f"{version}: the C unit list changed since prepare")
    final = build_both(checkpoint, "final", jobs)
    for version in VERSIONS:
        if final[version]["result"] != "PASS":
            raise AcceptError(f"{version}: the current tree no longer builds byte-identically")
        if final[version]["c_units"] != prepared["builds"][version]["c_units"]:
            raise AcceptError(f"{version}: the C units built now differ from the prepared ones")
    if orphan_sources():
        raise AcceptError(f"C files not listed in config/sources.json: {orphan_sources()}")
    matches = json.loads(MATCHES.read_text())
    if matches.get("schema_version") != 2:
        raise AcceptError("config/matches.json must be schema_version 2")
    ledger = {(f["version"], f["address"]): f for f in matches["functions"]}
    current = {(version, entry["address"]): entry for version in VERSIONS
               for entry in credited_functions(final[version]["c_units"], version, checkpoint)}
    withdrawn = sorted(f"{v} {ledger[(v, a)]['name']} ({a})" for v, a in ledger if (v, a) not in current)
    if withdrawn:   # credit always describes the current image: nothing credited may silently stop being C
        raise AcceptError(f"credited functions are no longer built from C: {withdrawn}")
    added, moved = [], []
    for version in VERSIONS:
        for entry in [e for (v, _), e in current.items() if v == version]:
            key = (version, entry["address"])
            previous = ledger.get(key)
            if previous is None:
                added.append(entry)
            elif any(previous[k] != entry[k] for k in ("name", "size", "source", "source_sha256")):
                moved.append({"from": previous, "to": entry})   # moved, edited or renamed: re-bound to this checkpoint
            else:
                continue
            ledger[key] = entry
    matches["functions"] = sorted(ledger.values(), key=lambda f: (f["version"], int(f["address"], 16)))
    rev02 = [f for f in matches["functions"] if f["version"] == "rev02"]
    point = {"checkpoint": checkpoint, "utc": now(), "functions": len(rev02), "bytes": sum(f["size"] for f in rev02)}
    document = {"checkpoint": checkpoint, "finalized_utc": now(), "note": note, "added": added, "moved": moved,
                "prepare_sha256": sha256((folder / "prepare.json").read_bytes()),
                "final_images": {v: final[v]["actual"]["sha256"] for v in VERSIONS},
                "reviews": [{"reviewer": r["reviewer"], "verdict": r["verdict"]} for r in reviews], "progress": point}
    write_once(folder / "finalize.json", document)   # claims the finalization before shared state changes
    MATCHES.write_text(json.dumps(matches, indent=1) + "\n")
    with HISTORY.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(point, sort_keys=True) + "\n")
    return document


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("prepare")
    p.add_argument("--checkpoint", required=True)
    p.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    r = sub.add_parser("review")
    r.add_argument("--checkpoint", required=True)
    r.add_argument("--reviewer", required=True)
    r.add_argument("--verdict", required=True)
    r.add_argument("--file", type=Path, required=True)
    f = sub.add_parser("finalize")
    f.add_argument("--checkpoint", required=True)
    f.add_argument("--note", required=True)
    args = parser.parse_args()
    try:
        if args.command == "prepare":
            document = prepare(args.checkpoint, args.jobs)
            if "builds" in document and "suite" in document:
                print(f"{args.checkpoint}: prepare {document['result']} "
                      + ", ".join(f"{v} {document['builds'][v]['result']}" for v in VERSIONS)
                      + f"; suite {document['suite']['summary']}" + (f"; {document['error']}" if "error" in document else ""))
            else:
                print(f"{args.checkpoint}: prepare {document['result']}: {document.get('error', 'no builds recorded')}")
            return 0 if document["result"] == "PASS" else 1
        if args.command == "review":
            document = review(args.checkpoint, args.reviewer, args.verdict, args.file)
            print(f"{args.checkpoint}: review by {document['reviewer']} recorded ({document['verdict']})")
            return 0
        document = finalize(args.checkpoint, args.note)
        print(f"{args.checkpoint}: finalized; +{len(document['added'])} function records, {len(document['moved'])} moved; "
              f"rev02 total {document['progress']['functions']} functions / {document['progress']['bytes']} bytes")
        return 0
    except (AcceptError, build.BuildError, cunits.CUnitError, romtool.RomError, FileExistsError, FileNotFoundError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
