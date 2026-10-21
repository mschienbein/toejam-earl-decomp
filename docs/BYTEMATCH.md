# ByteMatch progress tracking

[ByteMatch](https://bytematch.dev) tracks matching progress from objdiff reports uploaded by GitHub
Actions. Ruling [R002](rulings/R002-bytematch.md): register this project as soon as the repository
is up.

## What is in place

| Piece | File | Notes |
|---|---|---|
| Report generator | `ci/progress_report.py` | objdiff report v2 JSON; needs no ROM. Denominator: the REV 02 catalogue's 651 functions (180,702 bytes). Numerator: functions credited in `config/matches.json` (empty). Units are 64 KiB address blocks. |
| Workflow | `.github/workflows/progress.yml` | Runs on push to `main`, pull requests and on demand; uploads artifact `rev02_report` containing `report.json`. |
| Badges | `README.md` | Commented out until the first run is ingested; `measure=matched_code_percent` and `measure=matched_functions`. |

## Requirements (from the previous registrations)

- **Public GitHub repository**: self-service registration rejects private repositories.
- History comes only from **successful push runs on the default branch** that upload an artifact
  named `<version>_report`.
- The ByteMatch GitHub App (`bytematchdev`) is optional: instant ingestion and PR comments. The
  existing installation is limited to selected repositories; this one has to be added to it.

## Settings to enter

| Field | Value |
|---|---|
| name | ToeJam & Earl |
| shortName | tje |
| platform | Genesis / Mega Drive (the form's id; no Genesis project was listed on 2026-10-21) |
| defaultVersion | rev02 |
| defaultBranch | main |
| game | ToeJam & Earl |
| franchise | ToeJam & Earl |
| releaseYear | 1991 |
| workflow (advanced) | progress.yml |

## Going live

1. Audit the full history: no ROM bytes, assets, generated assembly or disassembly in any blob
   (`.gitignore` keeps `orig/`, `build/`, `.cache/`, `scratch/` out).
2. Make the repository public.
3. Register on bytematch.dev with the settings above; add the repository to the App installation.
4. Push to `main`, wait for the `progress` run, check
   `https://bytematch.dev/mschienbein/toejam-earl-decomp.json`.
5. Uncomment the badges in `README.md`.

## Registration record

(pending)
