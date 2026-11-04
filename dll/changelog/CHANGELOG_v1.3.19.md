# backimg DLL Changelog v1.3.18 → v1.3.19

- Audit source: Kimi K3.1 Pro read-only re-review report (`Desktop\backimg_GUI_v1.3.18_DLL_Audit_Report.txt`)
- Handling principle: verify each item against the v1.3.18 source code one by one; only fix what is confirmed true; fixes must not introduce new problems
- Conclusion: **all 14 items confirmed true** (of which P2-4 is "partially true", handled with a compromise approach), 13 code fixes + 1 handled via documentation

## I. Verification Conclusion Summary Table

| No. | Kimi Report Content | Verification Conclusion | Handling |
|---|---|---|---|
| P1-1 | GetSourceLocked releases IStream prematurely | **True** (WinAPI.cpp Releases immediately after FromStream, while DrawImage/delete decoded still use the stream afterward; GDI+ allows deferred decoding, the stream must stay alive until the bitmap is destroyed) | ✅ Fixed |
| P1-2 | Initialization uses plain bools with race conditions + GdiplusStartup return value ignored | **True** (when multiple STA threads enter OnWindowLoad simultaneously, B may decode directly right after A sets the flag but before GDI+ has started) | ✅ Fixed |
| P1-3 | "Scale and fill" target bitmap has no upper limit, ScaleRound may overflow int (UB) | **True** (100000x1 image + 1920x1080 window → an allocation request of about 434 GiB) | ✅ Fixed |
| P1-4 | Raw bytes have no budget, bad_alloc can cross COM/Hook boundaries | **True** (BitmapGDI construction resizes directly according to ftell, a single file can reach 2GB) | ✅ Fixed |
| P2-1 | MH_EnableHook failure leaves a "half-enabled set of hooks" | **True** (MinHook batch enable executes item by item and does not roll back automatically) | ✅ Fixed |
| P2-2 | Data race between config scalars and window-creation/painting threads | **True** (isRandom/imgPosMode/imgAlpha are read without locks) | ✅ Fixed |
| P2-3 | Real decoding does not check GetLastStatus/DrawImage status | **True** (contradicts what the project itself recorded in dllmain.cpp: "MinGW decoding failure still returns a non-null pointer") | ✅ Fixed |
| P2-4 | 32MB/64MB budget does not hold for a single over-budget entry | **Partially true**: mechanically a single entry can indeed exceed the budget (the budget is an upper bound of "budget + 1 entry") | ⚠️ Compromise (see Section IV) |
| P2-5 | Old list not cleared after the image directory becomes invalid, old background image remains | **True** (inconsistent with the "directory is empty" branch behavior) | ✅ Fixed |
| P2-6 | Non-random mode stops at the first bad image; switching to random does not rebuild the list | **True** (the first loop pass breaks unconditionally; random switching is blocked by the file-set early-exit condition) | ✅ Fixed |
| P2-7 | Registration path buffer uninitialized/return value unchecked; unregistration has an extra backslash | **True** (on truncation wcslen reads out of bounds; although `\\{CLSID}` is usually normalized by the registry API, both sides must be written consistently) | ✅ Fixed |
| P3-1 | JPEG detector returns IMG_OK even without SOF; missing EOI judged dead outright | **True** (a legitimate JPEG exceeding 256 marker scans gets wrongly rejected, without even a GDI+ fallback) | ✅ Fixed |
| P3-2 | std::wstring operations inside DllMain may throw exceptions within the loader lock | **True** (bad_alloc under low memory = taking down the host process) | ✅ Fixed |
| P3-3 | QueryInterface failure does not clear the output parameter | **True** (required by the COM contract) | ✅ Fixed |

## II. Fix Details

### P1-1 IStream lifetime (WinAPI.cpp GetSourceLocked)
- `stream->Release()` moved from after `FromStream` to after `delete decoded`
- All early-return branches uniformly finish with "delete the bitmap first, then release the stream" (aligned with the correct order of the construction probing path)
- Also fixed P2-3 along the way: `decoded->GetLastStatus()` and `GetLastStatus()` after `Graphics::DrawImage` must both be checked; on any failure, do not enter the cache

