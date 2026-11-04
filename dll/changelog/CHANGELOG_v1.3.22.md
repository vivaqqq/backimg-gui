# backimg v1.3.22 Changelog — Deepseek Re-Review Hardening (R1 Cross-Thread Destruction / R2~R4 Low-Risk Hardening)

- **Release date**: 2026-10-29
- **Baseline**: v1.3.21 (desktop copy preserved as-is, untouched)
- **Trigger**: the user had Deepseek V4 Pro perform a read-only re-review of v1.3.21. Re-review conclusion: all 8 of v1.3.21's
  fixes were confirmed true and properly implemented, with no regressions; 4 new "dormant" latent hazards were found
  that do not trigger under normal circumstances. Each was verified, and **all 4 confirmed true**, all handled per the "fix if fixable" principle
  (1 code hardening + 1 magic number replacement + 2 comment solidifications), with zero behavior change on normal paths.

---

## I. Verification Correction (one citation error in the re-review report, kept on record)

Re-review item 1 claimed "Windows itself allows cross-thread DestroyWindow (MSDN explicitly says so)" —
**the citation is wrong**: MSDN actually explicitly states that DestroyWindow cannot destroy a window created by another thread
(a cross-thread call fails and returns an error). But the hazard mechanism **still holds**: this plugin's hooks execute
their bookkeeping logic **before** the real DestroyWindow, so even if a cross-thread call fails to actually destroy,
the unregistration/cache clearing/session clearing have already happened — the pollution is done regardless. The fix proceeded as planned.

## II. Fix Details (4 DLL items, zero shell changes)

### R1 [Medium priority · the only substantive hardening] MyDestroyWindow thread-ownership hardening
- **Hazard**: v1.3.21's D1 used `GetCurrentThreadId()` **at destruction time** to match the tid recorded **at creation time**
  in the registry. On cross-thread destruction (the hook has already been entered, the real destruction has not happened):
  no tid matches at all → misjudging "this thread has no more windows"; `ReleaseThreadCache()` releases the
  **destroying thread's** cache (wrong target); `t_sessionHwnd/t_duiPaintDepth` clear the
  **destroying thread's** thread_local (wrong target).
- **Fix**: governed by the owner tid recorded at registration —
  - Owner == current thread (normal path; explorer's regular flow always takes this path): behavior **byte-identical** to v1.3.21
    (release the cache and clear this thread's session only if this thread has no other registered windows);
  - Owner != current thread (exception path): only unregister the registration + KillTimer; cache and session
    state are untouched entirely. Orphan caches are reclaimed by the SweepDeadThreadCaches / ReleaseAllCaches
    fallback (mechanism already in place, verified effective in Section 4 of the re-review).

### R2 [Low priority] Solidifying the TOCTOU pattern of the threadEmpty snapshot
- **Hazard**: the snapshot is computed inside the `g_wndDataMx` lock but used outside it ("check-use" separation).
  Current safety relies on "same-thread Create/Destroy go through the same message queue and are naturally serialized".
- **Handling**: not changed to nested locking (it would introduce a g_wndDataMx → g_registryMx nesting, violating
  lock discipline); instead, the invariant is **solidified in a comment** at the head of MyDestroyWindow as a warning to future maintainers.

### R3 [Low priority] RandomIndex(size_t) clamp magic number
- 2147483647u → `(size_t)INT_MAX` (WinAPI.h gains `#include <climits>`),
  explicitly expressing "clamp to the int upper bound", no longer relying on a 32-bit unsigned literal.

### R4 [Low priority] Semantics note for the globally shared s_rotTimerCur (not a bug, no behavior change)
- The re-review explicitly stated "no change needed"; per the suggestion, a comment in RotateTimerProc solidifies: the global sharing
  cannot currently go wrong (rotateMin is a global config + config changes trigger full reinstallation); if the interval is ever changed to
  per-window independent configuration, this must be changed accordingly to per-window records.

## III. Verification Record (All Actually Executed)

| Item | Result |
|---|---|
| DLL compilation | MinGW g++ -std=c++17 -O2 -Wall zero errors, only pre-existing benign warnings |
| EXE compilation | MinGW gcc -std=c11 -municode -Wall zero errors |
| Byte signature | DLL contains the v1.3.22 log header (UTF-16 ×1), zero v1.3.21 remnants; EXE contains v1.3.22, zero v1.3.21 remnants; byte comparison confirmed the EXE-embedded DLL is the new v1.3.22 |
| test_geometry | 227/227 all passed |
| test_bugs / test_filter / test_follow / test_leak | All passed (leak three rounds +0MB) |
| test_render | 116/116 all passed (RGBA standard sample) |
| test_race_loop | 8 threads × 300 rounds × 5 runs = 12000 operations, zero errors |
| test_random | Chi-square uniformity passed; boundary n≤1 passed; 8 threads × 20000 draws, zero out-of-bounds |
| test_imageheader | 13/16 (3 failures verbatim identical to all previous versions v1.3.20/21; ImageHeader.h diffs empty against the previous version, not introduced by this version) |
| Sample image environment | The desktop 111222333 sample image directory was cleaned out again (unrelated to the code); an RGBA standard sample with alpha gradients was rebuilt (307 bytes) and actually tested |

## IV. Fingerprints

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `aff84abc49dea9214ff07f3bc13004fdb66176b4e127d5a867a5c9a35726516a`
- backimg图形板_v1.3.22.exe: **1,251,835 bytes**, SHA256 `53e3982362c1573b1aa2add9913ec1ac07e070a5c9bf873131f0a88ceeb2c069`

## V. Notes for Users

- This version has **zero UI changes and zero config changes**; normal usage is exactly the same as v1.3.21; the changes are only
  in the DLL's exception-path hardening and comments.
- Upgrade installation: `backimg图形板_v1.3.22.exe` and `plugin\ExplorerBgTool.dll` must be
  **overwritten as a pair**, then restart File Explorer.
- Real-world explorer behavior needs to be confirmed by the user's own testing after installation.
