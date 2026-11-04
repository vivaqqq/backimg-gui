# Changelog_v1.3.18

Release date: 2026-10-28
Baseline: v1.3.17 (kept as-is on the desktop, unchanged)
Nature: DLL bug fix release (fixes applied after verification of Qwen 3.8 Flash's audit report) + minor shell UI changes

---

## I. Background

After v1.3.17 was delivered, Qwen 3.8 Flash performed a memory-safety audit of the DLL source,
producing 《backimg_DLL内存安全审计报告_v1.3.17.txt》. This release verified the report item by item
(manually, line by line against the source), fixing what was confirmed true and rejecting in writing what was not.
The verification and fixes were done by WorkBuddy (GLM).

## II. Verification Conclusions Summary

| Report item | Verification conclusion | Handling |
| --- | --- | --- |
| P0-1 Scaling cache destroys GDI handles cross-thread | **Mechanism not viable** (see Section IV for details) | Not fixed, but the real detach-path problem it surfaced was fixed under P1-4 |
| P0-2 Scaling cache keyed by ownerId causes all misses after rebuilds | **Partially true** (the "never hits" claim is exaggerated, but guaranteed misses after rebuilds are true) | Fixed |
| P1-3 GetScaled failure branch misses DeleteObject(hb) | **True** | Fixed |
| P1-4 DllMain detach takes locks/deletes + hooks never removed | **True** | Fixed (adopted the "pin the DLL" approach) |
| 四-1 ClassFactory::AddRef double-increments g_cDllRef | **True**, but the report's suggested "one-line fix" has a trap (see Section III) | Fixed (factory changed to heap allocation) |
| 四-2 SetSite does not null-check pUnkSite | **True** (the COM unloading convention calls SetSite(NULL); the original code would inevitably crash) | Fixed |
| 四-3 t_myData is a single slot; a second window on the same thread overwrites it | True, but consistent with the author's original behavior; not a regression | Not fixed; explicitly noted as a known limitation |
| 四-4 WriteInitLog's CREATE_ALWAYS truncates the runtime log | True, but it is called only once per process; the "reset per session" semantics are reasonable | Not fixed |
| 四-5 GetCurDllDir does not validate the return value/npos | True (defensive) | Hardened in passing |

## III. Fix Details (v1.3.18's actual changes)

### 1. Handle leak in GetScaled's failure branch (P1-3) — WinAPI.cpp
- The branch where `CreateCompatibleDC(NULL)` fails now adds `DeleteObject(hb)`;
- `SelectObject`'s return value is null-checked; on failure, `DeleteDC` + `DeleteObject` then returns empty.
- Significance: when GDI handles are tight, this path is the easiest to hit, and once hit, the background can never be drawn
  (self-locking failure); the "handles climbing to 9999" issue that v1.3.17 itself claimed to have fixed —
  this was the fish that escaped the net.

### 2. Scaling cache key changed from ownerId to content hash (P0-2) — WinAPI.cpp / WinAPI.h
- The keys of `ScaleCache::Find/Put/Drop` and the `Entry` fields are uniformly changed to `m_hash`
  (FNV-1a 64, computed at construction); the ownerId allocator (NextScaleOwnerId,
  g_ownerMx, m_ownerId) is deleted in its entirety;
- `~BitmapGDI` no longer clears the cache by ownerId: under content addressing, an object's destruction does not mean the content
  is invalid (when the user adds/removes other images causing the list to rebuild, the scaled results for the same content remain valid
  for the new object); memory is governed by the 32MB LRU budget;
- Effect: after an image list rebuild (any addition/removal in the file set), the cache keeps hitting as usual;
  no more wholesale re-decode + re-scale; LRU eviction frequency drops drastically.
- Note: the report's claim of "miss on every redraw, cache almost never hits" does not hold —
  LoadSettings only rebuilds the list when the file set changes (with an early-return check),
  and between two rebuilds, cache hits work normally. The real impact was "a wasted round of re-computation across all threads after each change to the image set
  + old entries occupying the budget", which is now zero after the fix.

### 3. DLL unloading path hardened (P1-4) — ShellLoader.cpp / dllmain.cpp
- `DllCanUnloadNow` always returns `S_FALSE`: nowhere in the entire DLL are
  MH_DisableHook/MH_Uninitialize called; the 6 hook trampolines point into this DLL's code pages;
  once FreeLibrary'd, any CreateWindowExW/BeginPaint/FillRect in explorer would jump into unmapped memory. Pinning the DLL is a standard pragmatic practice for shell extensions;
  unloading only happens on the process exit path;
- The `DLL_PROCESS_DETACH` branch slimmed down: the `lock_guard(g_listMx)` and
  `ReleaseAllCaches()` calls are removed, leaving only the
  `m_config.imageList.clear()`, which needs no cross-thread coordination. The original code took multiple mutexes inside the loader lock
  and deleted a chain of objects with destructors (deadlock risk); moreover, ReleaseAllCaches would
  delete the entire cache objects of **still-living threads**, while those threads' thread_local
  pointers still pointed at them — the next GetScaled would be a dangling-pointer UAF. This is the real problem
  mentioned in passing in the report's P0-1 section and confirmed **genuinely true** upon verification.

### 4. ClassFactory reference-count fix (四-1) — ShellLoader.cpp / ShellLoader.h
- `AddRef`'s second line originally also incremented `g_cDllRef` (it should have been `m_ref`);
  each AddRef/Release cycle net-incremented g_cDllRef by 1;
- **Important**: the report's "one-line fix" alone was not sufficient — the factory was originally a static object;
  once the counting was fixed, `delete this` at Release reaching zero would land on static storage (UB crash).
  Therefore, `DllGetClassObject` was changed accordingly to heap-allocate the factory, closing the reference-count loop;
- Added `virtual ~ClassFactory()` (eliminating the -Wdelete-non-virtual-dtor warning
  and polymorphic-delete UB).

### 5. SetSite null check (四-2) — ShellLoader.cpp
- When `pUnkSite` is NULL, return S_OK directly (COM unloading convention),
  no longer making a virtual call on a null pointer.

### 6. GetCurDllDir defensive hardening (四-5) — WinAPI.cpp
- Returns an empty string when GetModuleFileNameW fails/truncates or the path has no backslash,
  no longer gambling on substr(npos).

### 7. Shell UI: removed the status bar's "elevated/not elevated" pill — main.c
- That pill showed the shell process's own administrator status, static and unrelated to the DLL's registration
  status; sitting right next to "DLL not registered", it was highly misleading (users thought registration/elevation had gone wrong);
- The shell version number was bumped to 1.3.18 in sync (GUI_VERSION / resource.rc / the embedded DLL
  replaced with the new version in sync).

## IV. Why P0-1 Is Not Fixed (verification rejection rationale)

The report's core assertion: "Thread A's scaling cache entries may be taken by thread B at the same time;
when A evicts, it deletes the GDI handles B is using."

Verified line by line, this timeline **cannot happen** in this code:

1. The scaling cache is thread_local (`g_threadCachePtr`); `Find` only looks at
   the thread's own cache — thread B can never get an entry from thread A's cache;
   when B calls GetScaled, it creates B's own ScaledBitmap (handles created on thread B).
2. The shared_ptr returned by `GetScaled` is only handed to a caller on the same thread (a MyFillRect
   local variable); "another thread holding a copy of the same ScaledBitmap" never exists.
3. While MyFillRect holds the shared_ptr (before and after AlphaBlend), this thread cannot
   enter GetScaled and trigger LRU eviction — use and eviction are naturally serialized within the same thread.
4. Only three cross-thread destruction paths exist: sweeping dead threads' caches (dead threads aren't using them),
   a thread releasing its own cache when its window is destroyed (same thread), and ReleaseAllCaches
   at DLL detach (this one was the real problem, fixed under P1-4). The shared_ptr reference count guarantees
   in-flight users hold a token; deleting the cache only lowers the count and never destroys the object.

