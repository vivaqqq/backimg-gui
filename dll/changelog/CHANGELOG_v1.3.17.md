# backimg v1.3.17 Changelog (CHANGELOG)

- **Baseline**: `backimg_GUI_v1.3.16` (unchanged, kept on the desktop)
- **Nature**: bug fix release. Zero interface-logic changes; only the version number and behavior description text are updated.
- **Verification**: see the "Verification Record" at the end; everything is reproducible via the delivery package's `自检程序\` directory.

---

## I. Disposition of the Audit Report's 9 Findings

| # | Severity | Issue | Disposition |
|---|---|---|---|
| 1 | 🔴 Critical | Per-thread source image cache (`m_sources`) leaks without limit; measured 177MB of permanent extra residency per image switch | ✅ Fixed (see 1.1) |
| 2 | 🔴 Critical | `BitmapGDI::Size` uninitialized; garbage heap values read 200/200 times | ✅ Fixed (see 1.2) |
| 3 | 🟠 Medium | Early return means `t_myData.size` never updates → self-triggered redraw every frame | ✅ Fixed (see 1.3) |
| 4 | 🟠 Medium | `g_gdiplusMx` defined but zero references; the serialization promised in comments never implemented | ✅ Fixed (see 1.4) |
| 5 | 🟠 Medium | mode 5 "Scale" semantics changed relative to v1.3.15 without explanation | ✅ GUI behavior description supplemented (see 1.5) |
| 6 | 🟡 Minor | DLL version number shown in the GUI was still 1.3.15 | ✅ `DLL_VERSION` changed to 1.3.17 |
| 7 | 🟡 Minor | DLL initialization log header said v1.3.15 | ✅ Changed to v1.3.17 |
| 8 | 🟡 Minor | Delivery package lacked the DLL build script | ✅ Added `DLL源码\build.cmd` |
| 9 | 🟡 Minor | Self-test programs depended on the missing `probe/veh.h` and had development-machine paths hardcoded | ✅ Fixed (see 1.6) |

### 1.1 Source image cache leak (audit #1)

**Root cause**: `ScaleCache` has two containers — `m_items` (scaling results) is bounded by the 32MB LRU budget,
while `m_sources` (per-thread decoded source bitmaps) is stored by ownerId with **no budget at all**;
`Drop()` only clears `m_items`. So every image switch/directory switch, the old ownerId's source bitmaps stayed resident forever.

**Fix** (two layers):
1. `m_sources` re-keyed by **file content hash**: rebuilding the list with the same content hits the cache directly, zero growth;
   when content changes, the LRU budget evicts.
2. Hard upper bound added: per-thread source image cache ≤ 64MB and ≤ 16 entries; beyond that, LRU `delete`.

