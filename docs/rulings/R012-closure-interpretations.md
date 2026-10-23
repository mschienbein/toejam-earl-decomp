# R012 — What closes the last data bytes (2026-10-23)

Context: the independent final review of checkpoint c034-final (FinalReviewA) raised two readings
of "every ROM byte is closed: code from source; data from typed, named units … rebuilt from assets":

- A1: the six bytes per revision of `FMPatchAuxiliary` (after the FM instrument patches) are a
  named unit rebuilt from an extracted asset, typed `unreferenced` with reviewed evidence (no
  reader anywhere; not a complete patch), and excluded from typed-byte credit.
- A2: forty-three text strings (2,330 bytes per revision) reach the ROM from the matched C
  functions' own string literals (MCC68K `/STRINTEXT` places a function's strings right after it),
  not from the extracted text units, which remain as checks.

User:
- A1: "Accept as closed" — a named, extracted unit with a reviewed `unreferenced` type and its
  evidence closes those bytes, as classification closes the R011 functions.
- A2: "C literals are fine" — bytes emitted from matched C source are closed by that source; the
  text assets for those strings serve as extraction checks.
