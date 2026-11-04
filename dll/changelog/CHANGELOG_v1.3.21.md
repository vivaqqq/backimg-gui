# backimg v1.3.21 Changelog — Deepseek V4 Pro Audit Fixes (Shell G1~G3 / DLL D1~D5)

- **Release date**: 2026-10-28
- **Baseline**: v1.3.20 (desktop copy preserved as-is, untouched)
- **Trigger**: the user had Deepseek V4 Pro perform a read-only audit of v1.3.20, producing 8 findings
  (3 shell items G1~G3, 5 DLL items D1~D5). Each was verified against the source code; **all 8 confirmed true**,
  and per the "fix it if true" principle all were fixed — none rejected, none compromised.

---

## I. GUI Shell (3 items)

### G1 [High priority · reported on the user's real machine] The timed rotation dropdown button face is always blank
- **Symptom**: "Every 1 minute" is checked in the dropdown menu, but the button face displays no text at all; selecting "Custom…" is likewise blank.
- **Root cause**: after `rot_ui_init` created the dropdown button, it **missed one line of handle backfill** —
  `g_cb_rot.btn = g_app.hwndRotBtn;` was never written. In `combo_state_set`, the entire
  `if (cb->btn) SetWindowTextW(...)` statement is skipped, so the button text remains the empty string from creation.
  Comparing with `g_cb_clsid` / `g_cb_img`, both have the backfill; only `g_cb_rot` was omitted.
- **Fix**: add the backfill (main.c `rot_ui_init`), rebuild the EXE.

### G2 [Medium priority] Custom minutes not synchronized with the summary
- **Root cause 1**: the custom input box (ID_EDIT_ROTMIN) has no EN_CHANGE handler — when the user edits the number manually,
  `v_rotmin` is not updated in real time, and is only read once when "Save Config" is clicked.
