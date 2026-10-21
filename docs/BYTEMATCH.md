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
- **No Genesis platform id upstream.** ByteMatch's platform table
  ([`src/lib/platforms.ts`](https://github.com/Veradictus/ByteMatch/blob/main/src/lib/platforms.ts),
  commit `33c2976`, 2026-10-21) lists `3ds dc gba gbc gb gc n64 nds pc ps ps1 ps2 ps3 psp saturn
  snes switch wii wiiu xbox x360`, and the form only offers those. The server stores any id, so
  the project uses `md` (shown as "md" until upstream names it); adding
  `md: "Mega Drive / Genesis"` upstream needs the user's approval (pushing to another repository).

## Registration record (2026-10-21)

Live: https://bytematch.dev/mschienbein/toejam-earl-decomp.

- Signed in through a separate Edge window with a temporary profile (CDP port 9333); the owner
  completed GitHub sign-in and two-factor authentication there. The window was closed and its
  profile deleted afterwards.
- Manage → Submit → "Your repositories" → Add (`/manage/mschienbein/toejam-earl-decomp?added=1`).
- Saved settings, verified through `/api/v1/projects?q=toejam`:

  | Field | Value |
  |---|---|
  | name / short name | ToeJam & Earl / tje |
  | platform | `md` (see above) |
  | game | ToeJam & Earl, linked to IGDB id 8389 (1991; Genesis/MegaDrive) |
  | franchise / year | ToeJam & Earl / 1991 |
  | description | Matching C decompilation of ToeJam & Earl (Sega Genesis / Mega Drive, 1991, REV 02 "Rev A"). Both retail revisions rebuild byte-identically. |
  | default branch / version | main / rev02 (auto, from the `rev02_report` artifact) |
  | workflow / artifact pattern | `progress.yml` / default `^(?<version>.+)_report$` |
  | PR comments, enabled | on, on |

- Art (hotlinked; SteamGridDB game 34242, both return 200 without a Referer):
  - icon: `https://cdn2.steamgriddb.com/icon/a84b069f0a61431cddd4784ff42737f3.png` (official, 1024×1024);
  - cover: `https://cdn2.steamgriddb.com/hero/78ccad7da4c2fc2646d1848e965794c5.jpg` (in-game scene, 1920×620).
- GitHub App: repository id 1429009537 added to the existing installation 171769441 (Only select
  repositories: toejam-earl-decomp, shiren2-decomp, kanken-training-2-decomp). The manage page
  reports "The GitHub App is installed: new runs arrive instantly and pull requests get progress
  comments".
- First ingested commit: `4debbc9` (0 / 651 functions, 0 / 180,702 code bytes).