### P1-2 Initialization synchronization (dllmain.cpp)
- Deleted the two plain bools `m_isInitHook`/`s_gdiStarted`, replaced with `std::once_flag` + `std::atomic<bool> s_initDone`
- The initialization body extracted into `InitBackground()`, executed by `std::call_once`: other threads block until it completes, no longer leaving a window of "seeing the flag set but GDI+ not started"
- `GdiplusStartup` return value checked: on failure → call `BitmapGDI::NotifyGdiplusStartupFailed()`; from then on construction probing and GetScaled all short-circuit, the background image feature is disabled entirely, File Explorer is unaffected

### P1-3 Hard upper limit on target size (DrawScale.h + WinAPI.cpp)
- `ScaleRound`: clamps to within INT_MAX before converting to int, eliminating the double→int overflow UB
- `ComputeDrawRect`: if the total target pixel count (w×h) exceeds **64 million pixels** (256MB at 32bpp), return an empty rectangle and give up drawing
- `GetScaled` entry: check dstW×dstH×4 with 64-bit arithmetic; if it exceeds 256MB, return empty directly
- 64 million pixels covers 8K fullscreen (about 33 million pixels) with headroom; zero impact on normal wallpapers (verified by test_extreme)

### P1-4 Memory budget + exception boundaries
- BitmapGDI construction: single-file raw byte limit of **256MB** (far exceeding any reasonable wallpaper); over-limit files are treated as invalid
- Exception boundaries (`try/catch(...)`): `OnWindowLoad`, `MyCreateWindowExW`, `MyFillRect`, `DllGetClassObject`, `ClassFactory::CreateInstance` — any bad_alloc no longer crosses COM/Hook boundaries, swallowed in place (no background image ≠ explorer crash)

### P2-1 Rollback on enable failure (dllmain.cpp InitBackground)
- When `MH_EnableHook(MH_ALL_HOOKS)` fails: `MH_DisableHook(MH_ALL_HOOKS)` + item-by-item `MH_RemoveHook`, logging records the Disabled status — no longer leaving the half-enabled state of "BeginPaint hooked, EndPaint not"

### P2-2 Atomicizing config scalars (dllmain.cpp)
- `Config::imgPosMode/isRandom/imgAlpha` changed to `std::atomic`, read side explicitly calls `.load()`, eliminating the data race with window-creation/painting threads (UB)

### P2-5 Clear list when directory invalid (dllmain.cpp LoadSettings)
- The `imgPath.empty()` branch aligned with the "directory is empty" branch: clear `m_config.imageList` under `g_listMx` — after the directory is deleted/disconnected, the old background image no longer remains; when the directory recovers, it can be reloaded automatically (imageList is already empty, so the rebuild condition still holds)

### P2-6 Non-random bad image + random switching (dllmain.cpp LoadSettings)
- Loop changed to break only "after successfully collecting the first valid image" — when the first file after sorting is bad, keep looking for good ones behind it
- Added `s_listIsFullSet` state: non-random = single image (not the full set), random = full set; when the user switches from non-random to random, the full list is force-rebuilt even if the file set has not changed

### P2-7 Registry (ShellLoader.cpp)
- `dllpath[MAX_PATH] = {0}` + check the `GetModuleFileNameW` return value (0 or truncation means failure)
- The two critical writes — CLSID default value and InProcServer32 DLL path — check return values, returning `SELFREG_E_CLASS` on failure
- The backslash of the BHO path in unregistration unified with the registration side (old style `Objects\\{CLSID}`)

### P3-1 JPEG probing contract (ImageHeader.h)
- When no valid SOF size is obtained, return `IMG_UNKNOWN` (let GDI+ arbitrate), no longer returning `IMG_OK` empty-handed, which caused legitimate images to be wrongly rejected
- Missing EOI (truncated/with trailing data) relaxed from `IMG_SUSPECT` to `IMG_UNKNOWN` — upholding the principle "the parser never vetoes an image on its own"

