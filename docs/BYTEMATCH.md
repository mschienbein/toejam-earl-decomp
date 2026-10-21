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

## Requirements (from the previous registrations and ByteMatch's docs)

- **Public GitHub repository**: self-service registration rejects private repositories.
- Registration itself is interactive: *Sign in with GitHub* on bytematch.dev as the repository
  admin, then **Manage → Add project**. There is no token API for adding a project.
- History comes only from **successful push runs on the default branch** that upload an artifact
  named `<version>_report`.
- The ByteMatch GitHub App (`bytematchdev`) is optional: instant ingestion and PR comments. The
  existing installation is limited to selected repositories; this one has to be added to it.
- **No Genesis platform id yet.** ByteMatch's platform table
  ([`src/lib/platforms.ts`](https://github.com/Veradictus/ByteMatch/blob/main/src/lib/platforms.ts),
  commit `33c2976`, 2026-10-21) lists `3ds dc gba gbc gb gc n64 nds pc ps ps1 ps2 ps3 psp saturn
  snes switch wii wiiu xbox x360`; the Add-project form only offers those. Register with platform
  *Not set* and ask upstream to add Mega Drive / Genesis (then set it on the manage page).

## Settings to enter

| Field | Value |
|---|---|
| repository | `mschienbein/toejam-earl-decomp` |
| name | ToeJam & Earl |
| shortName | tje |
| platform | Not set (no Genesis id yet, see above) |
| defaultVersion | rev02 |
| defaultBranch | main |
| game | ToeJam & Earl |
| franchise | ToeJam & Earl |
| releaseYear | 1991 |
| workflow (advanced) | progress.yml |

## Going live

1. ~~Audit the full history~~ — done 2026-10-21: 33 blobs, all text; no hex run of 12+ bytes
   and no printable 16-byte window from either ROM except the header fields in `config/title.json`
   (`(C)SEGA 1991.OCT`/`.JUL`) and the console string `SEGA GENESIS` in the synthetic test ROM.
2. ~~Make the repository public~~ — done 2026-10-21 (ruling R002).
3. Register on bytematch.dev with the settings above; add the repository to the App installation.
4. **Poll now** (or push to `main`), then check
   `https://bytematch.dev/mschienbein/toejam-earl-decomp.json`.
5. Uncomment the badges in `README.md`.

## Registration record

(pending: needs the owner's signed-in browser session)