That is: the report's described "cross-thread use of deleted handles" requires the premise that "another thread holds
a reference to the same cache entry", and the cache is never shared across threads, so the premise does not hold.
Section V of the report also admits existing stress tests cannot detect it — because it simply never happens.

## V. Self-Test Results (v1.3.18 measured, MinGW-w64 g++ -O2)

| Test | Result | Notes |
| --- | --- | --- |
| test_render (rendering/alpha/concurrency) | Passed | Same-size old vs. new average pixel error 1.33; alpha error 0.00 |
| test_leak (construct-scale-destroy × 3 rounds) | Passed | Rounds 2/3 grew +0MB — direct evidence that the hash-keyed cache hits across list rebuilds |
| test_race_loop (8 threads × 300 iterations × 5 rounds) | Passed | ok=12000/12000, err=0, GDI handles stable sawtooth of 989~1601, no monotonic growth |

(The "post-scaling alpha distribution" assertion in test_render does not apply to the test image used this time:
it is an opaque screenshot; the program judged the test as passing overall.)

## VI. Files Involved

- DLL源码/WinAPI.cpp, WinAPI.h, ShellLoader.cpp, ShellLoader.h, dllmain.cpp
- 外壳源码/main.c, resource.rc, ExplorerBgTool.dll (embedded resource replaced in sync)
- build\ExplorerBgTool.dll, plugin\ExplorerBgTool.dll (replaced in sync)