### P3-2 Pure Win32 inside DllMain (dllmain.cpp)
- Process name check changed to `wcsrchr` + `_wcsicmp`, removing std::wstring construction/erase/transform — zero dynamic allocation and zero possibility of exceptions inside the loader lock

### P3-3 COM contract (ShellLoader.cpp)
- Both `QueryInterface` implementations: null check at entry (E_POINTER), `*ppv = NULL` before failure
- `CreateInstance`/`DllGetClassObject`: output parameter null checks + exception boundary (E_OUTOFMEMORY)

## III. Self-Test Program Synchronized Fixes (Section 6 of the Kimi report, not counted as DLL bugs)
1. `test_render.cpp`: removed the `_exit(0)` that made result statistics unreachable; `g_fail` now honestly determines the exit code
2. `test_follow.cpp`: `return bad == 0 ? 0 : 1` (previously fixed at 0)
3. `test_race_loop.cpp`: accumulates err and returns an exit code based on results (previously fixed at 0)
4. `test_follow.cpp`/`test_race_loop.cpp`: fixed the code-page mismatch of "computing length with CP_ACP but converting with CP_UTF8" (distortion of Chinese paths)
5. Added `test_extreme.cpp`: P1-3 regression — extreme aspect ratios (100000x1, 1x100000, 999999x999999) × 7 modes must all stay under the 64-million-pixel limit, and 8K normal filling must not be wrongly rejected

## IV. Handling Decision for P2-4 (Honest Statement)

Kimi pointed out that "a single over-budget entry can reside in the cache long-term, so the hard upper bound does not hold". Mechanically true, but **it was not fixed the way Kimi suggested ("do not put it into the cache")**, for these reasons:

- If scaling results/source bitmaps do not enter the cache → re-decode + re-render every frame (GetScaled is called on every FillRect);
  in large-image/large-window scenarios the CPU maxes out immediately — trading a CPU disaster for a memory bound, the loss outweighs the gain
- Actually adopted: **per-item size cap** (the source bitmap's 512MB dimension cap unchanged, a new 256MB area cap added for scaling results);
  cache upper bound = budget + single-item maximum, bounded and known in the worst case (single thread ≤ 512+256MB; scaled by thread count, still bounded likewise)
- Documentation (delivery notes "known behavior") honestly states this semantics, without falsely claiming a "hard upper bound"

## V. Verification Record (All Actually Executed)

1. **Compilation**: MinGW g++ -Wall zero warnings (only the pre-existing MinGW `#pragma comment` notice); SHA256 consistent across the three DLL copies
2. **test_render**: 115 passed / 1 failed — the failure is the alpha distribution assertion caused by the sample image itself being an opaque screenshot (so since v1.3.18, a property of the test image, not a code defect); old-vs-new rendering error 1.33 / alpha 0.00
3. **test_leak**: three rounds of list rebuilds, rounds 2/3 +0MB — hash-key cache hits work normally, memory is bounded
4. **test_race_loop**: 8 threads × 300 rounds × 3 runs = 7200 scaling operations with zero errors, GDI handle sawtooth fluctuation with no leaks; exit code 0 (from this release onward the exit code is real)
5. **test_follow**: geometry checks for 7 modes × multiple window sizes all passed (no distortion, no exceeding the window), exit code 0
6. **test_extreme** (new): extreme aspect ratio protection fully passed, 8K normal filling not wrongly rejected
7. **EXE byte signature**: contains v1.3.19, zero v1.3.18 remnants; version resource FileVersion 1.3.19.0

## VI. Fingerprints

| File | Size | SHA256 (first 32) |
|---|---|---|
| ExplorerBgTool.dll | 921,713 B | `8c3ef57614a11ba3767905ece70d8158` |
| backimg图形板_v1.3.19.exe | 1,233,842 B | `f38f03b4191fa5e17bf0a0a4031b2533` |