**Measured** (user's 39 PNGs at 1024×1536):

| Metric | v1.3.16 | v1.3.17 |
|---|---|---|
| One scaling pass | +280 MB | +166 MB |
| Residue after list destruction | **+177 MB** | **+66 MB (bounded)** |
| 5 rebuilds | Linear growth to +885 MB | **+0 MB per round** |

### 1.2 Size uninitialized (audit #2)

- `SIZE Size = { 0, 0 };` given an initial value;
- New `IsValid()`: true only when "the image header parsing confirms the structure is complete" or "the GDI+ probe status is Ok";
- The filter in `dllmain.cpp` restored to `bmp->IsValid()` (v1.3.16 had degraded to the bare comparison `Size.cx > 0`).

**Measured**: constructing non-existent files 200 times, garbage values 200/200 → **0/200**.

### 1.3 Infinite redraw (audit #3)

`t_myData.size = wndSize;` moved before all early returns (read the old value for comparison first, then write back immediately).
The normal path still converges to 1 redraw; with `imageList` empty / `GetScaled` failure, reduced from 1000/1000 redraws to **1**.

### 1.4 GDI+ serialization lock (audit #4)

`GdiplusMutex()` actually wired up, covering all GDI+ call sites: decoding (`FromStream`),
`DrawImage`, `GetHBITMAP`, and GDI+ object `delete`. The lock is an "intentionally leaked singleton",
avoiding clashes with thread_local destruction order. Cache lookups and GDI handle operations remain outside the lock.

### 1.5 mode 5 semantics explanation (audit #5)

mode 5's semantic change was made in v1.3.16 per the requirement (aspect-ratio-preserving, no distortion),
but the GUI did not make clear the point "relative to v1.3.15's stretch-to-fill". The behavior description now states:

> "Scale" = the entire image fully fits (may leave margins on all sides); "Scale and fill" = aspect-ratio-preserving fill with overflow cropped.

### 1.6 Self-test programs reproducible (audit #9)

- Removed the dependency on the missing `probe/veh.h` (replaced with a bundled `crashlog.h`);
- Output paths changed to be relative to the current directory;
- Image paths all go through command-line arguments (Windows argv is ANSI code page; converted to wide chars with `CP_ACP`);
- Added `运行自检.cmd` to run all self-tests with one click.

---

## II. Issues Newly Discovered and Fixed in v1.3.17 (beyond the audit report)

### 2.1 🔴 MinGW `thread_local` destruction crash mine (more severe than every audit finding)

**Symptom**: `static thread_local ScaleCache g_threadCache;` (containing a `std::vector`) at thread exit —
the TLS destruction callback runs on **memory that has already been freed and filled with 0xEE by ucrt** → double free → crash.

gdb-measured crash stack:
```
ntdll!RtlExitUserThread → tls_callback → run_dtor_list
  → ScaleCache::~ScaleCache → vector::~vector → _M_deallocate
The freed pointer = 0xfeeefeeefeeefeee
```

**Independent minimal reproduction** (unrelated to GDI+): a `std::vector` in `thread_local` always crashes;
an `int` alone passes 200 rounds. The destruction logs printed size/capacity values that were all garbage.

**Why v1.3.15/v1.3.16 never exposed it**: the other `thread_local MyData` in v1.3.16 was all POD
with no destructor; and the original claim of "40/40 zero crashes" was actually because
**the stress test had a VEH exception handler installed that swallowed the crashes** — an illusion.

**Fix**: `thread_local` now stores only **raw pointers** (trivial destruction, no TLS callback registered);
cache objects are heap-allocated and registered in a global table; on window destruction (`MyDestroyWindow`) the current thread's cache is released proactively,
and on DLL unload everything is released as a fallback.

### 2.2 🟠 GDI handle exhaustion (thread exit does not release caches)

Exposed after fixing 2.1: per-thread caches are no longer destructed at thread exit (and shouldn't rely on destruction anyway),
so in stress tests handles went 2583 → 5161 → 7739 → **9999 hitting the cap**, after which `CreateCompatibleDC` failed and the background couldn't be drawn.

**Fix**: added **dead-thread cache sweeping** — the registry records each cache's owner thread id,
periodically checks whether the owner thread has exited (`GetExitCodeThread` == STILL_ACTIVE as the liveness check),
and releases the caches of exited threads along with them. Measured handles changed from monotonic growth to **stable sawtooth of 655~1959**.

> Pitfall record: liveness was first checked with `WaitForSingleObject(h,0)==WAIT_TIMEOUT`,
> but **the current thread waiting on itself returns WAIT_OBJECT_0 immediately**, misjudging a living self as dead,
> and the sweep deleted the cache in active use → heap corruption → `abort()` (and abort goes through raise(),
> bypassing VEH, so the crash logger saw nothing). Changed to `GetExitCodeThread` + a direct pass-through for the current thread.

> Pitfall record 2: the sweep was initially placed inside `GetThreadCache()`, which is called in `GetScaled`
> **while holding the GDI+ lock** → the sweep deletes a cache → `~ScaleCache` takes the same lock again
> → `std::mutex` is non-reentrant → **self-deadlock** (manifesting as the stress test hanging rather than crashing).
> The sweep has been moved to before taking the lock.

### 2.3 🟡 Corrupt images sent directly to GDI+'s newly exposed attack surface

v1.3.15 happened to `delete[]` the buffer right before `SHCreateMemStream`, so GDI+ read a first byte polluted by
free-list metadata; with the PNG signature corrupted, GDI+ exited safely and early —
"accidentally lucky" protection. v1.3.16 keeps the full bytes, so a corrupt PNG would actually be parsed.

**Fix**: added `ImageHeader.h`, a lightweight image header parser (PNG chunk walk + IHDR CRC check,
JPEG SOI/EOI/SOF). **The core principle is "never veto alone"** — rejection only when "confirmed corrupt" can be proven
(chunk out of bounds = truncation, IHDR length/CRC mismatch, absurd dimensions);
anything unparseable is handed to GDI+ for the verdict, avoiding false kills.

> Pitfall record: the first version hardcoded "at most 64 PNG chunks", misjudging one of the user's legitimate PNGs with
> 212 IDAT chunks as corrupt. The count cap has been removed (the loop advances at least 12 bytes per round and chunk lengths
> are already bounds-checked, so it naturally cannot run wild), and a regression test added to lock this in.

---

## III. Verification Record

| Test | Result |
|---|---|
| Geometry unit tests (aspect-ratio/distortion/overflow/degenerate inputs) | **227/227** |
| Image header parsing (39 real PNGs + 15 malformed samples) | **54/54** |
| Corrupt file end-to-end filtering (text/truncated/normal/non-existent) | **4/4** |
| Size garbage values | **0/200** (200/200 before the fix) |
| Infinite redraw invariant | **1000→1** |
| Rendering end-to-end (old vs. new pixel comparison) | Average difference 2.03, **alpha difference 0.00** |
| Window following (aspect-ratio/no distortion/no overflow/AlphaBlend return value) | All passed |
| Concurrency stress test (8 threads × 300 iterations × 12 rounds, forcing sweep triggers) | **12/12 zero crashes**, GDI handles sawtoothing but bounded (629~1959, no monotonic growth) |
| Memory upper bound (39 images × 3 rebuild rounds) | Residue **constant +66 MB, +0 MB per round** |

---

## IV. Build

See `DLL源码\build.cmd`. For the shell, see `外壳源码\build.cmd`.
