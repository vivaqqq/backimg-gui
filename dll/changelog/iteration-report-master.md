# ExplorerBgTool Background Plugin DLL · Master Iteration Report
**(Author original v1.1.0.1 → Fixed release v1.3.15 → Aspect-ratio-preserving scaling release v1.3.16 → Audit-fix release v1.3.17 → Audit re-verification fix release v1.3.18 → Re-audit fix release v1.3.19 → Random feature rework release v1.3.20 → Deepseek audit-fix release v1.3.21 → Deepseek re-audit hardening release v1.3.22 → Kimi independent re-audit hardening release v1.3.23 → Hunyuan 4 re-audit wrap-up release v1.3.24)**

- **Report date**: 2026-10-29 (master volume updated edition)
- **About this volume**: This volume was merged from the former split volumes 《版本迭代报告_v1.3.15.md》 and 《版本迭代报告_v1.3.16.md》,
  and fills in the v1.3.17 chapter, which previously had only a changelog and no iteration report — all historical-version modifications are retained in full, unabridged.
  The two split volumes have been deleted; this volume is authoritative.
- **Version number note** (read first to avoid confusion): early on, the DLL fix line used the version numbers v1.3.16~v1.3.20, which were later retired and archived;
  from 2026-10-27, v1.3.16/v1.3.17 were re-enabled as the **current** version numbers. v1.3.16/v1.3.17 mentioned in this volume always refer to the current versions.
- **Companion details**: `v1.3.15.md` (a 30-item item-by-item list) /
  `v1.3.16.md` / `v1.3.17.md` (an 11-issue item-by-item list) /
  `v1.3.18.md` (Qwen audit verification item-by-item list) /
  `v1.3.19.md` (Kimi re-audit verification item-by-item list) /
  `v1.3.20.md` (random feature rework item-by-item list) /
  `v1.3.21.md` (Deepseek audit, 8-item list) /
  `v1.3.22.md` (Deepseek re-audit hardening, 4-item list) /
  `v1.3.23.md` (Kimi independent re-audit, 5+1-item list) /
  `v1.3.24.md` (Hunyuan 4 wrap-up, 2+1-item list) — all in the same directory
- **Current deliverables**: the embedded resource of `..\backimg图形板_v1.3.24.exe` and `..\plugin\ExplorerBgTool.dll`
  (both SHA256 identical: `481ee8b8…`, 936,501 bytes)

---

## Chapter 1: Lineage Notes (Read This Section First)