- **Root cause 2**: after selecting a tier, `on_rot_menu` does not call `update_summary()`.
- **Fix**: added `on_rotmin_edit_change()` (clamps 0~1440 and writes back to `v_rotmin`,
  using the same parsing as the save path; empty string treated as 0; no SetWindowText write-back, so EN_CHANGE does not loop),
  wired into the EN_CHANGE branch of WM_COMMAND; at the end of both tier-selection branches, `on_rot_menu` now calls
  `update_summary()` (the summary panel's "last refresh" timestamp follows immediately).
- **Note**: Deepseek's report claimed "the randomMin row in the summary panel does not refresh" — actual testing shows
  the summary grid has only 9 rows and **has no randomMin row** (that claim is inaccurate); this version does not add
  a summary row (adding one would require touching the right-card fill planning formula, whose layout regression risk outweighs the benefit),
  and instead delivers "feedback on tier selection" via timestamp refresh + real-time variable synchronization.

### G3 [Low priority] After saving, the input box does not write back the clamped result
- **Symptom**: entering 9999 and saving writes the correctly clamped 1440 to disk, but the box still shows 9999; it only becomes correct after reopening the program.
- **Fix**: after clamping, `save_config` writes the final value back to the input box via SetWindowText (only when the custom state is visible).

## II. DLL (5 items)

### D1 [Medium priority] Thread-level scaling cache mismatches "per-window bookkeeping"
- **Root cause**: the scaling cache is thread_local (multiple tabs on the same thread share one copy). After v1.3.20 changed window
  bookkeeping to per-window registration, `MyDestroyWindow` unconditionally called `ReleaseThreadCache()` when unregistering
  **any** window — also clearing the shared cache of tabs still alive, so other tabs' next repaint required a full
  re-decode + re-scale (a performance regression, not a crash).
- **Fix**: added a `tid` field to `MyData` (recording the owner thread at window registration); on destruction, scan once inside
  the `g_wndDataMx` lock: release the cache only if this thread **has no other registered windows**, otherwise keep it.
  Lock discipline unchanged (ReleaseThreadCache is called only after g_wndDataMx goes out of scope, and it does not nest with
  the internal g_registryMx anyway). Residual caches when threads die unexpectedly are still reclaimed by the
  SweepDeadThreadCaches / ReleaseAllCaches fallback.

### D2 [Low priority] ApplyRotateTimerAll fully reinstalls timers on every window open
- **Root cause**: every window creation goes through `LoadSettings(true)`, which unconditionally calls
  `ApplyRotateTimerAll()` — opening a new folder window resets every window's rotation countdown back to the full period
  (the comment says "after reading a config change", but in fact no check is done).
- **Fix**: build a signature from "random on + interval minutes" (no rotation = 0), store it in a `static std::atomic<int>`
  and compare with the previous value; **fully reinstall only when the signature actually changed**; on the first run (-1), always install. Each new window's own timer
  is handled by the registration path's `ApplyRotateTimer` and is unaffected.

### D3 [Low priority] RotateTimerProc unconditionally resets the timer on every tick
- **Root cause**: the comment says "reset only if the interval was changed", but in fact SetTimer is called on every tick (functionally correct, purely redundant).
- **Fix**: a `static std::atomic<int>` records the last interval; reset only when different; initial installation of the period is
  handled by ApplyRotateTimer, which shares the same config value as the callback — at most one extra set, no correctness issue.

### D4 [Low priority] RandomIndex comment inconsistent with implementation
- **Root cause**: the comment says GetTickCount64, but the actual code uses GetTickCount (changed during the v1.3.20 build
  because the toolchain headers did not expose that symbol; the comment was not updated). Wraparound does not affect seeding quality (other
  entropy sources back it up); only the comment was inaccurate.
- **Fix**: comment aligned to GetTickCount with the reason noted (zero code changes).

### D5 [Low priority] RandomIndex((int)imgSize) cast
- **Root cause**: the image list size is size_t; the `(int)` cast at call sites is undefined behavior when the count exceeds INT_MAX
  (in reality nearly impossible due to memory limits; a robustness hardening).
- **Fix**: added an `inline int RandomIndex(size_t)` overload to WinAPI.h (forwards to the int version after clamping
  to INT_MAX); both call sites (RotateTimerProc / MyCreateWindowExW) removed the cast and pass size_t directly. Self-test programs call the int version and are unaffected.

## III. Verification Record (All Actually Executed)

| Item | Result |
|---|---|
| DLL compilation | MinGW g++ -std=c++17 -O2 -Wall only the pre-existing benign warnings (#pragma comment), zero errors |
| EXE compilation | MinGW gcc -std=c11 -municode -Wall, zero errors (including added forward declarations: update_summary / on_rotmin_edit_change) |
| Byte signature | DLL contains the v1.3.21 log header (UTF-16 ×1), zero v1.3.20 remnants; EXE contains v1.3.21, zero v1.3.20 remnants; byte comparison confirmed the EXE-embedded DLL is the new v1.3.21 |
| test_random | Single thread, 41 tiers × 40000 draws, chi-square 37.2 (40 degrees of freedom, threshold 120, uniform); boundary n≤1 passed; 8 threads × 20000 draws, zero out-of-bounds |
| test_render | **116/116 all passed** (v1.3.20's 115/1 failure was due to the opaque sample image; this version added an RGBA standard sample with alpha gradients and it truly passes in full) |
| test_filter | **4/4 all passed** (v1.3.20's 1 FAIL was due to a missing sample image; after adding it, it truly passes in full) |
| test_imageheader | 13/16 (3 failures verbatim identical to v1.3.20; ImageHeader.h diffs empty against v1.3.20 — differences in expectations for purely synthetic samples, not introduced by this version) |
| test_geometry | 227/227 all passed |
| test_bugs / test_follow | All passed |
| test_leak | Three rounds of list rebuilds, memory +0MB |
| test_race_loop | 8 threads × 300 rounds × 5 runs = 12000 operations, zero errors, GDI handles bounded |
| Version resource | resource.rc FILEVERSION corrected to 1,3,21,0 (v1.3.20 had missed it, leaving 1,3,18); string version 1.3.21.0 |

## IV. Fingerprints

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `260828d95bf40795d5f73877c292dd549df607bc647d076b8eec0968b1dc49c0`
- backimg图形板_v1.3.21.exe: **1,251,835 bytes**, SHA256 `bd41bdbf51811c9858c7d9f33aaf40e133e7a7c90f49aed0c54a1b4baed21185`

## V. Notes for Users

- This version has **zero new features and zero UI layout changes**: after the G1 fix, the "timed rotation" button face will correctly display
  the current tier text (v1.3.20's blank face was a deterministic bug).
- Upgrade installation: `backimg图形板_v1.3.21.exe` and `plugin\ExplorerBgTool.dll` must be
  **overwritten as a pair**, then restart File Explorer after saving the config.
- Multi-tab open/close no longer clears each other's caches (D1); in large-image multi-tab scenarios, re-decodes drop significantly;
  opening a new window no longer resets other windows' rotation countdowns (D2).
- Real-world explorer behavior (dropdown display, timed rotation feel) needs to be confirmed by the user's own testing after installation.
