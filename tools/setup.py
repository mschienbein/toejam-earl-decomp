"""Provision the pinned toolchain from toolchain.lock.json into .cache/toolchain/.

    python tools/setup.py            # build GNU binutils for every pinned target (m68k-elf, z80-elf)
    python tools/setup.py --check    # verify what is installed without downloading or building

The GNU tarball is checked against its pinned size and sha256 before use. Each target is built out of
tree into its own prefix; afterwards every installed program is hashed and its `--version` banner
recorded in .cache/toolchain/installed.json, which builds bind into their receipts. Nothing here is
committed. Needs a host C compiler and make (no system-wide installation).
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from toolchain import INSTALL, INSTALLED, LOCK, ROOT, binutils_prefix, file_sha256  # noqa: E402

DOWNLOADS = ROOT / ".cache/downloads"
WORK = ROOT / ".cache/work"
PROGRAMS = ("as", "ld", "objcopy", "objdump", "nm", "readelf")


class SetupError(RuntimeError):
    pass


def fetch(url: str, size: int, sha256: str) -> Path:
    """Download once into .cache/downloads/, verifying the pinned size and hash (also for cached files)."""
    DOWNLOADS.mkdir(parents=True, exist_ok=True)
    dest = DOWNLOADS / url.rsplit("/", 1)[1]
    if not dest.is_file() or dest.stat().st_size != size or file_sha256(dest) != sha256:
        tmp = dest.with_suffix(dest.suffix + ".part")
        print(f"downloading {url}", flush=True)
        with urllib.request.urlopen(url) as response, tmp.open("wb") as out:
            shutil.copyfileobj(response, out)
        if tmp.stat().st_size != size or file_sha256(tmp) != sha256:
            tmp.unlink()
            raise SetupError(f"{dest.name}: size/sha256 differ from the pinned values")
        tmp.replace(dest)
    return dest


def build_binutils(target: str, tarball: Path, jobs: int) -> Path:
    spec = LOCK["binutils"]
    prefix = binutils_prefix(target)
    if all((prefix / "bin" / f"{target}-{p}").is_file() for p in PROGRAMS):
        return prefix
    source = WORK / f"binutils-{spec['version']}"
    if not source.is_dir():
        WORK.mkdir(parents=True, exist_ok=True)
        with tarfile.open(tarball) as archive:
            archive.extractall(WORK, filter="data")
    build = WORK / f"build-{target}"
    if build.exists():
        shutil.rmtree(build)
    build.mkdir(parents=True)
    env = {**os.environ, "LC_ALL": "C"}
    make_vars = [f"{k}={v}" for k, v in spec.get("make_variables", {}).items()]
    print(f"configuring binutils {spec['version']} for {target}", flush=True)
    subprocess.run([str(source / "configure"), f"--target={target}", f"--prefix={prefix}", *spec["configure"]],
                   cwd=build, env=env, check=True, stdout=subprocess.DEVNULL)
    print(f"building {target} (jobs={jobs})", flush=True)
    subprocess.run(["make", f"-j{jobs}", *make_vars], cwd=build, env=env, check=True, stdout=subprocess.DEVNULL)
    subprocess.run(["make", "install", *make_vars], cwd=build, env=env, check=True, stdout=subprocess.DEVNULL)
    shutil.rmtree(build)
    return prefix


def record() -> dict:
    spec = LOCK["binutils"]
    result = {"schema_version": 1, "binutils": {"version": spec["version"], "sha256": spec["sha256"], "targets": {}}}
    for target in spec["targets"]:
        prefix = binutils_prefix(target)
        programs = {}
        for name in PROGRAMS:
            path = prefix / "bin" / f"{target}-{name}"
            if not path.is_file():
                raise SetupError(f"{path} missing after install")
            banner = subprocess.run([str(path), "--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0]
            programs[name] = {"path": str(path.relative_to(ROOT)), "sha256": file_sha256(path), "banner": banner}
        result["binutils"]["targets"][target] = programs
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="verify the installed toolchain only")
    parser.add_argument("--jobs", type=int, default=max(1, min(16, os.cpu_count() or 1)))
    args = parser.parse_args()
    spec = LOCK["binutils"]
    if not args.check:
        tarball = fetch(spec["url"], spec["size"], spec["sha256"])
        for target in spec["targets"]:
            build_binutils(target, tarball, args.jobs)
    current = record()
    INSTALL.mkdir(parents=True, exist_ok=True)
    if args.check:
        if not INSTALLED.is_file() or json.loads(INSTALLED.read_text()) != current:
            print("installed toolchain differs from .cache/toolchain/installed.json; run tools/setup.py", file=sys.stderr)
            return 1
        print("toolchain OK")
        return 0
    INSTALLED.write_text(json.dumps(current, indent=1, sort_keys=True) + "\n")
    for target, programs in current["binutils"]["targets"].items():
        print(f"{target}: {programs['as']['banner']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
