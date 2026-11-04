# Changelog CHANGELOG — v1.3.23 (Shell v1.3.23 + DLL v1.3.23)

**Release date**: 2026-10-29
**Baseline**: backimg_GUI_v1.3.22 (desktop copy preserved as-is, untouched)
**Nature**: Kimi independent re-review hardening release — the user switched back to the Kimi large model for an
independent audit with "a fresh perspective, looking only at the source code". The audit confirmed v1.3.22 was in good shape
(zero new findings on the DLL side, the cross-thread destruction hardening deemed a closed loop), and raised 5 new
shell/DLL hazards. Each was verified, and **all 5 confirmed true**, all fixed (1 P2 memory-safety item +
4 P3 items), plus one additional initial malloc null check — same root as its P2 but not listed separately in the audit report — fixed together.
**Zero UI changes, zero config changes**. Details below; the original independent audit report is kept on record by the user.

---

## K1 [P2 · Medium · Memory safety] remove_folder_section's APPENDW2 macro realloc not null-checked

- **Location**: shell main.c (roughly lines 1116-1120 in v1.3.22)
- **Problem**: `out = realloc(out, ...)` directly overwrites the original pointer. When memory is insufficient, realloc returns NULL:
  ① the original memory block is lost (leak); ② the immediately following `memcpy(out + out_len, ...)` becomes a write to a null base address,
  crashing outright. The trigger condition is that system memory happens to be exhausted when "Delete Rules" is clicked —
  low probability but the path genuinely exists. In the same file, the APPENDW macro in `ini_write_value` has a complete null check; only this spot missed it (an asymmetry defect).
- **Fix**: align the style with APPENDW — receive realloc into a temporary pointer, and on null, `free(out); return;`.
  On failure, give up as a whole: g_ini_text is preserved as-is and the ini content is unchanged (the rule is not deleted, no half-finished product left behind).

## K1b [Same root · not listed separately in the audit, fixed together] remove_folder_section's initial malloc not null-checked

- **Location**: the start of the same function
- **Problem**: `WCHAR *out = malloc(64 * sizeof(WCHAR)); out[0] = 0;` — if malloc fails,
  this is an immediate null-pointer write. Same category as K1 ("out-of-memory crash"); not listed separately in the report but verified true.
- **Fix**: added `if (!out) return;`.

## K2 [P3 · Low] folded collapse buffer malloc not null-checked

- **Location**: the "collapse empty lines" stage of the same function
- **Problem**: after `folded = malloc(...)` fails and returns NULL, `folded[j++]` is a null-pointer write.
- **Fix**: added `if (!folded) { free(out); return; }` (likewise giving up as a whole, g_ini_text untouched).

## K3 [P3 · Low] wWinMain: CreateMutexW before memset(&g_app), handle gets zeroed

- **Location**: the start of shell main.c's wWinMain
- **Problem**: the mutex handle is first stored in `g_app.hMutex`, then `memset(&g_app, 0, sizeof(g_app))`
  zeroes it to NULL — at exit, `if (g_app.hMutex) CloseHandle(...)` is skipped and the handle leaks.
  The single-instance lock function itself is unaffected (the mutex object is still held by the process); it is a logic-order error with minor consequences.
- **Fix**: move memset before CreateMutexW (clear first, then create; order restored).

## K4 [P3 · Low · Defensive] winsize.ini window width bw has no upper limit

- **Location**: where shell main.c reads the window size
- **Problem**: bw is only validated as `>= 760`; manually changing the width in winsize.ini to a huge number (e.g. 99999999)
  passes validation and creates an abnormally giant window. Inconsistent with the project's principle of "config treated as untrusted input with fallbacks".
- **Fix**: added the upper limit `bw <= 7680` (8K screen workspace width, ample margin).

## K5 [P3 · Low · Boundary] DLL MyBeginPaint: records a session even when BeginPaint returns NULL

- **Location**: DLL dllmain.cpp MyBeginPaint
- **Problem**: BeginPaint returns NULL for an invalid window (extremely rare timing such as the window already destroyed before BeginPaint);
  previously NULL was also recorded into the registry and deepened the session depth; then MyFillRect's
  `hDC == hDC` match (NULL==NULL) would treat the abnormal scene as a normal draw. Actual testing shows GDI calls on a NULL DC
  only fail without crashing, but this is a boundary hole of "treating an abnormal state as normal".
- **Fix**: condition augmented with `hDC &&` — only record/deepen the session when a valid DC is obtained.

---

## Design Trade-offs Acknowledged (not bugs, not changed)

- **Image list count inconsistency**: the shell's image management page whitelist includes .webp, the DLL's enumeration does not (GDI+ has no webp
  decoder) → when the directory contains .webp files, the shell counts N images while the DLL actually loads N-1. A known trade-off from the audit's self-assessment.
  Behavior not changed for now (changing the shell's count would affect existing users' perception); kept on record for awareness; in the future, consider marking .webp as "unsupported" in the list.

---

## Verification Record

| Item | Result |
|---|---|
| Compilation | DLL / EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL/EXE contain v1.3.23, zero v1.3.22 remnants; byte comparison confirmed the EXE-embedded DLL is the new v1.3.23 |
| test_geometry / test_render / test_filter | 227/227, 116/116, filter all passed |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | All passed / all passed / passed / 5 runs zero errors / all passed |
| test_imageheader | 13/16 (verbatim identical to all previous versions v1.3.20/21/22; ImageHeader.h diffs empty against the previous version, not introduced by this version) |

## Fingerprints

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `8B111243652DFC18FA5261F416FBF0F2DEA8A214855785EBB1A36715677166EB`
- backimg图形板_v1.3.23.exe: 1,251,835 bytes, SHA256 `22783AF3C0DE8F8FDF55F44D78C4F441A8D4279EECE794A1295EA703232A6051`
