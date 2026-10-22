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


def prepare(checkpoint: str, jobs: int) -> dict:
    if not NAME.match(checkpoint):
        raise AcceptError("checkpoint names look like c001-short-name")
    record_path = ACCEPT / checkpoint / "prepare.json"
    if record_path.exists():
        raise AcceptError(f"{record_path.relative_to(ROOT)} exists; use a new checkpoint name")
    builds, ok = {}, True
    for version in VERSIONS:
        rom = romtool.load(version)
        catalogue = json.loads((ROOT / "config" / version / "catalogue.json").read_text())
        baserom = ROOT / romtool.title()["versions"][version]["orig"]
        receipts = []
        for run in ("a", "b"):
            out = ROOT / "build" / "accept" / checkpoint / run / version
            shutil.rmtree(out, ignore_errors=True)
            out.mkdir(parents=True)
            receipt = build.build_image(rom, baserom, catalogue, out, jobs, namestool.load(version),
                                        cunits.load_sources(version), build.tus.load(version))
            (out / "receipt.json").write_text(json.dumps(receipt, indent=1) + "\n")
            receipts.append(receipt)
        same = receipts[0]["actual"]["sha256"] == receipts[1]["actual"]["sha256"]
        passed = all(r["result"] == "PASS" for r in receipts) and same
        ok &= passed
        builds[version] = {"result": "PASS" if passed else "FAIL", "image_sha256": [r["actual"]["sha256"] for r in receipts],
                           "c_units": receipts[0]["c_units"], "raw_words": receipts[0]["instructions"]["raw_words"],
                           "receipt_sha256": [sha256(json.dumps(r, sort_keys=True).encode()) for r in receipts]}
    suite = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests"], cwd=ROOT, capture_output=True, text=True)
    summary = (suite.stderr.strip().splitlines() or ["(no output)"])[-1]
    ok &= suite.returncode == 0
    document = {"checkpoint": checkpoint, "prepared_utc": now(), "result": "PASS" if ok else "FAIL",
                "git": git_head(), "sources": {v: sources_digest(v) for v in VERSIONS},
                "builds": builds, "suite": {"returncode": suite.returncode, "summary": summary}}
    write_once(record_path, document)
    return document


def review(checkpoint: str, reviewer: str, verdict: str, file: Path) -> dict:
    if verdict not in ("PASS", "PASS_WITH_QUALIFICATIONS", "FAIL"):
        raise AcceptError("verdict is PASS, PASS_WITH_QUALIFICATIONS or FAIL")
    prepared = json.loads((ACCEPT / checkpoint / "prepare.json").read_text())
    body = file.read_bytes()
    document = {"checkpoint": checkpoint, "reviewer": reviewer, "verdict": verdict, "recorded_utc": now(),
                "prepare_sha256": sha256((ACCEPT / checkpoint / "prepare.json").read_bytes()),
                "review_file": file.name, "review_sha256": sha256(body), "prepared_result": prepared["result"]}
    write_once(ACCEPT / checkpoint / f"review-{re.sub(r'[^A-Za-z0-9_-]', '_', reviewer)}.json", document)
    shutil.copyfile(file, ACCEPT / checkpoint / f"review-{re.sub(r'[^A-Za-z0-9_-]', '_', reviewer)}-{file.name}")
    return document


def finalize(checkpoint: str, note: str) -> dict:
    folder = ACCEPT / checkpoint
    prepared = json.loads((folder / "prepare.json").read_text())
    if prepared["result"] != "PASS":
        raise AcceptError("prepare did not pass")
    reviews = [json.loads(p.read_text()) for p in sorted(folder.glob("review-*.json")) if "verdict" in p.read_text()]
    reviews = [r for r in reviews if r.get("checkpoint") == checkpoint]
    if not any(r["verdict"].startswith("PASS") for r in reviews) or any(r["verdict"] == "FAIL" for r in reviews):
        raise AcceptError("finalize needs at least one PASS review and no FAIL review")
    for version in VERSIONS:
        if sources_digest(version) != prepared["sources"][version]:
            raise AcceptError(f"{version}: C sources changed since prepare")
    matches = json.loads(MATCHES.read_text())
    credited = {(u["version"], u["source"]) for u in matches["units"]}
    added = []
    for version in VERSIONS:
        for unit in prepared["builds"][version]["c_units"]:
            key = (version, unit["source"])
            if key in credited:
                continue
            entry = {"version": version, "source": unit["source"], "sha256": unit["sha256"], "address": unit["address"],
                     "size": unit["size"], "functions": unit["functions"], "checkpoint": checkpoint}
            matches["units"].append(entry)
            added.append(entry)
    matches["units"].sort(key=lambda u: (u["version"], int(u["address"], 16)))
    MATCHES.write_text(json.dumps(matches, indent=1) + "\n")
    rev02 = [u for u in matches["units"] if u["version"] == "rev02"]
    point = {"checkpoint": checkpoint, "utc": now(), "functions": sum(len(u["functions"]) for u in rev02),
             "bytes": sum(u["size"] for u in rev02), "units": len(rev02)}
    with HISTORY.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(point, sort_keys=True) + "\n")
    document = {"checkpoint": checkpoint, "finalized_utc": now(), "note": note, "added": added,
                "prepare_sha256": sha256((folder / "prepare.json").read_bytes()),
                "reviews": [{"reviewer": r["reviewer"], "verdict": r["verdict"]} for r in reviews], "progress": point}
    write_once(folder / "finalize.json", document)
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
            print(f"{args.checkpoint}: prepare {document['result']} "
                  + ", ".join(f"{v} {document['builds'][v]['result']}" for v in VERSIONS) + f"; suite {document['suite']['summary']}")
            return 0 if document["result"] == "PASS" else 1
        if args.command == "review":
            document = review(args.checkpoint, args.reviewer, args.verdict, args.file)
            print(f"{args.checkpoint}: review by {document['reviewer']} recorded ({document['verdict']})")
            return 0
        document = finalize(args.checkpoint, args.note)
        print(f"{args.checkpoint}: finalized; +{sum(len(a['functions']) for a in document['added'])} functions; "
              f"rev02 total {document['progress']['functions']} functions / {document['progress']['bytes']} bytes")
        return 0
    except (AcceptError, build.BuildError, cunits.CUnitError, romtool.RomError, FileExistsError, FileNotFoundError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