| Lineage | DLL size | Key traits | Status |
|---|---|---|---|
| Author's zip version (DLL's own version number 1.1.0.1) | 221,696 B | Thread-safe hashMap, per-HWND multi-window tracking, folder-specific backgrounds (OnDocComplete + COM event sink), folderExt, 16-process blacklist, mt19937 true randomness | The original the user runs daily; never modified by any release |
| GitHub upstream public version | — | Single-window design (one record per thread), rand() randomness, none of the above evolutions | **The fix line's lineage baseline** |
| Fixed release v1.3.15 | 891,738 B | Upstream lineage + 30 revisions | Superseded by v1.3.16; preserved intact on the desktop as `backimg_GUI_v1.3.15\` |
| Aspect-ratio-preserving scaling release v1.3.16 | 912,114 B | + aspect-ratio-preserving scaling / follows the window live / scaling cache | Core functionality correct but 9 defects; superseded by v1.3.17, preserved intact on the desktop |
| Audit-fix release v1.3.17 | 919,310 B | Fixed all 9 issues of v1.3.16 + 2 more serious new findings | Superseded by v1.3.18, preserved intact on the desktop |
| Audit re-verification fix release v1.3.18 | 919,131 B | Qwen audit verified item by item: 4 valid fixes + 2 hardenings + 1 rejection (P0-1's mechanism does not hold) | Superseded by v1.3.19, preserved intact on the desktop |
| Re-audit fix release v1.3.19 | 921,713 B | Kimi K3.1 Pro re-audit verified item by item: all 14 valid, 13 fixed + 1 compromise (P2-4) | Superseded by v1.3.20, preserved intact on the desktop |
| Random feature rework release v1.3.20 | 935,871 B | A true random source / B per-window bookkeeping / C timed rotation (randomMin, whole minutes) | Superseded by v1.3.21, preserved intact on the desktop |
| Deepseek audit-fix release v1.3.21 | 936,501 B | Shell G1 rotation button face blank (missed backfill) / G2 live sync / G3 clamped write-back; DLL D1~D5 (thread caches released by owner bookkeeping, timers reinstalled only when the config signature changes, etc.) | Superseded by v1.3.22, preserved intact on the desktop |
| Deepseek re-audit hardening release v1.3.22 | 936,501 B | R1 cross-thread destruction judged by the registered owner tid (the only substantive hardening) + R2~R4 comment/magic-number solidification | Superseded by v1.3.23, preserved intact on the desktop |
| Kimi independent re-audit hardening release v1.3.23 | 936,501 B | K1 "delete rule" realloc missing its null check (P2) + K1b/K2 same-root null checks + K3 mutex handle zeroed + K4 window width cap + K5 sessions accept only valid DCs | Superseded by v1.3.24, preserved intact on the desktop |
| Hunyuan 4 re-audit wrap-up release v1.3.24 (current) | 936,501 B | The first P0/P2 double-zero across four audit rounds; N1 buffer enlargement aligning size conventions + N2 hook lprc null check; **the audit loop concludes here** | **Current version** |

**Honest note**: the fix line started back then from the GitHub upstream source (two unpacked directories both named explorerTool-main, easy to mix up),
so the fix line does **not** carry three of the author version's evolution paths — per-HWND multi-window, folder-specific backgrounds, and folderExt.
This is a difference inherited from the lineage baseline, not a regression introduced by the fixes (see §7 Known Limitations in `v1.3.15.md`).

---

## Chapter 2: Iteration Overview

| Version | Date | Theme | DLL size | SHA256 (first 32 hex) |
|---|---|---|---|---|
| Author original | 2021 | Original functionality | 221,696 B | — (own version number 1.1.0.1) |
| v1.3.15 Fixed release | 2026-10-23 | 30 audit revisions: white-screen resistance, crash resistance, logging, format extensions | 891,738 B | `fe8ae612c1519dcb6e37c56355f33218` |
| v1.3.16 Aspect-ratio-preserving scaling release | 2026-10-27 | Aspect-ratio-preserving scaling + follows the window live (major new feature) | 912,114 B | `a2b9a8a7db34c2dba743f0768042ce28` |
| v1.3.17 Audit-fix release | 2026-10-28 | Fixed 9 issues of v1.3.16 + 2 more serious new findings, 11 items in total | 919,310 B | `d62a5ac6467eea601e448076466fd634` |
| v1.3.18 Audit re-verification fix release | 2026-10-28 | Qwen audit verified item by item: 4 fixed + 2 hardened + 1 rejected (P0-1); cache key changed to content hash | 919,131 B | `9656e27e3bad4358fbffaa55bfc0f6f8` |
| v1.3.19 Re-audit fix release | 2026-10-28 | Kimi K3.1 Pro re-audit verified item by item: all 14 valid, 13 fixed + 1 compromise (P2-4 single-item cache residency) | 921,713 B | `8c3ef57614a11ba3767905ece70d8158` |
| v1.3.20 Random feature rework release | 2026-10-28 | A true random source (mt19937 + entropy-source mixed seeding) / B per-window bookkeeping (g_wndData) / C timed rotation (randomMin) | 935,871 B | `f4fbc1ad7197c24b4b76fcd92ccd68b7` |
| v1.3.21 Deepseek audit-fix release | 2026-10-28 | Shell G1 rotation button face blank (deterministic bug) / G2 / G3 + DLL D1~D5, 8 items in total | 936,501 B | `260828d95bf40795d5f73877c292dd54` |
| v1.3.22 Deepseek re-audit hardening release | 2026-10-29 | R1 cross-thread destruction judged by the registered owner tid + R2~R4 comment/magic-number solidification, 4 items in total | 936,501 B | `aff84abc49dea9214ff07f3bc13004fd` |
| v1.3.23 Kimi independent re-audit hardening release | 2026-10-29 | K1 realloc null check (P2) + K1b/K2/K3/K4/K5, 5+1 items in total; zero new DLL-side findings (v1.3.22 judged mechanically closed) | 936,501 B | `8b111243652dfc18fa5261f416fbf0f2` |
| v1.3.24 Hunyuan 4 re-audit wrap-up release | 2026-10-29 | The first P0/P2 double-zero across four audit rounds; N1 buffer enlargement + N2 hook null check + N3 acknowledged trade-off; the audit loop concludes | 936,501 B | `481ee8b899d4d68bebfae58e16731a2f` |
| v1.3.25 Second-granularity rotation + directory self-healing release | 2026-10-29 | D1 rotation interval down to seconds (randomVal) + D2 three-layer fix for the custom image directory intermittently not showing (N1 normalization / N2 transient retry / N3 self-healing reload) | 942,255 B | `b6a4966a772724977cfa4763e7182115` |
| **v1.3.26 Version-linked release** | 2026-10-29 | Zero DLL functionality changes; version stamp only, bumped in lockstep with shell v1.3.26 (rotation custom box moved to a sub-row) | 942,255 B | `1d4d7e882e350fd8775739fda82658a2` |
| **v1.3.27 Version-linked release** | 2026-10-29 | Zero DLL functionality changes; version stamp only, bumped in lockstep with shell v1.3.27 (Deepseek V4 Pro review, three fixes) | 942,255 B | `85e19df9fd4f36fb9e28eab7a83b4415` |
| **v1.3.28 Version-linked release (current)** | 2026-10-29 | **Zero DLL functionality changes; version stamp only, bumped in lockstep with shell v1.3.28 (Deepseek re-audit, three fixes: P1 state-flag reset / P2 dead variable / P3 re-selecting Custom preserves the entered value)** | 942,255 B | `7846debd35f5f259a75bc45a047b13db8` |

---

## Chapter 3: v1.3.15 Fixed Release (2026-10-23)

### 3.1 Phase 1 · Functionality Fixes (11 items)

9 items inherited from the 2026-10-20 fix line (reconstructed from session logs), 2 new this round:

| # | Fix | One-line description |
|---|---|---|
| 1 | Removed modal popups ×2 | The real cause of the white screen: explorer's UI thread was stalled by a "missing image directory/file" popup, so the whole desktop stopped painting |
| 2 | Removed FreeLibraryAndExitThread | On double initialization it killed itself on explorer's UI thread |
| 3 | m_isInitHook flag set earlier | Initialize only once; win or lose, it is never entered a second time |
| 4 | All-or-nothing hooking | Enabled only if all 6 hooks install; on failure roll back one by one and write init.log |
| 5 | ImgIndex out-of-bounds guard | After the image list shrank, stale indices crashed explorer (segfault observed in the self-check) |
| 6 | GetIniString uninitialized-value fix | On file read failure it would previously allocate memory using a garbage value |
| 7 | Memory leaks ×2 | GetWindowClassName leaked 1,560 bytes per window; GetWindowTitle leaked and dropped one character |
| 8 | Dirty-config guard | posType=abc no longer throws an exception that takes explorer down |
| 9 | Nested paint counter | A paint session is no longer mistakenly closed by another window |
| 10 | Format whitelist (new) | Added *.jpeg *.bmp *.gif *.tif *.tiff (webp has no decoder and is unsupported; gif uses the first frame) |
| 11 | RestoreDC by exact ID (new) | Restores precisely by the saved token number instead of popping "the most recent one" with -1 |

### 3.2 Phase 2 · Five Audit Rounds (R1 → R5, 19 revisions + 2 documentation corrections)

| Round | Auditor | Key findings | Outcome |
|---|---|---|---|
| R1 | Minimax-M3.1 Flash 《审计报告_对比上游原版》 | 3 P0s: substr without a length guard (an exception inside DllMain = taking down the host process), no process gating (resides in any process), LoadSettings fully re-decoding per window | 6 revisions (R2-1~6); additionally reproduced in testing the "disguised png corrupt file accepted into the list" issue and fixed it |
| R2 re-audit | 《审计报告_R2复审_对比上游》 | The use-after-free was only half fixed: the rebuild side got a lock, while the paint side still ran bare — the calibrated probe measured 1.3 million invalid dereferences | 4 revisions (R3-1~4): **shared_ptr credential system as the root cure** — the paint thread takes a "borrow token"; while the token is held, the object is never destroyed early |
| R3 re-audit | 《审计报告_R3复审_对比上游》 | std::map only solves iterator invalidation; concurrent writes to the container structure are still UB; two self-check delivery defects (missing sample image, exit segfault); criticized the sensitivity of our stress-test probe (valid; its calibration conclusion was cited) | 4 revisions (R4-1~4): **m_duiList removed entirely in favor of thread_local** (better than the lock-based approach the audit suggested — zero locks, zero UB, zero deadlock risk, and the DLL got 8KB slimmer) |
| R4 re-audit | 《审计报告_R4复审_对比上游》 | No new P0/P1; the per-HWND wording could cause upgrade misjudgment; two minor documentation blemishes | 3 revisions (R5-1~3, all documentation/comment level); additionally corrected one inaccurate statement of theirs |

**Methodology lesson**: the verification method itself must first be calibrated with a negative control — first catch 473 invalid dereferences with the probe on the raw-pointer structure to prove the probe works,
then measure the fixed version and get 0; only that "0" is hard evidence.

### 3.3 Phase 3 · Real-Machine Gate (passed 2026-10-23 ✅)

The audit loop left exactly one open item: on Win11, if multiple windows/tabs share the UI thread, the single-record design might lose backgrounds.

**Real-machine test on the user's installation: opened multiple folder windows, all background images displayed correctly.**
Real-machine gate passed → the single-record design is finalized; the per-HWND restoration plan is **shelved indefinitely**.
A second same-day test: rotating images, adding/removing image files, opening new windows, random rotation mode — **all displayed correctly**; all real-machine observation points closed.

### 3.4 Verification Matrix (R5 final state + real machine)

- Compiled 7 times, all zero errors; **concurrency stress test**: rebuild thread forced through the "table-swap" path 2,823 times × paint thread dereferencing the real way it fetches images
  **301 million times — zero invalid dereferences, zero dirty data**, clean exit
- Multi-format self-check **6/6 PASS**; regression self-check **9/9 PASS ×3**
- **Reproducible build**: the auditor independently reproduced a byte-for-byte identical DLL using the commands in §8 of the changelog
- Final binary inspection: all 4 COM exports present, no MessageBoxW / FreeLibraryAndExitThread, all 7 format wildcards present, x64
- **Real machine (2026-10-23)**: backgrounds correct in all windows ✅; rotation / add-remove / new windows / random rotation all correct ✅

### 3.5 Fingerprint

- ExplorerBgTool.dll: **891,738 bytes**
- SHA256: `fe8ae612c1519dcb6e37c56355f332189bba3e537bac36c4a17e3c1deacf453d`

---

## Chapter 4: v1.3.16 Aspect-Ratio-Preserving Scaling Release (2026-10-27)

### 4.1 User Requirements and How They Were Met

| # | Requirement | Outcome |
|---|---|---|
| 1 | Images must not be constrained by "must be smaller than the window"; no overflow at any window size | ✅ Modes 0~4 auto-shrink with the aspect ratio preserved when they don't fit; modes 5/6 fit naturally |
| 2 | Image follows the window live when dragging/maximizing/full-screening | ✅ Repaints as soon as the client-area size changes, no ghosting |
| 3 | **Except for the last "Scale and Fill" button, all other buttons must scale with the aspect ratio preserved — no distortion** | ✅ All seven modes preserve the aspect ratio (mode 6 keeps its original rule unchanged) |
| 4 | Do not overwrite the old version; new folder on the desktop | ✅ `backimg_GUI_v1.3.16`; v1.3.15 fingerprint unchanged |

### 4.2 Problems Before the Rework (measured, not conjectured)

A probe program tested all three GDI scaling routes:

| Route | Interpolation quality | Alpha channel | Usable as AlphaBlend source | Conclusion |
|---|---|---|---|---|
| `AlphaBlend` direct scaling | **Nearest-neighbor** (4×4→40×40 solid-color blocks) | Fully preserved | — | Enlargement gives full-screen mosaic, **unusable** |
| `StretchBlt` pre-scaling | HALFTONE high quality | **All zeroed out** (24000/24000) | **No** (returns 0) | Transparency turns into black blocks, **unusable** |
| `GDI+ DrawImage + GetHBITMAP` | **High-quality bicubic** | Fully preserved | **Yes** (returns 1) | **Adopted** |

The old DLL had **no aspect-ratio-preserving scaling implementation at all**: posType 0~4 blitted directly at the original pixel size → any large image overflowed;
the "scaling" of posType 5/6 was actually AlphaBlend's nearest-neighbor → distortion + mosaic.
Also, the scaling baseline used `GetWindowRect()` (includes borders, reads large), which was another direct cause of "overflowing the window".

### 4.3 List of Changes

| File | Change |
|---|---|
| `DrawScale.h` (new) | Pure geometry module `ComputeDrawRect()`; the scaling/positioning rules of all seven modes centralized in one place, independently unit-testable |
| `WinAPI.h/.cpp` | Added `BitmapGDI::GetScaled()`: GDI+ high-quality scaling + per-thread private LRU cache (32MB budget) |
| `dllmain.cpp` | Painting switched to the target rectangle from `ComputeDrawRect()` + `AlphaBlend` 1:1 blit; scaling baseline changed to the client area; removed the posType!=0 redraw exception |

### 4.4 New Semantics of the Seven Modes (effective v1.3.16)

| posType | Name | Scaling rule | Position |
|---|---|---|---|
| 0 | Top-left | Aspect ratio preserved (shrinks only when it doesn't fit, never enlarges) | Snapped to top-left |
| 1 | Top-right | Same as above | Snapped to top-right |
| 2 | Bottom-left | Same as above | Snapped to bottom-left |
| 3 | Bottom-right | Same as above | Snapped to bottom-right |
| 4 | Centered | Same as above | Centered |
| 5 | Scale | **Aspect-ratio-preserving scaling to fully fit the window** (follows the window in both dimensions) | Centered |
| 6 | Scale and Fill | Aspect-ratio-preserving fill of the window (short side overflows and is cropped) | Centered — **unchanged as required** |

> Want the image to grow and shrink with the window → choose **5**; want it to fill with no blank space → choose **6**.

### 4.5 Three Key Technical Pitfalls (documented in the code comments)

1. **`AlphaBlend`'s source DC must have no palette**: `CreateCompatibleDC(NULL)` → succeeds;
   `CreateCompatibleDC(GetDC(NULL))` → returns 0 and fails outright.
2. **`SelectObject(dc, NULL)` permanently ruins the bitmap's alpha compositing capability**: after unselecting it, selecting it into any DC
   makes `AlphaBlend` always return 0. Correct order: `DeleteDC` first, then `DeleteObject`.
3. **GDI+ Bitmap objects cannot be shared across threads**: 8 threads sharing one GDI+ Bitmap for scaling → segfault in stress-test round 6;
   single thread, all 40 rounds passed. **Root cure**: each thread decodes its own copy of the source bitmap; zero GDI+ objects shared between threads.
   (Also a hidden bug: `CreateDIBSection(GetDC(NULL),…)` never released the screen DC → each scaling leaked one DC handle,
   exhausting GDI resources under high frequency; the symptom was "crashes after a few rounds", the hardest to locate.)

### 4.6 Verification and Fingerprint

- Geometry unit tests **227/227**; render end-to-end old-vs-new pixel error **2.17**, alpha error **0.00**;
  real screenshots of four window types always preserved the aspect ratio, never overflowed, scaled up step by step
- The concurrency stress test was recorded back then as "40/40 passed" — **v1.3.17 proved this was an illusion** (the stress test's VEH exception handler swallowed the crashes),
  see Chapter 5
- Delivered source reproduces the build: differs from the delivered DLL by only 6 bytes (PE timestamp); EXE embedded-DLL check `a2b9a8a7…` passed
- ExplorerBgTool.dll: **912,114 bytes**, SHA256 `a2b9a8a7db34c2dba743f0768042ce28…`

---

## Chapter 5: v1.3.17 Audit-Fix Release (2026-10-28)

### 5.1 Disposition of the Audit Report's 9 Issues

| # | Severity | Issue | Disposition |
|---|---|---|---|
| 1 | Critical | Per-thread source-image cache leaked without limit; each image change permanently added 177MB of residency | ✅ Switched to content-hash keys + 64MB/16-entry LRU bound; residue went from +177MB to a **constant +66MB, zero growth across rebuilds** |
| 2 | Critical | `BitmapGDI::Size` uninitialized; 200/200 reads returned heap garbage | ✅ Added initial value `{0,0}` + `IsValid()` flag; garbage values **0/200** |
| 3 | Medium | Early return left the repaint state never updated → self-triggered repaint every frame when the directory was empty or decoding failed (CPU maxed out) | ✅ State write-back moved before all returns; **1000/1000 → 1** |
| 4 | Medium | GDI+ serial lock defined but never referenced; the serialization promised in comments was never implemented | ✅ `GdiplusMutex()` (a deliberately leaked singleton) actually wired in, covering all GDI+ call sites: decode/DrawImage/GetHBITMAP/delete |
| 5 | Medium | mode 5 "Scale" semantics changed relative to v1.3.15 without explanation | ✅ Documented in the shell behavior notes (see the shell master report) |
| 6 | Minor | DLL version shown by the shell was still 1.3.15 | ✅ `DLL_VERSION` changed to 1.3.17 |
| 7 | Minor | DLL init log header said v1.3.15 | ✅ Changed to v1.3.17 |
| 8 | Minor | Delivery package lacked the DLL build script | ✅ Added `DLL源码\build.cmd` (later corrected for encoding and path concatenation during the third-party verification round; tested to build with one click) |
| 9 | Minor | Self-check program depended on missing `probe/veh.h` and hard-coded the dev machine's paths | ✅ Ships its own `crashlog.h`, relative-path output, image paths via command-line arguments, added one-click `运行自检.cmd` |

### 5.2 Two More Serious Problems Found and Fixed Beyond the Audit Report

#### 5.2.1 🔴 MinGW `thread_local` Destructor Crash Landmine

At thread exit, the TLS destructor callback for `static thread_local ScaleCache` (containing a `std::vector`) ran on
**memory that had already been freed and filled with 0xEE by ucrt** → double free → crash. Crash stack observed in gdb:
`ntdll!RtlExitUserThread → tls_callback → run_dtor_list → ScaleCache::~ScaleCache → vector::~vector`,
with the freed pointer = `0xfeeefeeefeeefeee`. Independent minimal reproduction: `thread_local` holding a `std::vector` always crashes; holding only an `int` always passes.

**Why v1.3.15/v1.3.16 never exposed it**: the other `thread_local MyData` in v1.3.16 was all POD with no destructor;
the earlier claim of "40/40 zero crashes" was an **illusion** produced by the stress test's VEH swallowing the crashes.

**Fix**: `thread_local` now stores only a raw pointer (trivial destructor, no TLS callback registered); cache objects are heap-allocated and registered in a global table;
caches for this thread are released proactively on window destruction, with a backstop releasing everything at DLL unload.

#### 5.2.2 🟠 GDI Handle Exhaustion (caches not released at thread exit)

Exposed after fixing 5.2.1: thread exit no longer destructs caches automatically; stress-test handles went 2583 → 5161 → 7739 → **9999, hitting the system cap**,
after which `CreateCompatibleDC` failed and backgrounds could not be painted.

**Fix**: added **dead-thread cache sweeping** — the registration table records each cache's owner thread id, liveness is checked periodically with `GetExitCodeThread`,
and exited threads are released together with their caches. Measured handles went from monotonic growth to **bounded sawtooth fluctuation (629~1959, not monotonically growing)**.

> Pitfall 1: liveness was first checked with `WaitForSingleObject(h,0)==WAIT_TIMEOUT`, but **a thread waiting on itself returns immediately**,
> so the live thread misjudged itself as dead and deleted the cache it was using → heap corruption → `abort()` (abort goes through raise() and bypasses the VEH,
> so the crash recorder showed nothing). Switched to `GetExitCodeThread` + a direct pass-through for the current thread.
> Pitfall 2: sweeping was initially inside `GetThreadCache()`, which is called while `GetScaled` **holds the GDI+ lock** →
> sweeping deleted a cache → `~ScaleCache` then took the same lock → `std::mutex` is non-reentrant → **self-deadlock**
> (manifested as the stress test hanging, not crashing). Sweeping has been moved to before taking the lock.

#### 5.2.3 🟡 New Exposure Surface: Corrupt Images Fed Straight to GDI+

v1.3.15 had "stumbled into" blocking corrupt files because it happened to `delete[]` the buffer before `SHCreateMemStream`; v1.3.16 kept the full bytes,
so a corrupt PNG would be genuinely parsed by GDI+. **Fix**: added lightweight image-header parsing in `ImageHeader.h` (PNG chunk walk + IHDR CRC,
JPEG SOI/EOI/SOF). **The core rule is "never veto on its own"** — rejection only when "confirmed corruption" can be proven (chunk out of bounds / wrong length or CRC /
absurd dimensions); anything unreadable is left to GDI+ to adjudicate, avoiding false rejections.
(Pitfall: the first version hard-coded "at most 64 PNG chunks" and misjudged a user's legitimate PNG with 212 IDAT chunks as corrupt; the count limit was removed and a regression test added to lock the behavior in.)

### 5.3 Post-Delivery Third-Party Verification Round (2026-10-28)

- The external auditor issued 《backimg_v1.3.17_验证报告》: **all 9 issues confirmed fixed at root-cause level, no need to touch the DLL itself**;
  the auditor fixed two script defects on the spot — `build.cmd` (encoding / path concatenation) and `运行自检.cmd` (wildcard-based location of the source directory)
- The two leftover "1-minute" small items were completed: `test_race_loop.cpp` changed to print per round (the GDI handle sawtooth curve directly visible:
  325 → 987 → 657); the delivery document's "handles stable" wording changed to "**bounded, not monotonically growing**"
- **Reproducible build re-check**: rebuilt from the delivered source with one command; byte-level comparison against the delivered DLL showed **exactly 6 bytes of difference, all PE timestamps**
- The auditor recorded one narrow hazard **not recommended to change**: the sweep's liveness check relies on thread ids, so in theory id reuse could cause misjudgment —
  measured 0 reuses across 400 short-lived threads, documented for the record

### 5.4 Verification Record

| Test | Result |
|---|---|
| Geometry unit tests (aspect ratio / distortion / overflow / degenerate inputs) | **227/227** |
| Image header parsing (39 real PNGs + 15 malformed samples) | **54/54** |
| Corrupt-file end-to-end filtering (text / truncated / normal / nonexistent) | **4/4** |
| Size garbage values | **0/200** (200/200 before the fix) |
| Endless repaint invariant | **1000 → 1** |
| Render end-to-end (old vs. new pixel comparison) | mean difference 2.03, **alpha difference 0.00** |
| Window follow (aspect ratio / no distortion / no overflow / AlphaBlend return values) | All passed |
| Concurrency stress test (8 threads × 300 iterations × 12 rounds, forcing sweeps and LRU evictions) | **12/12 zero crashes**; GDI handles sawtoothed but bounded (629~1959, not monotonically growing) |
| Memory upper bound (39 images × 3 rebuild rounds) | Residue a **constant +66 MB, +0 MB per round** |

### 5.5 Fingerprint

- ExplorerBgTool.dll: **919,310 bytes**
- SHA256: `d62a5ac6467eea601e448076466fd634…` (full value in `..\交付说明.txt`)

---

## Chapter 6: v1.3.18 Audit Re-Verification Fix Release (2026-10-28)

### 6.1 Background: Item-by-Item Verification of a Third-Party Audit

After v1.3.17 was delivered, Qwen 3.8 Flash performed a memory-safety audit of the DLL source
(《backimg_DLL内存安全审计报告_v1.3.17.txt》). This release worked as follows:
**each finding was manually verified against the source; valid ones were fixed, invalid ones were rejected in writing** — the audit report's conclusions
were not accepted as fact directly. Verification and fixes were done by WorkBuddy (GLM).

### 6.2 Master Table of Verification Conclusions

| Report item | Verification conclusion | Disposition |
|---|---|---|
| P0-1 Scaling cache destroying GDI handles across threads | **Mechanism does not hold** (see 6.4) | Not fixed; the real detach-path problem it surfaced was fixed under P1-4 |
| P0-2 Scaling cache keyed by ownerId, all miss after a list rebuild | **Partially valid** — the claim "misses on every repaint, almost never hits" is exaggerated (LoadSettings only rebuilds when the file set changes; between rebuilds hits work normally); "the whole batch misses after a rebuild, wasting one recomputation round" is valid | ✅ Cache key changed to content hash |
| P1-3 GetScaled failure branch misses DeleteObject(hb) | **Valid** (CreateCompatibleDC failure branch + SelectObject result unchecked) | ✅ Fixed |
| P1-4 DllMain detach takes a lock / deletes; hooks never removed | **Valid** | ✅ Fixed ("pin the DLL" approach) |
| 四-1 ClassFactory::AddRef double-writes g_cDllRef | **Valid**, but the report's suggested "one-line fix" has a trap | ✅ Fixed (factory switched to heap allocation) |
| 四-2 SetSite does not null-check pUnkSite | **Valid** (the COM unload convention calls SetSite(NULL); the original code made a virtual call on a null pointer) | ✅ Fixed |
| 四-3 t_myData is a single slot; a second window on the same thread overwrites it | Valid, but consistent with the author's original behavior, not a regression | Not fixed; explicitly listed as a known limitation |
| 四-4 WriteInitLog CREATE_ALWAYS truncates the log | Valid, but called only once per process; the "reset per session" semantics is reasonable | Not fixed |
| 四-5 GetCurDllDir does not check the return value / npos | Valid (defensive) | ✅ Hardened in passing |

### 6.3 Fix Details

| # | Fix | Description |
|---|---|---|
| 1 | **GetScaled failure-branch handle leak (P1-3)** | Added DeleteObject(hb) to the CreateCompatibleDC failure branch; SelectObject's return value is null-checked, with DeleteDC+DeleteObject on failure. Under handle pressure, once this path was hit the background could never be painted again (failure self-lock); it was the one that escaped v1.3.17's handle fixes |
| 2 | **Scaling cache key ownerId → content hash (P0-2)** | Keys of Find/Put/Drop and the Entry field unified to m_hash (FNV-1a 64); NextScaleOwnerId/g_ownerMx/m_ownerId deleted as a whole; ~BitmapGDI no longer clears the cache (with content addressing, object destruction does not mean the content is invalid); memory kept in check by the 32MB LRU budget. The cache keeps hitting normally after list rebuilds |
| 3 | **DLL unload path hardening (P1-4)** | DllCanUnloadNow always returns S_FALSE — the 6 hook trampolines point into this DLL's code pages; once FreeLibrary'd, any CreateWindowExW/BeginPaint/FillRect in explorer would jump into unmapped memory; pinning the DLL is standard practice for shell extensions. The detach branch dropped lock_guard(g_listMx) and ReleaseAllCaches(): taking multiple locks under the loader lock is a deadlock risk, and ReleaseAllCaches would delete whole cache objects of **still-live threads** while those threads' thread_local pointers still point at them — the next GetScaled would be a dangling-pointer UAF (a real problem mentioned in passing in the P0-1 section and confirmed valid on verification) |
| 4 | **ClassFactory reference-count fix (四-1)** | AddRef's second line also incremented g_cDllRef (it should have been m_ref). **It could not be settled with the report's "one-line change"**: the factory was originally a static object; once the count was fixed, the delete this at Release reaching zero would land on static storage (UB crash) — so DllGetClassObject was changed to heap-allocate the factory, closing the reference-count loop; added virtual ~ClassFactory() (eliminating -Wdelete-non-virtual-dtor) |
| 5 | **SetSite null check (四-2)** | If pUnkSite is NULL (called per the COM unload convention), return S_OK directly |
| 6 | **GetCurDllDir defensive hardening (四-5)** | GetModuleFileNameW failure/truncation or a path without a backslash now returns an empty string, no longer gambling on substr(npos) |

### 6.4 Why P0-1 Was Not Fixed (rejection rationale, kept for the record)

The report's core assertion: "an entry in thread A's cache may be taken by thread B at the same time, and when A evicts it, it deletes the GDI
handle B is using." Line-by-line verification shows this timeline cannot happen:

1. The cache is thread_local (g_threadCachePtr); Find only looks at the thread's own cache —
   thread B can never fetch thread A's entries; B calling GetScaled creates B's own
   ScaledBitmap (handles created on thread B).
2. The shared_ptr returned by GetScaled is only handed to same-thread callers (a MyFillRect local);
   "another thread holding a copy of the same ScaledBitmap" never exists.
3. While MyFillRect holds the token (around AlphaBlend), this thread cannot enter GetScaled
   and trigger LRU eviction — within one thread, use and eviction are naturally serialized.
4. Cross-thread destruction has only three routes: sweeping dead-thread caches (dead threads aren't using them), a thread releasing
   its own cache on window destruction (same thread), and ReleaseAllCaches at detach (the real problem, fixed under P1-4).
   The shared_ptr reference count guarantees in-flight users hold a token; the cache being deleted
   only decrements the count, never destroys the object.

That is, the scenario the report describes requires the premise "cache entries shared across threads", and that premise does not hold.
Section 5 of the report itself admits the existing stress test cannot detect it — because it simply never happens.

### 6.5 Verification Record (measured on MinGW-w64 g++ -O2)

| Test | Result |
|---|---|
| test_render (render/alpha/8-thread concurrency) | Passed; same-size old-vs-new mean pixel error 1.33, alpha error 0.00 |
| test_leak (construct-scale-destroy × 3 rounds) | Passed; **rounds 2/3 grew +0MB — direct evidence that the hash-keyed cache hits across list rebuilds** |
| test_race_loop (8 threads × 300 iterations × 5 rounds) | Passed; ok=12000/12000, err=0, GDI handles 989~1601 stable sawtooth, no monotonic growth |
| Compile | -Wall zero errors (only MinGW's inherent #pragma comment ignored-warning); the DLL's three copies (build/shell source/plugin) have identical SHA256; the EXE embedded-resource byte signature verified as v1.3.18 |

### 6.6 Fingerprint

- ExplorerBgTool.dll: **919,131 bytes**
- SHA256: `9656e27e3bad4358fbffaa55bfc0f6f8e49af8ff72a3a6980adf0d4d33217b71`
- Companion shell: backimg图形板_v1.3.18.exe (1,231,282 bytes, SHA256 `70b62c49…`)

---

## Chapter 7: v1.3.19 Re-Audit Fix Release (2026-10-28)

### 7.1 Background: The Second Third-Party Audit (Kimi K3.1 Pro)

After the Qwen verification round, the user switched to Kimi K3.1 Pro for another audit. This report was significantly higher quality:
all 14 findings came with specific line numbers and trigger conditions, and after verification against the source **all were valid**
(P2-4 was "mechanism valid but the fix requires trade-offs"). The audit also re-confirmed that the 7 fixes of v1.3.18 were effective.

### 7.2 Master Table of Verification Conclusions and Dispositions

| ID | Issue | Disposition |
|---|---|---|
| P1-1 | GetSourceLocked released the IStream early (GDI+ lazy decoding requires the stream to live until bitmap destruction) | ✅ Fixed: stream release moved to after delete decoded, unified order across all branches |
| P1-2 | Plain-bool race in initialization + GdiplusStartup return value ignored | ✅ Fixed: once_flag + atomic; on startup failure the background is cleanly disabled |
| P1-3 | "Scale and Fill" target bitmap had no cap (a 100000x1 image → a 434GiB request), ScaleRound int overflow UB | ✅ Fixed: ScaleRound clamping + a 64-million-pixel (256MB) target cap |
| P1-4 | No budget for raw bytes, bad_alloc could escape through the COM/Hook boundary | ✅ Fixed: 256MB per-file cap + exception boundaries at five places |
| P2-1 | MH_EnableHook failure left a "half-installed set of enabled hooks" | ✅ Fixed: Disable + Remove rolled back everything, with logging |
| P2-2 | Data race between config scalars and window-creation/paint threads | ✅ Fixed: three items switched to std::atomic |
| P2-3 | Real decoding never checked GetLastStatus/DrawImage status | ✅ Fixed: decode and draw status fully checked; failures never enter the cache |
| P2-4 | A single entry over budget could stay resident in the cache indefinitely | ⚠️ Compromise: per-item size caps (scaled 256MB / source 512MB) + explicit documentation; no forced eviction — that would re-decode and re-render large images every frame, a CPU disaster |
| P2-5 | Old list not cleared after the image directory goes bad | ✅ Fixed: aligned with the "directory empty" branch |
| P2-6 | Non-random mode stopped at the first bad image; switching to random didn't rebuild the list | ✅ Fixed: break only after finding the first valid image + s_listIsFullSet state |
| P2-7 | Registration path buffer uninitialized / return value ignored; unregister path had one extra backslash | ✅ Fixed: all three spots |
| P3-1 | JPEG prober returned IMG_OK without SOF; missing EOI hard-judged as dead | ✅ Fixed: no SOF handed to GDI+; missing EOI relaxed to UNKNOWN |
| P3-2 | std::wstring inside DllMain could throw under the loader lock | ✅ Fixed: switched to wcsrchr + _wcsicmp, pure Win32 |
| P3-3 | QueryInterface didn't clear the output parameter on failure | ✅ Fixed: null check at entry + clearing *ppv on failure |

### 7.3 Verification Record

| Item | Result |
|---|---|
| Compile | MinGW g++ -Wall zero warnings (only MinGW's inherent #pragma comment note); the DLL's three copies have identical SHA256 |
| test_render | 115 passed / 1 failed (an alpha assertion on an opaque sample screenshot — a sample-image property, not a code defect); old-vs-new error 1.33 / alpha 0.00 |
| test_leak | Three rebuild rounds +0MB, cache hits normal |
| test_race_loop | 8 threads × 300 iterations × 3 rounds = 7200 runs, zero errors; exit code 0 (from this release on the exit code is truthful) |
| test_follow | 7 modes × multi-window geometry checks all passed |
| test_extreme (new) | 100000x1, 1x100000, 999999x999999 × 7 modes all within the cap; 8K normal fill not falsely rejected |
| EXE byte signature | Contains v1.3.19, zero v1.3.18 residue; FileVersion 1.3.19.0 |

Additionally, per Section 6 of the report, the self-check program's own problems were fixed: fake exit codes in three tests
(`_exit(0)` short-circuiting the real result tally) and two CP_ACP/CP_UTF8 code-page mismatches — from this release on,
"self-check passed" regains its value as an automatic gate.

### 7.4 Fingerprint

- ExplorerBgTool.dll: **921,713 bytes**, SHA256 `8c3ef57614a11ba3767905ece70d8158…`
- backimg图形板_v1.3.19.exe: 1,233,842 bytes, SHA256 `f38f03b4191fa5e17bf0a0a4031b2533…`

---

## Chapter 8: v1.3.20 Random Feature Rework Release (2026-10-28)

### 8.1 Background

Users reported "it never feels quite random". Verification confirmed three real causes (none hallucinated):
① the `srand(time(0))` seed is only precise to the second, so an explorer restart within the same second reproduced the whole sequence;
② `t_myData` is a per-thread single slot, so multiple tabs on the same thread overwrote each other's records — two tabs painted the same image;
③ one draw per window, frozen once drawn, with no timed rotation. This release fixes the first two with A/B, and adds rotation with C.

### 8.2 The Three Changes

| Item | Content |
|---|---|
| A True random source | `RandomIndex()` (WinAPI.cpp): mt19937 + three-way mixed seeding from random_device/GetTickCount/ASLR addresses, uniform_int_distribution eliminating modulo bias, serialized by a mutex; `srand` removed |
| B Per-window bookkeeping | `t_myData` single slot → global `g_wndData` (map<HWND,MyData>, protected by g_wndDataMx); paint sessions threaded through a thread_local `t_sessionHwnd`; the two locks never nest on any path; snapshot-style access, only lookups and scalar copies inside the lock |
| C Timed rotation | `randomMin` (whole minutes, 0 = no rotation, clamped to 1440); registered windows get window timers, RotateTimerProc re-draws + InvalidateRect; the callback self-corrects (random off / interval changed follow automatically); after LoadSettings, ApplyRotateTimerAll uniformly reinstalls across all windows; the callback is wrapped in try/catch throughout |

### 8.3 Panel-Side Companion Changes

To the right of the random row, added a "Timed Rotation" owner-drawn dropdown (new_btn_dropdown + TrackPopupMenu,
same lineage as "Interface Scaling", current notch check-marked) + a "Custom…" whole-minutes input box (ES_NUMBER,
decimal points cannot be typed); with random off the whole group is grayed out; illegal values have a fallback (<0→0, >1440→1440).

### 8.4 Verification Record

| Item | Result |
|---|---|
| Compile | DLL/EXE zero warnings; the DLL's three copies SHA256 identical `f4fbc1ad…` |
| test_random (new) | 41 bins × 40,000 draws, chi-square 26.1 (df=40, uniform); n≤1 boundary always 0; 8 threads × 20,000 draws, zero out-of-bounds |
| test_render / test_leak / test_race_loop | 115/1 (sample-image property) / three rounds +0MB / 7200 runs, zero errors |
| test_geometry / test_imageheader / test_filter / test_bugs / test_follow / test_extreme | Item-by-item identical to the v1.3.19 baseline (the few failures are sample-image properties; cross-checked to confirm they were not introduced this release) |
| EXE byte signature | Contains v1.3.20, zero v1.3.19 residue; version resource 1.3.20.0 |
| config.ini | Added the `randomMin=0` key (plugin directory and embedded resource in sync) |

Also: test_random has been added to `自检程序\运行自检.cmd` (compile + run + gate).

### 8.5 Fingerprint

- ExplorerBgTool.dll: **935,871 bytes**, SHA256 `f4fbc1ad7197c24b4b76fcd92ccd68b7…`
- backimg图形板_v1.3.20.exe: 1,250,299 bytes, SHA256 `138d6006e3b9eaff71b1854356df740d…`

---

## Chapter 9: v1.3.21 Deepseek Audit-Fix Release (2026-10-28)

### 9.1 Background

The user had Deepseek V4 Pro perform a read-only audit of v1.3.20, producing 8 findings (shell G1~G3,
DLL D1~D5). Each was verified against the source; **all 8 were valid** and, per the "fix-what-is-valid" principle, all were fixed,
none rejected, none compromised. This release has zero new features and zero UI layout changes. Details in
`v1.3.21.md`.

### 9.2 Fix Details (5 DLL-side items)

| ID | Issue | Fix |
|---|---|---|
| D1 [Medium] | The scaling cache is thread_local (multiple tabs on one thread share one copy), yet MyDestroyWindow unconditionally called ReleaseThreadCache when deregistering any window — surviving tabs had to re-decode and re-scale the whole image on their next repaint (performance degradation, not a crash) | MyData gained a tid field (owner thread recorded at window registration); on destruction, scan inside the lock and release only when this thread has no other registered windows; lock discipline unchanged (ReleaseThreadCache called only after g_wndDataMx leaves scope); unexpected thread disappearance is still backstopped by SweepDeadThreadCaches / ReleaseAllCaches |
| D2 [Low] | OnWindowLoad ran LoadSettings on every window open → unconditionally ApplyRotateTimerAll — opening a new folder window reset every window's rotation countdown back to a full cycle (the comment said "after config changes" but nothing was actually checked) | A signature assembled from "random-on + interval minutes" (no rotation = 0) stored in a static std::atomic<int>; full reinstall only when the signature truly changed; first time (-1) always installs; a new window's own timer is handled by the window-registration path |
| D3 [Low] | RotateTimerProc's comment said "re-set only when the interval changed", but in fact SetTimer ran on every tick (functionally correct, pure redundancy) | A static std::atomic<int> remembers the last interval; re-set only when different; initial installation is ApplyRotateTimer's job; at worst one extra set, no correctness issue |
| D4 [Low] | RandomIndex's seeding comment said GetTickCount64 while the code used GetTickCount (changed at compile time in v1.3.20, comment not updated; wraparound harmless, only the comment is inaccurate) | Comment aligned with the reason noted (zero code changes) |
| D5 [Low] | The size_t→int cast in RandomIndex((int)imgSize) is undefined behavior if the count exceeds INT_MAX (nearly impossible in reality; robustness hardening) | WinAPI.h gained an inline int RandomIndex(size_t) overload (clamped to INT_MAX and forwarded); the cast removed at both call sites; the self-check program uses the int version and is unaffected |

The 3 shell-side items (G1 blank button face / G2 live input-box sync / G3 clamped write-back on save)
are covered in the corresponding chapters of `../../shell/iteration-report-master.md`.

### 9.3 Verification Record

| Item | Result |
|---|---|
| Compile | DLL/EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL contains the v1.3.21 log header (UTF-16 ×1), zero v1.3.20 residue; the EXE's embedded DLL confirmed by byte comparison to be the new v1.3.21 |
| test_random | 41 bins × 40,000 draws, chi-square 37.2 (df=40, uniform); n≤1 boundary passed; 8 threads × 20,000 draws, zero out-of-bounds |
| test_render / test_filter | **116/116 and 4/4, all passed** — this release added the RGBA standard sample with an alpha gradient and the missing sample images; v1.3.20's "sample-image property failure" no longer reproduces |
| test_imageheader | 13/16 (the 3 failures are character-for-character identical to v1.3.20; ImageHeader.h diffs empty against the previous release — not introduced here) |
| test_geometry / test_bugs / test_follow / test_leak / test_race_loop | 227/227 / all passed / all passed / three rounds +0MB / 12000 runs, zero errors |
| Version resource | resource.rc FILEVERSION corrected to 1,3,21,0 (v1.3.20 had missed it, stuck at 1,3,18) |

### 9.4 Fingerprint

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `260828d95bf40795d5f73877c292dd549df607bc647d076b8eec0968b1dc49c0`
- backimg图形板_v1.3.21.exe: 1,251,835 bytes, SHA256 `bd41bdbf51811c9858c7d9f33aaf40e133e7a7c90f49aed0c54a1b4baed21185`

---

## Chapter 10: v1.3.22 Deepseek Re-Audit Hardening Release (2026-10-29)

### 10.1 Background

The user had Deepseek V4 Pro perform a read-only re-audit of v1.3.21. Conclusion: all 8 fixes of v1.3.21
are valid and properly in place, with no regressions; 4 new latent hazards were found. Verified one by one — **all 4
valid**, all handled (1 substantive hardening + 1 magic-number replacement + 2 comment solidifications); normal-path
behavior is identical to v1.3.21. Details in `v1.3.22.md`.

**Correction to a citation in the re-audit report (for the record)**: Item 1 claimed "MSDN explicitly allows cross-thread
DestroyWindow" — MSDN in fact explicitly **does not** (cross-thread calls fail). The mechanism still holds, though:
this plugin's hooks do their bookkeeping **before** the real destruction, so even if a cross-thread call's real destruction fails, the pollution has already happened.

### 10.2 Fix Details

| ID | Issue | Handling |
|---|---|---|
| R1 [Medium] | MyDestroyWindow matched the **destroying thread's** GetCurrentThreadId() against the **owner thread's** tid in the registration table — on cross-thread destruction it misjudged "this thread has no windows", so ReleaseThreadCache released the wrong thread's cache and the session reset cleared the wrong thread's thread_local | Go by the owner tid recorded at registration: only when owner == current thread (the normal path; byte-for-byte identical behavior to v1.3.21) are the cache released / the session cleared; cross-thread only deregisters + KillTimer, leaving cache and session state untouched (orphaned caches are reclaimed by the SweepDeadThreadCaches / ReleaseAllCaches backstop) |
| R2 [Low] | The threadEmpty snapshot was computed inside the lock but used outside it (a TOCTOU pattern); safety depended on "same-thread Create/Destroy being serialized by the message queue" | No nested locks introduced (that would violate the lock discipline); the invariant is captured in a comment to warn future maintainers |
| R3 [Low] | RandomIndex(size_t)'s clamp used the magic number 2147483647u, depending on unsigned 32-bit | Changed to (size_t)INT_MAX (WinAPI.h gains <climits>) |
| R4 [Low] | s_rotTimerCur globally shared "last interval" (the re-audit itself judged it not a bug, no change required) | Semantics and maintenance boundary captured in a comment (must be updated in sync if intervals ever become per-window config) |

### 10.3 Verification Record

| Item | Result |
|---|---|
| Compile | DLL/EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL contains the v1.3.22 log header, zero v1.3.21 residue; the EXE's embedded DLL confirmed by byte comparison to be the new v1.3.22 |
| test_geometry / test_render / test_filter | 227/227, 116/116, 4/4, all passed |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | All passed / all passed / three rounds +0MB / 12000 runs zero errors / chi-square uniform + 8 threads zero out-of-bounds |
| test_imageheader | 13/16 (the 3 failures are character-for-character identical to v1.3.20/21; ImageHeader.h diffs empty against the previous release) |
| Sample-image environment | The desktop 111222333 sample-image directory was cleaned out yet again (unrelated to the code); retested after rebuilding the RGBA standard sample (307 B) |

### 10.4 Fingerprint

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `aff84abc49dea9214ff07f3bc13004fdb66176b4e127d5a867a5c9a35726516a`
- backimg图形板_v1.3.22.exe: 1,251,835 bytes, SHA256 `53e3982362c1573b1aa2add9913ec1ac07e070a5c9bf873131f0a88ceeb2c069`

---

## Chapter 11: v1.3.23 Kimi Independent Re-Audit Hardening Release (2026-10-29)

### 11.1 Background

The user switched back to the Kimi model for an independent audit with a "fresh perspective, source code only"
(no prior audit conclusions, no iteration reports). Conclusion: **zero new DLL-side findings** — v1.3.22's cross-thread destruction hardening
was independently judged "mechanically closed"; the paint path's dual caps, random source, COM layer, and cache budgets all passed
verification; the shell newly raised 5 hazards (1 P2 + 4 P3). Verified one by one — **all valid**,
all fixed; additionally fixed 1 initial-malloc null check that the audit did not list separately but shares the same root as its P2.
Normal-path behavior is identical to v1.3.22. Details in `v1.3.23.md`.

### 11.2 Fix Details (numbered K*, including K1b which the audit did not list separately)

| ID | Severity | Location | Issue | Handling |
|---|---|---|---|---|
| K1 | P2 | Shell remove_folder_section APPENDW2 macro | The realloc return value directly overwrote the original pointer — on low memory, a null-pointer write crash + the original block leaked (the same file's APPENDW has a null check; only this spot was missed) | Aligned with the APPENDW pattern: a temporary pointer receives realloc; on null-check failure, free + return; g_ini_text left intact (rule not deleted, no half-applied state) |
| K1b | Same root | Same function's initial malloc | `out[0]=0` executed without a null check after malloc (not listed separately by the audit, verified valid) | Added `if (!out) return;` |
| K2 | P3 | Same function's folding buffer | folded malloc not null-checked; `folded[j++]` a null-pointer write | Added the null check; on failure abandon the whole operation |
| K3 | P3 | Shell wWinMain | CreateMutexW ran before memset(&g_app), so the handle was zeroed → no CloseHandle at exit (handle leak; single-instance functionality unaffected) | memset moved before CreateMutexW |
| K4 | P3 | Shell winsize.ini reading | The window width bw was checked only for a lower bound, no upper bound; a hand-edited huge number could create a giant window | Added an upper bound of 7680 (8K screen width) |
| K5 | P3 | DLL MyBeginPaint | When BeginPaint returned NULL (an extremely rare timing with an invalid window), the session was still recorded and the depth incremented; the NULL==NULL match treated the anomalous case as a normal paint | Condition extended with `hDC &&` — only a valid DC records/increments a session |

**Acknowledged design trade-off (unchanged)**: the shell's image-management page whitelist includes .webp while the DLL's enumeration does not
(GDI+ has no webp decoder) → when the directory contains .webp files, the shell counts N images but the DLL actually loads N-1.
The audit itself flagged it as a known trade-off; documented for the record; a future option is labeling such entries "unsupported" in the list.

### 11.3 Verification Record

| Item | Result |
|---|---|
| Compile | DLL/EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL/EXE contain v1.3.23, zero v1.3.22 residue; the EXE's embedded DLL confirmed by byte comparison to be the new v1.3.23 |
| test_geometry / test_render / test_filter | 227/227, 116/116, all passed |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | All passed / all passed / passed / 5 rounds zero errors / all passed |
| test_imageheader | 13/16 (the 3 failures are character-for-character identical to v1.3.20/21/22; ImageHeader.h diffs empty against the previous release) |

### 11.4 Fingerprint

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `8B111243652DFC18FA5261F416FBF0F2DEA8A214855785EBB1A36715677166EB`
- backimg图形板_v1.3.23.exe: 1,251,835 bytes, SHA256 `22783AF3C0DE8F8FDF55F44D78C4F441A8D4279EECE794A1295EA703232A6051`

---

## Chapter 12: v1.3.24 Hunyuan 4 Re-Audit Wrap-Up Release (2026-10-29)

### 12.1 Background

The user switched to Hunyuan 4 for the fourth independent audit (fresh perspective, read-only read-through of the source). Conclusion: **zero P0,
zero P2** (the first double-zero across four rounds), only 3 P3s. Verified one by one: N1/N2 valid (N1's trigger condition
verified as even stricter than the report stated, see below); N3 is an acknowledged design trade-off, left unchanged. **With this round's fixes, the audit loop
concludes** — across four rounds P0 went 1→0→0→0 and P2 went many→1→1→0, monotonically converging; each round confirmed the previous round's fixes
caused no regressions, with none of the "fix A, break B" cycling signature; the four models' independent audit conclusions corroborate one another. Details in
`v1.3.24.md`.

### 12.2 Fix Details

| ID | Severity | Location | Issue | Handling |
|---|---|---|---|---|
| N1 | P3 | Shell on_rule_add + read side ×2 | imgc[1024] sits right at the theoretical limit (the img source is 4096 WCHAR); the write side counts 1024 bytes / the read side counts 1024 characters — asymmetric conventions | Verification note: img comes from FindFirstFile, and NTFS caps file names at 255 characters → worst-case UTF-8 is 1020+NUL=1021 ≤ 1024, so **a legitimate file name cannot actually trigger truncation**; imgc was still enlarged to 12288 and the read side img→12288/imgw→4096 to align the conventions, eliminating the edge-hugging magic numbers |
| N2 | P3 | DLL MyFillRect | Inside the global hook, `lprc->left` was dereferenced without a null check — if a caller illegally passed NULL and the original function didn't crash, the hook became the new crash point | `if (savedDC)` → `if (savedDC && lprc)`; the hook is more conservative than the original function |
| N3 | Trade-off | WinAPI.h BitmapGDI | The image collection's resident raw bytes have no total cap (N images = N×file size); there is a 256MB per-file cap and a bad_alloc backstop so it doesn't crash | Acknowledged, unchanged: adding a total soft cap would require refusing loads or on-demand decoding; the latter sacrifices the thread-safety model — not worth the cost |

### 12.3 Verification Record

| Item | Result |
|---|---|
| Compile | DLL/EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL/EXE contain v1.3.24, zero v1.3.23 residue; the EXE's embedded DLL confirmed by byte comparison to be the new v1.3.24 |
| test_geometry / test_render / test_filter | 227/227, 116/116, all passed |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | All passed / all passed / passed / 5 rounds zero errors / all passed |
| test_imageheader | 13/16 (character-for-character identical across all previous releases; ImageHeader.h diffs empty against the previous release) |

### 12.4 Fingerprint

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `481EE8B899D4D68BEBFAE58E16731A2FEF1B579DD380CEEBF17639AE5A8FBFC5`
- backimg图形板_v1.3.24.exe: 1,251,835 bytes, SHA256 `8112533E6AB2039B054033C630596B15D3694020C5A00104D3696103C5611DB3`

---

## Chapter 13: v1.3.25 Second-Granularity Rotation + Directory Self-Healing Release (2026-10-29)

**Driven by two user requirements; details are governed by `v1.3.25.md`.**

### D1 Rotation Interval Down to Seconds (companion UI changes, see the shell master report's v1.3.25 section)

- The user found in practice that "even 1 minute feels too slow" → the authoritative unit changed from whole minutes to **seconds**
- New config keys `randomVal` (seconds) + `randomUnit` (the panel's display unit; the DLL does not convert it);
  the old key `randomMin` (minutes) is still read for compatibility ×60, and the GUI still writes randomMin on save to backstop old DLLs
- Upper limit 86400 seconds (one day); RotateTimerProc / ApplyRotateTimer period = seconds × 1000ms
- The rotation signature (ApplyRotateTimerAll's change detection) switched to seconds in sync; the load log displays it for readability

### D2 Fix for the Custom Image Directory "Intermittently Not Showing" (bug on the user's real machine)

- **Symptom**: with custom=true and folder pointing outside the program directory (G://MyPics), occasionally the whole window lost its background;
  switching back to the plugin-directory Image and then back again restored it — intermittent and transient
- **Root cause**: LoadSettings made a one-shot verdict — IsDirExisting only proves the metadata is reachable;
  when the disk isn't ready / the volume mounts late / an antivirus holds a transient lock, FindFirstFile cannot get handles at all,
  so it was judged "directory exists but has no images", the list cleared, and return — with no recovery path thereafter
- **N1 Path normalization**: folder trimmed of leading/trailing whitespace/quotes/trailing backslash (drive roots preserved)
- **N2 Transient retry**: if enumeration is empty and the `*` probe also fails → retry in place 3×200ms, then enumerate again;
  a truly empty directory passes the probe, never reaches the retry, and doesn't slow window opening
- **N3 Self-healing reload**: if still not recovered → all registered windows get a 3-second one-shot RETRY_TIMER_ID
  timer that re-runs LoadSettings; after recovery, clear the timer + InvalidateRect for a full repaint;
  on continued failure retry every 3 seconds until the directory recovers; newly opened windows likewise set the timer while the list is empty and recovery is pending
- Exception boundaries follow the same discipline as RotateTimerProc: the callback is wrapped in try/catch throughout, never escaping back into explorer

## Chapter 14: v1.3.26 Version-Linked Release (2026-10-29)

**Zero DLL functionality changes; only the internal version stamp 1.3.25→1.3.26, bumped in lockstep with the shell.**

- All changes this round are GUI-side (the user's red box): the custom-interval input box + the "minutes/seconds" unit box
  moved from the random toggle row to a dedicated sub-row beneath it, with widths freed up (input box S(80), unit box S(88));
  the number and the word "seconds" are no longer crammed into the toggle row — details in the shell master report's v1.3.26 section
- Config keys (randomVal/randomUnit/randomMin compatibility), protocol, and timer behavior are identical to v1.3.25;
  the two DLL versions are directly interchangeable
- Incidental cleanup: the self-healing log line prefix `v1.3.25 :` changed to the neutral `提示 :` (avoiding pinning the version into the log text)
- Artifact: 942,255 bytes, first 32 hex of SHA256 `1D4D7E882E350FD8775739FDA82658A2`

## Chapter 15: v1.3.27 Version-Linked Release (2026-10-29)

**Zero DLL functionality changes; only the internal version stamp 1.3.26→1.3.27, bumped in lockstep with the shell.**

- This round the user sent **v1.3.26 out to Deepseek V4 Pro for review**; the report's three issues were verified one by one:
  Issue 1 valid (the sub-row title didn't show/hide with the random toggle), Issue 2 partially valid (two of the five globals
  were truly dead variables; the claim "would definitely trigger -Wall warnings" is inaccurate — a file-level static assignment already counts as a reference),
  Issue 3 valid (when the custom value exactly equals a preset notch, relying purely on reverse-inferring the number matched the wrong notch; present since v1.3.25)
- All three issues were fixed GUI-side (F1/F2/F3, details in the shell master report's v1.3.27 section and
  `v1.3.27.md`); the DLL's config keys/protocol/timer behavior are identical to
  v1.3.26; the two DLL versions are directly interchangeable
- Artifact: 942,255 bytes, first 32 hex of SHA256 `85E19DF9FD4F36FB9E28EAB7A83B4415`

## Chapter 16: v1.3.28 Version-Linked Release (2026-10-29 · Current)

**Zero DLL functionality changes; only the internal version stamp 1.3.27→1.3.28, bumped in lockstep with the shell.**

- This round the user sent **v1.3.27 out to Deepseek for re-audit** (verification of the previous round's three fixes + new findings):
  the F1/F2 fixes verified correct; the F3 fix was in the right direction but **missed load_config's state-flag reset**
  (Issue 1, a real functional bug: after "Restore config" restored a preset from a .bak file, the UI still stayed on
  the "Custom…" notch); Issue 2 (the hRotLabel dead variable) and Issue 3 (re-selecting Custom reset the already-entered value)
  were non-fatal tidiness/UX issues
- All three issues were fixed GUI-side (P1/P2/P3, details in the shell master report's v1.3.28 section and
  `v1.3.28.md`); the DLL's config keys/protocol/timer behavior are identical to
  v1.3.27; the two DLL versions are directly interchangeable
- Artifact: 942,255 bytes, first 32 hex of SHA256 `7846DEBD35F5F259A75BC45A047B13DB8`

## Chapter 17: Known Limitations (v1.3.28 Current-State Summary)

- Relative to the author's v1.3.14, **lost**: per-directory image switching (OnDocComplete), per-HWND multi-window (real-machine testing proved the single record sufficient),
  the whole folderExt chain (setting it in config has no effect) — differences inherited from the lineage baseline, not introduced by the fixes
- Replacing the content of a same-named file takes effect only after restarting explorer (no rebuild when the file-name set is unchanged, matching upstream); adding/removing image files takes effect immediately
- After a differential rebuild, an old window's ImgIndex may "switch to a different image" (out-of-bounds clamping backstops it; it will not crash)
- No version resource (the Properties page shows no version number); the version is identified by the first line of `plugin\ExplorerBgTool_init.log`
- Modes 0~4 never enlarge the original (small images stay sharp); to enlarge and follow the window use 5 (Scale) or 6 (Scale and Fill)
- Multi-window background ownership is now tracked per window handle (v1.3.20 B); multiple tabs / multiple windows on one thread no longer overwrite each other
- Timed rotation's minimum granularity has been 1 second since v1.3.25 (randomVal; the old key randomMin remains compatible); the on-schedule re-draw has out-of-bounds clamping as a backstop
- Scaling cache lifecycle (v1.3.21 D1 + v1.3.22 R1; the v1.3.23 independent re-audit found nothing new): bookkeeping by registered owner thread;
  a thread's cache is released only when its last registered window is destroyed; cross-thread destruction leaves caches untouched;
  unexpected thread disappearance is backstopped by SweepDeadThreadCaches / ReleaseAllCaches
- The per-thread source-image cache has an upper bound (64MB / 16 entries), and the scaled-result cache is a per-thread 32MB LRU — memory stays bounded even in extreme many-image scenarios
- The DLL stays pinned in the process (DllCanUnloadNow always S_FALSE, since v1.3.18) — standard practice for shell extensions;
  unloading happens only at process exit; the hooks no longer have the "jump into nothing after unload" exposure surface
- In src/, ShellLoader.cpp/.h are GBK-encoded and the rest UTF-8 — confirm the encoding before editing ShellLoader

## Chapter 18: Build (Reproducible)

- DLL: `DLL源码\build.cmd [MinGW bin directory]` (internally the g++ command from §8 of `v1.3.15.md`,
  plus `--image-base,0x10000000` for a fixed image base; a rebuild differs from the deliverable by only 6 bytes of PE timestamp, re-verified by the third-party verification round)
- Shell: `外壳源码\build.cmd`
- Self-check: `自检程序\运行自检.cmd [image directory]` (requires MinGW-w64 g++ on PATH or `BACKIMG_GPP` pointing to g++.exe)

---

*This volume was merged from the split-volume reports (2026-10-28); the technical details of each version's revisions are governed by the `CHANGELOG_*.md` files in the same directory (from v1.3.25 on, after the audit loop concluded, the project returned to the "user-requirement-driven" cadence);
the shell-side iteration history is in `../../shell/iteration-report-master.md`.*
