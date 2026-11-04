# backimg v1.3.20 Changelog — Random Feature Redone (A True Random Source / B Per-Window Bookkeeping / C Timed Rotation)

- **Release date**: 2026-10-28
- **Baseline**: v1.3.19 (desktop copy preserved as-is, untouched)
- **Trigger**: user feedback of "it never feels quite random". Verification confirmed three real root causes, all fixed;
  and per the user's request, a timed rotation feature was added (visual configuration in the GUI panel).

---

## I. Root Cause Verification (what was changed and why)

| No. | Root Cause | Handling |
|---|---|---|
| A | `rand()` + `srand(time(0))`: the seed is only precise to the second — explorer restarting within the same second gets the same seed and the entire "random" sequence reproduces verbatim; rand() itself is a low-quality linear congruential generator whose consecutive values are strongly correlated, and it is not thread-safe | ✅ Added `RandomIndex()` (WinAPI.cpp): mt19937 Mersenne Twister + three-way mixed seeding from a random device/system counter/ASLR address, `uniform_int_distribution` eliminates modulo bias, mutex serialization (call frequency is extremely low, no contention) |
| B | `t_myData` single slot per thread: Win10/11 multi-tab File Explorer shares one UI thread; opening a second tab on the same thread makes the new record directly overwrite the old one — both tabs draw the same image | ✅ Changed to a global registry `g_wndData` (key = window handle, protected by `g_wndDataMx`); draw sessions chained via thread_local `t_sessionHwnd`; the two locks (g_wndDataMx/g_listMx) are never nested on any path, no deadlock surface |
| C | One window one draw, welded shut after drawing: randomness happens only once at the moment the window is born; while the window lives, the image never changes | ✅ Added timed rotation: registered windows get a Windows window timer; at each tick, re-draw + InvalidateRect repaint; the interval is configured in the panel, persisted as `randomMin` (integer minutes) |

## II. Design Details of C Timed Rotation

- **Configuration**: `config.ini → [image] randomMin`, **the unit is minutes itself** (explicitly requested by the user:
  no decimals accepted, no conversion done), 0 = no rotation (default, compatible with old configs), upper limit 1440 (one day).
- **Panel control**: a custom-drawn "timed rotation" dropdown to the right of the random row (same drawing approach as "UI scaling",
  with a check mark on the current tier) plus a "Custom…" integer-minutes input box (the inner EDIT uses `ES_NUMBER`,
  so decimal points cannot be entered at the source). The whole group is greyed out when random rotation is off.
- **Tiers**: No rotation / Every 1 minute / Every 2 minutes / Every 5 minutes / Every 10 minutes / Every 30 minutes / Custom…
- **Invalid-value fallback** (config.ini treated as untrusted input, same discipline as v1.3.18's SafeToInt):
  negative/missing key → 0 (no rotation); >1440 → 1440; in custom state the input box can only contain digits.
- **Timer mechanism**:
  1) TimerProc runs on the window-owner thread (window timer WM_TIMER is posted to the window's message queue), so re-draw/repaint are naturally same-thread;
  2) Self-correcting callback: reads the current config every time — with random off or the interval cleared, the timer removes itself automatically; if the interval changed, it is reset to the new period;
  3) After `LoadSettings` reads a config change, `ApplyRotateTimerAll()` uniformly reinstalls timers for all registered windows
     (SetTimer/KillTimer with hWnd attaches to the target window's own message queue, safe to call cross-thread);
  4) When there is only one image (<2), no change at the tick; after the image list is wholesale replaced, index out-of-bounds has a clamp fallback (mechanism carried over from v1.3.16);
  5) The callback is fully wrapped in try/catch — any exception is swallowed in place, never fed into explorer's message loop.

## III. All Touchpoints Involved in the B Refactor

| Function | Change |
|---|---|
| `MyCreateWindowExW` | Register into `g_wndData[hWnd]` (no longer overwritten per thread); drawing uses RandomIndex; after registration, install the rotation timer per the current config |
| `MyDestroyWindow` | Unregister by window handle; explicit KillTimer (idempotent); the condition for zeroing the session flag/depth changed from "this thread's record matches" to "this window unregistered successfully" |
| `MyBeginPaint` | Look up the table by handle to open a session; only opens when "no in-progress session, or it is the current session window (re-entrant)", semantics identical to the old version |
| `MyEndPaint` | When session depth returns to zero, hand back `t_sessionHwnd` |
| `MyFillRect` | First take a scalar snapshot from the registry (hWnd/ImgIndex) before working; size bookkeeping and clamped out-of-bounds results are both written back to the registry; inside the lock only lookup and scalar copying are done |
| `MyCreateCompatibleDC` | Look up the table by session window to replace the recorded DC |
| `LoadSettings` | Parse randomMin (clamped); on change, ApplyRotateTimerAll |
| `InjectionEntryPoint` | Removed srand (the random source seeds itself) |

## IV. Verification Record (All Actually Executed)

| Item | Result |
|---|---|
| DLL compilation | MinGW g++ -Wall zero warnings; DLL's three copies (build/shell source/plugin) SHA256 consistent `f4fbc1ad…` |
| EXE byte signature | Contains v1.3.20, zero v1.3.19 remnants; version resource 1.3.20.0; `138d6006…` |
| test_random (new) | Single thread, 41 tiers × 40000 draws, chi-square 26.1 (40 degrees of freedom, far below the threshold 120, uniform); boundary n≤1 always 0; 8 threads × 20000 draws, zero out-of-bounds |
| test_render | 115/1 (the only failure is the alpha assertion due to the opaque sample image, a property of the test image, same as v1.3.19) |
| test_leak | Three rounds of list rebuilds, memory +0MB |
| test_race_loop | 8 threads × 300 rounds × 3 runs = 7200 operations, zero errors, GDI handles bounded |
| test_follow / test_extreme | Geometry all passed; extreme aspect ratio protection all passed, 8K filling not wrongly rejected |
| test_geometry / test_imageheader / test_filter / test_bugs | Item-by-item identical to the v1.3.19 baseline (imageheader 12/3 and filter 1 FAIL are both sample image properties; comparing against the v1.3.19 source gives the same results, not introduced by this version) |
| Self-test script | `运行自检.cmd` now includes test_random (compile + run + gate) |
| config.ini | Added `randomMin=0` key (with English and Chinese comments), synced in the plugin directory and the embedded resource |

## V. Fingerprints

- ExplorerBgTool.dll: **935,871 bytes**, SHA256 `f4fbc1ad7197c24b4b76fcd92ccd68b7…`
- backimg图形板_v1.3.20.exe: **1,250,299 bytes**, SHA256 `138d6006e3b9eaff71b1854356df740d…`

## VI. Notes for Users

- After installation, the default behavior is exactly the same as v1.3.19 (randomMin defaults to 0 = no rotation) — to enable timed image switching,
  pick a tier or enter custom minutes to the right of the random row in the panel, **save config → restart File Explorer**.
- Multi-tab/multi-window now each **draw independently**, without overwriting each other (before this version they overwrote each other).
- Timed rotation is "each window switches its own image": at each tick, every folder window re-draws independently, not everyone synchronously switching to the same image.
- Real-world explorer behavior (random feel, timed switching, multi-tab independence) needs to be confirmed by the user's own testing after installation.
