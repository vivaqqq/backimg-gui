# backimg Background Plugin · Feature Comparison (Original → Fixed Build v1.3.28)

- **This document answers one question**: what does the original open-source author's DLL do? And as we iterated all the way to v1.3.28, what was added and what was fixed?
- **Intended readers**: ordinary users (no coding knowledge required)
- **To see the technical details of every single change**:
  v1.3.15 → `../dll/changelog/v1.3.15.md` (item-by-item list of 30 entries)
  v1.3.16 → `../dll/changelog/v1.3.16.md` (item-by-item list of the aspect-ratio-preserving scaling)
  v1.3.17 → `../dll/changelog/v1.3.17.md` (item-by-item list of 11 issues)
  v1.3.18 → `../dll/changelog/v1.3.18.md` (Qwen audit verification, item-by-item list)
  v1.3.19 → `../dll/changelog/v1.3.19.md` (Kimi re-review verification, item-by-item list)
  v1.3.20 → `../dll/changelog/v1.3.20.md` (random rotation rework, item-by-item list)
  v1.3.21 → `../dll/changelog/v1.3.21.md` (Deepseek audit, 8 items, item-by-item list)
  v1.3.22 → `../dll/changelog/v1.3.22.md` (Deepseek re-review hardening, 4 items, item-by-item list)
  v1.3.23 → `../dll/changelog/v1.3.23.md` (Kimi independent re-review, 5+1 items, item-by-item list)
  v1.3.24 → `../dll/changelog/v1.3.24.md` (Hunyuan 4 final audit, 2+1 items, item-by-item list)
  v1.3.25 → `../dll/changelog/v1.3.25.md` (second-level rotation + directory self-healing, item-by-item list)
  v1.3.26 → `../dll/changelog/v1.3.26.md` (version-synced release: zero DLL changes, UI moved to a dedicated sub-row)
  v1.3.27 → `../dll/changelog/v1.3.27.md` (Deepseek V4 Pro review, three fixes, DLL still unchanged)
  v1.3.28 → `../dll/changelog/v1.3.28.md` (Deepseek re-review, three fixes: P1 restore-configuration state flag / P2 dead variable / P3 re-selecting Custom keeps its value)
- **To see the complete story** → `../dll/changelog/iteration-report-master.md` (original → v1.3.15 → … → v1.3.28, all in one volume)

---

## 1. What This DLL Originally Did (Original Features)

This little plugin, written by open-source author Maple (winmoes.com) in 2021, does exactly one thing:

> **Replace the white background in folder windows with a picture you like.**

How it works: it registers as an add-on for Explorer and "hitches a ride" during Explorer's window painting — after the white background is laid down and before the icons and text are drawn, it slips the image in underneath. That is why icons and file names are unaffected.

Features of the original (the DLL's own version number 1.1.0.1, 221,696 bytes):

| Feature | Description |
|---|---|
| Background image in folder windows | Images live in the `plugin\Image\` directory (or a custom directory specified in the panel) |
| Random rotation across multiple images | Put multiple images in the directory; each new window randomly picks one |
| Placement modes and transparency | Several image placement/positioning modes (switchable in the panel) + an image transparency slider |
| Per-folder background | Assign a dedicated image to one specific folder (the "Per-folder background" page in the panel) |
| File dialog background (folderExt) | The open/save file dialog can get a background image too |
| Only two image formats recognized | **Only .png and .jpg; all other formats are ignored outright** |
| No logging at all | When something goes wrong there is no record — all you can do is guess |

The accompanying backimg Panel handles configuration changes, DLL registration/unregistration, and restarting Explorer.

## 2. What v1.3.15 Added

| # | New feature | What it means for you |
|---|---|---|
| 1 | **Image formats expanded to 7**: .png .jpg .jpeg .bmp .gif .tif .tiff (case-insensitive) | BMPs and GIFs (first frame) from your phone, camera, or old software can now be used directly as backgrounds — no format conversion needed first |
| 2 | **Runtime log** `plugin\ExplorerBgTool_init.log` | Which step failed and why — open the log and see at a glance, no guessing |
| 3 | **Bad-file rejection** | Bad files disguised as images (half-finished failed downloads, renamed extensions) no longer get into the random pool and waste slots |

## 3. v1.3.16 New: Aspect-Ratio-Preserving Background Scaling + Live Window Following (Major Feature)

This was the biggest feature upgrade in the entire fix line.

**The problem before** (true of v1.3.15 and the original):
Images were hard-pasted onto the window at their **original pixel size** — an image larger than the window simply overflowed and you could not see the whole picture;
even choosing the "Scale" position actually used the crudest nearest-neighbor interpolation to enlarge — your portrait photo blown up to fullscreen became 2.5x, **a mosaic of pixels and distorted**.

**Now** (from v1.3.16 on):

| # | New capability | What it means for you |
|---|---|---|
| 1 | **All seven display positions scale proportionally, never distorted** (at your request, the last position "Scale and Fill" keeps the original rule unchanged) | Portraits and landscape photos, no matter how large, keep the exact proportions of the original — never stretched fat or squashed flat |
| 2 | **Follows the window live at any size**: dragging, maximizing, fullscreen — the image grows and shrinks with the window, preserving aspect ratio | Fills the screen even in fullscreen, never extending past the window |
| 3 | **High-quality interpolation** (GDI+ bicubic) | Smooth when enlarged, no more mosaic |
| 4 | **Scaling cache** | No recomputation while dragging the window — no stutter |
| 5 | **Transparency fully preserved** | PNGs with transparent backgrounds stay transparent (measured bit-for-bit identical alpha with the old version) |

**Quick reference for the seven positions** (the row of buttons under "Image display position" in the panel):

| Button | Behavior |
|---|---|
| Top-left / Top-right / Bottom-left / Bottom-right / Center | If the image fits, it is placed at original size (small images are not blown up into blur); **if it does not fit, it is automatically scaled down proportionally, no longer overflowing the window** |
| **Scale** | **The whole image fits the window proportionally in full, growing and shrinking with the window live** (may leave margins around the edges) — pick this one if you want the image to follow the window |
| Scale and Fill | Fills the window proportionally, centered, cropping the overflow (same as the original behavior, unchanged) |

> Note: the semantics of the "Scale" position changed from the old version — the old "Scale" **stretched to fill (with distortion)**;
> from v1.3.16 it became **full proportional fit (no distortion, may leave margins)**. To fill without margins, use "Scale and Fill".

## 4. What v1.3.17 Fixed (user-visible problems introduced/exposed by v1.3.16)

| # | Problem | After the fix |
|---|---|---|
| 1 | **Unbounded memory growth**: every time you switched image directories, Explorer permanently retained about 177MB more | Residue is bounded (39 images ≈ a constant 66MB); repeated directory switches add zero growth |
| 2 | **A landmine that could crash explorer**: the thread cache hung off Windows thread-local storage, and the MinGW compiler would run its destructor on already-freed memory when a thread exited — any window-thread exit could crash | Fully switched to manual lifecycle management; freed as soon as a window closes |
| 3 | **CPU maxed out when the image directory was empty or files corrupted**: every frame self-triggered a repaint (measured 1000/1000 frames) | Converged to 1 |
| 4 | **GDI handle exhaustion from accumulation**: after repeatedly opening and closing windows, handles hit the 10000 cap and the background stopped drawing | Dead thread caches are swept automatically; handles are bounded and do not grow monotonically (measured sawtooth of a few hundred to about 1300) |
| 5 | Bad files could actually reach the decoder (a new exposure introduced by v1.3.16) | Lightweight image-header parsing screens first: only confirmed-corrupt files are rejected; anything it cannot understand is left to the system to judge — **normal images are never wrongly rejected** (all 39 of your images passed) |

Also a small UI fix: the DLL version number shown in the panel was corrected to 1.3.17 (v1.3.16 had missed it and still said 1.3.15).

## 5. What v1.3.18 Fixed (user-visible, after Qwen audit verification)

After v1.3.17 shipped, Qwen 3.8 Flash ran a round of memory-safety auditing over the DLL source; we verified each finding against the source code:
valid ones were fixed, invalid ones were rejected in writing (the full rejection rationale is in the changelog; recorded here faithfully).

| # | Problem | After the fix |
|---|---|---|
| 1 | **A stray GDI handle leak**: in one rare failure branch the scaling result leaked a handle; under handle pressure, hitting it meant the background inevitably failed to draw (a self-locking failure) | Fixed; all failure paths now release cleanly |
| 2 | **Cache completely wasted after changing the image set**: adding/removing image files rebuilt the list, the scaling cache's "keys" were renewed along with the objects, and every thread wasted a full recomputation pass | The cache is now addressed by **image content** — the same image hits the cache no matter how the list is rebuilt (self-check measured: zero memory growth after rebuilds) |
| 3 | **Deadlock/dangling-pointer hazards on the DLL unload path**: the unload cleanup code took locks at the system's most sensitive moments and deleted caches other threads were using | The DLL is pinned in the process and finalizes when the process exits (standard practice for shell extensions); the dangerous cleanup code was removed wholesale |
| 4 | **Two old landmines in the COM layer present since the original**: the factory object's reference counting was written on the wrong line (a net +1 on every add/remove); when the unload convention called SetSite(NULL) it would null-pointer crash | Both fixed; fixing the count deliberately avoided the trap where "changing that one line directly crashes the static object" (the factory was moved to proper heap allocation) |

UI change: **removed the "Elevated / Not elevated" pill from the status bar** — it showed the panel process's own administrator status,
statically unchanging and unrelated to the DLL registration state; sitting right next to "DLL not registered" it was highly misleading, so it was deleted.

Also, in all honesty: P0-1 in the Qwen report, flagged as "the most severe in the whole package" (cross-thread destruction of scaling handles), was verified line by line and the **mechanism does not hold** — the cache is per-thread private; other threads simply cannot touch entries in your thread's cache, so it was not fixed;
but the real unload-path problem it dragged into the light (item 3 above) does exist, and was fixed along with it.

## 6. What v1.3.19 Fixed (user-visible, after Kimi re-review verification)

The third-party audit rotated to Kimi K3.1 Pro; all 14 findings were verified one by one and all proved valid — 13 were fixed and
1 was handled with a compromise (the cache's residency of a single oversized image; see section 4 of the changelog for details):

| What it means for you | What was fixed |
|---|---|
| Extreme images cannot drag explorer down | A 100000x1 "thin noodle" image under "Scale and Fill" used to make the old version request an allocation of about 434GB; now, if the target bitmap exceeds 64 million pixels (256MB), drawing is abandoned outright — zero impact on normal wallpapers (including 8K fullscreen) |
| Memory errors no longer take explorer down with them | Exception boundaries added to all COM exits and hook callbacks: allocation failures are swallowed in place — that frame simply has no background |
| A better-behaved decoder | Fixed the "GDI+ stream released early" API violation; decode/draw states fully checked so bad images never enter the cache; single files over 256MB are rejected; if GDI+ startup fails, the background is cleanly disabled |
| More stable with multiple windows | Initialization switched to a real synchronization primitive (no race when multiple window threads enter at once); three configuration items became atomic variables, eliminating data races with the drawing thread |
| Corrected directory/mode switch behavior | Stale backgrounds no longer linger after the image directory is deleted/disconnected; non-random mode skips bad images and keeps looking for good ones; switching to random mode correctly rebuilds the full image set |
| More reliable register/unregister | Registry write failures are reported faithfully; the unregister path is unified with the register side; JPEG header parsing no longer wrongly rejects images that are "unusual in structure but valid" |

## 7. What v1.3.20 Did (random rotation rework, user-visible)

Users reported that "it never felt quite random"; verification confirmed three real causes, all fixed, plus timed rotation added per a feature request:

| What it means for you | What was done |
|---|---|
| "Random" is no longer the same sequence after a restart | The old random seed was only second-precision, so an explorer restart within the same second reproduced the previous "random" result verbatim; replaced with a high-quality random source (Mersenne Twister mixed with the system entropy source); measured: even distribution across 41 positions × 40,000 draws, and zero out-of-bounds across 160,000 draws on 8 threads |
| Multiple tabs/windows each draw independently | In the old version, opening a second tab on the same thread overwrote the first tab's record — both tabs drew the same image (one of the main reasons it "didn't feel random"); now each window keeps its own books and none overwrite another |
| New feature: timed rotation | Pick an interval to the right of the random row in the panel (no rotation/1/2/5/10/30 minutes/custom whole minutes); each folder window independently switches to a new random image every N minutes; defaults to "no rotation", so behavior is unchanged after upgrading |
| Zero harm from invalid values | randomMin only accepts whole minutes (the input box physically will not take a decimal point); out-of-range values are auto-clamped to 1 day; the configuration file is handled defensively as untrusted input |

## 7.5 What v1.3.25 Did (second-level rotation + custom directory self-healing, user-visible)

### ① Timed rotation now supports seconds (your red/green-box feature request)

| Change | Description |
|---|---|
| Custom time box narrowed | The input box shrank to digits only (at most 5 digits: 86400), making room for the new unit box |
| **New "Minutes/Seconds" unit dropdown** | Behind the input box (where the green arrow is); pick Seconds and you can enter values like 10, 30, or 90 — the image rotation interval is now completely free |
| Configuration key upgrade | `randomVal` (seconds) is the authoritative key; the old key `randomMin` is still read for compatibility and is still written in sync on save |
| Unit-switch rule | Switching to "minutes" with a value that is not a whole minute (say 90 seconds) auto-rounds to minutes with a note in the log; the "seconds" position is always exact |

### ② Fix for the custom image directory "occasionally not showing" (the bug you reported on real hardware)

**Symptom recap**: with the directory set to `G://MyPics`, occasionally the whole window had no background; switching back to the program directory Image fixed it;
switching back to G: fixed it again — not deterministic, intermittent.

**Root cause**: a directory "existing" (the system can see it) is not the same as "its files can be listed right now". When a disk is not ready
(an external drive just waking up, a volume mounting late after a reboot) or antivirus transiently locks the directory, enumeration can momentarily fail to get any handles at all;
the old version then declared "no images" and cleared the background, **with no automatic recovery afterwards** — the only chance to retry was coincidentally opening a new window.

**After the fix (three layers)**:

| Layer | What it does |
|---|---|
| Path normalization | Trailing stray `\`, quotes, and spaces are cleaned automatically — hand-edited ini files cannot be ambiguous |
| Transient retry | Directory exists but contents cannot be listed → retry in place a few times immediately (genuinely empty directories are unaffected; window opening is not slowed) |
| **Automatic self-healing** | If that still fails, it auto-reloads every 3 seconds; the moment the directory recovers, **already-open windows do not need reopening — the background comes back by itself** |

## 7.6 What v1.3.26 Did (rotation custom box moved to a dedicated sub-row, the fourth UI change)

| Change | Description |
|---|---|
| Custom box + unit box relocated | Moved out of the random toggle row into a **dedicated sub-row below it** (the exact empty space you boxed out), a full row of the "Custom interval" title + input box + minutes/seconds unit box |
| Widths fully opened up | Input box 42→80 (fits 6 digits), unit box 56→88 — **the words "Minutes"/"Seconds" now display in full**, and the numbers are no longer cramped |
| Top row back to clean | To the right of the random row there is now only "Timed rotation" + the preset dropdown; the caption text stays fixed and no longer slides sideways |
| Hint in non-custom states | The sub-row shows "Pick "Custom…" above, then enter the rotation interval here"; when random is off, the hint hides along with it |
| No layout jumping | The sub-row space is permanently reserved; switching presets never makes the layout expand/collapse |
| Zero DLL changes | Configuration keys, protocol, and timer behavior are identical to v1.3.25; the two DLL builds are directly interchangeable |

## 7.7 What v1.3.27 Did (Deepseek V4 Pro review, three fixes)

v1.3.26 was sent out for review by Deepseek V4 Pro; all three problems were verified one by one and fixed (verification details in section 0 of the changelog):

| Fix | Description |
|---|---|
| Sub-row title show/hide | Turning off "random image rotation" now hides the "Custom interval" title together with the sub-row controls — no more orphan line of text left behind |
| Dead variable cleanup | The five leftover UI-position global variables from v1.3.25 were fully removed from file scope (zero warnings on rebuild) |
| No preset jump in custom state | In custom state, typing 60/120/300 etc. that exactly equals a preset no longer makes the input box vanish on the spot or the value snap back to the preset — detection now uses a separate state flag meaning "the user actively chose Custom" |
| Zero DLL changes | Configuration keys, protocol, and timer behavior are identical to v1.3.25/26 |

## 7.8 What v1.3.28 Did (Deepseek re-review, three fixes, zero UI changes)

The re-review first checked v1.3.27's three fixes (F1/F2 correct, F3 achieving its main scenario), then found three new problems, each verified valid:

| Fix | Description |
|---|---|
| P1 restore-configuration state flag | When "Restore configuration" recovers a preset (e.g. "every 1 minute") from a backup, the UI no longer wrongly stays on the "Custom…" preset — the custom-state flag is reset to zero when the configuration is re-read |
| P2 dead variable removal | Caught the hRotLabel that slipped through the last round (after v1.3.26 removed the "caption sliding sideways", it had no readers from the moment it was created) |
| P3 re-selecting Custom keeps its value | With 120 already entered in custom state, clicking "Custom…" again **no longer resets it to 3 minutes** — the 180 starting point is given only when first entering Custom from a preset |
| Zero DLL changes | Configuration keys, protocol, and timer behavior are identical to v1.3.25/26/27; the five DLL builds are directly interchangeable |

## 8. Which Original Bugs v1.3.15 Fixed

### The catastrophic ones (real in the original, all actually hit in practice)

| # | Original problem | Consequence | After the fix |
|---|---|---|---|
| 1 | When the image directory was empty/had no images, a **popup** asked you what to do | The popup froze Explorer's UI thread → **the entire desktop went white** (the true culprit of that infamous incident back then) | No popup; a log line is written and it carries on |
| 2 | Under edge conditions, double initialization would **terminate Explorer's UI thread** | The desktop went straight to a white screen | This self-destruct logic was deleted outright |
| 3 | State corruption when some of the six internal hooks failed partially | Unpredictable behavior afterwards | Enabled only if all succeed; any failure automatically rolls back all of them and logs it |
| 4 | After images were removed, old windows still remembered now-invalid indices | Crashed Explorer outright | Automatically falls back to the first image |
| 5 | Corrupted configuration file (letters where digits belong) | Threw an exception, taking Explorer down | Safely ignored; defaults used |
| 6 | Another window happened to be painting at the exact moment of an image switch | It used just-freed memory → crash or corrupted display (technically, use-after-free) | Reworked into a "borrow ticket" mechanism; 301 million reads in external stress testing with zero misses; **your 2026-10-23 real-machine test: adding/removing images, rotating images, and multiple windows all normal** |

### Chronic consumption and hidden hazards

| # | Original problem | After the fix |
|---|---|---|
| 7 | Two memory leaks, about 1.5KB per window opened, adding up over the years | Fixed, zero leaks |
| 8 | Stayed resident in any program (did not leave when other software loaded it) | Only two processes are recognized — Explorer and the registrar; every other process exits immediately |
| 9 | Left a dead registry key after uninstall that could never be cleaned | Deleted during unregistration |
| 10 | Painting state markers were retrieved by "undo the most recent one", which could theoretically pop someone else's | Retrieved by marker ID, by name |
| 11 | Internal hazards such as painting sessions being mistakenly closed by other windows and lock-free log deduplication | All hardened (see the changelog for details) |

### What was fixed, in one sentence

> **The original was a machine that "worked, but hit any landmine and you'd get a white screen, a crash, or a memory leak"; the fix line filled in every known landmine,
> added the new aspect-ratio-preserving scaling feature (v1.3.16), then filled in the landmines that new feature itself introduced (v1.3.17),
> then went through a second pair of eyes (the Qwen audit), re-checking item by item — fixing what held up, rejecting what didn't (v1.3.18),
> then a third pair of eyes (the Kimi re-review) dug up 14 deep problems, all valid, all fixed (v1.3.19),
> then "random" was rebuilt from scratch — a true random source, per-window bookkeeping, plus timed rotation (v1.3.20),
> then a fourth pair of eyes (the Deepseek audit) found 8 problems, all valid, all fixed — including the deterministic bug "timed rotation
> button panel blank" reported from a user's real machine (v1.3.21), and the re-review hardened 4 more
> hidden hazards (cross-thread destruction ownership, TOCTOU invariants, the INT_MAX magic number, timer semantics) (v1.3.22),
> then Kimi did another independent source read-through, fixing 5+1 memory-safety and boundary issues such as an unchecked realloc
> on the "deletion rules" path (v1.3.23), and finally the Hunyuan 4 fourth audit achieved the first double zero of P0/P2, leaving only two
> one-line defenses (buffer upsizing + hook null checks) — the audit loop concluded there (v1.3.24),
> after which the cadence returned to features by user request: rotation intervals down to seconds + a three-layer self-healing
> fix for the custom image directory occasionally not showing (v1.3.25),
> and throughout it all there was an extra pair of eyes (the log) and a whole self-check suite (exit codes fixed into a real gate),
> so breakage is easy to diagnose and changes are easy to verify.**

## 9. What the Original Has That the Fixed Build Does Not (honestly disclosed)

| Original feature | Fixed build | Description |
|---|---|---|
| Per-folder background | ✗ Not present | Setting that page in the panel **has no effect** |
| File dialog background (folderExt) | ✗ Not present | Setting folderExt=true in the config **has no effect whatsoever** |
| Multi-window tracking in extreme scenarios | Simplified | No impact in daily use — your real-machine testing with a pile of windows open showed all backgrounds normal; only the extreme scenario of "one UI thread simultaneously showing multiple windows each with a different background" is theoretically supported only by the original |

**Why they are missing**: the fix line started from the source the author published on GitHub, while the three items above were later evolutions the author added to the zip package but **never pushed to GitHub** — a difference between two lineages, not something lost in the fixing.
If ever needed, a restoration plan is ready (recorded in the changelog).

## 10. One Master Table (Original vs Fixed Build v1.3.28)

| Capability | Original v1.1.0.1 | Fixed build v1.3.28 |
|---|---|---|
| Background image in folder windows | ✓ | ✓ |
| jpg / png | ✓ | ✓ |
| bmp / gif (first frame) / jpeg / tif / tiff | ✗ | **✓** |
| Random rotation across multiple images | ✓ | ✓ (with invalid-index fallback) |
| Placement modes / transparency | ✓ | ✓ |
| Custom image directory | ✓ | ✓ |
| Image larger than the window | **Overflows; full picture not visible** | **✓ All scale proportionally, never out of bounds** |
| Image grows/shrinks with the window | ✗ (nearest-neighbor stretch only, distorted + mosaic) | **✓ "Scale" position follows live, aspect-ratio preserved** |
| Transparent backgrounds | ✓ | ✓ (alpha bit-for-bit identical after scaling) |
| Per-folder background | ✓ | ✗ |
| File dialog background | ✓ | ✗ |
| Diagnosable when something breaks | ✗ No logs | **✓ init.log** |
| White-screen/crash/corrupt-config resistance | ✗ Five crash points | **✓ All hardened** |
| Memory leaks | Yes (two) | **✓ Fixed, and directory switching no longer accumulates (bounded)** |
| Image-switch race condition | Hazard present | **✓ Eliminated at the root (300 million-operation stress test)** |
| Bad files disguised as images | Admitted, wasting slots | **✓ Rejected by header parsing (no false rejections)** |
| Residency in non-target processes | Yes | **✓ Only Explorer is recognized** |
| Thread-exit crash landmine / GDI handle exhaustion | — (introduced in v1.3.16) | **✓ Fixed in v1.3.17 (handles bounded, no accumulation)** |
| Scaling cache hits across rebuilds | — (cache introduced in v1.3.16) | **✓ Content-addressed in v1.3.18, zero recomputation when the image set changes** |
| DLL unload safety / COM reference counting | A typo and unload hazards | **✓ Fixed in v1.3.18 (DLL pinned, finalizes with process exit)** |
| Extreme image protection / exception boundaries | None (extreme aspect ratios could trigger huge allocations) | **✓ Fixed in v1.3.19 (256MB target cap + exception guards on every boundary)** |
| Random quality / multi-window randomness | rand() + second-precision seed, reproducible; multiple tabs shared one slot and overwrote each other | **✓ Fixed in v1.3.20 (true random source + per-window bookkeeping) + timed rotation added** |
| Timed rotation interval granularity | — (not in the original) | **✓ v1.3.25 supports down to seconds (randomVal; old key randomMin auto-compatible); v1.3.26 moved the custom box to a dedicated sub-row below, with fuller display of numbers and units** |
| Custom directory occasionally not showing | — (a transient gap existed once custom directories took effect in v1.3.18) | **✓ Three-layer fix in v1.3.25 (path normalization + transient retry + 3-second automatic self-healing, no window reopening needed)** |
| Multi-tab scaling cache / rotation countdown | — (regressions introduced in v1.3.20: closing one tab cleared the other tabs' caches; opening a new window reset everyone's countdown) | **✓ Fixed in v1.3.21 (released after per-thread bookkeeping + reinstall only on config change); v1.3.22 re-review hardened cross-thread destruction ownership** |
| Accompanying panel | ✓ | ✓ v1.3.28 (sub-row layout + six fixes converged over two rounds of outsourced review); v1.3.21 fixed the "Timed rotation" dropdown echo |

## 11. How to Tell Which Version Is Currently Running on Your Machine

Open `plugin\ExplorerBgTool_init.log` and look at the first line:

- `v1.3.28` → the current build is running
- `v1.3.25` / `v1.3.26` / `v1.3.27` → an older build (functionally identical to the current one; only UI details are older)
- `v1.3.15` / `v1.3.16` / `v1.3.17` or no such line → an older build or the original is running

Rollback: overwrite with the old DLL and restart Explorer — the configuration file needs no changes
(thirteen complete old packages, v1.3.15 ~ v1.3.27, are kept on the desktop, each with its own DLL).

---

*Document date: 2026-10-29 ｜ This package: backimg_GUI_v1.3.28 (shell v1.3.28 + DLL version-synced build v1.3.28)
｜ DLL: 942,255 bytes, SHA256 7846debd…83C18E7 ｜ Iterated per user requests after the audit concluded + cross-validated by outsourced reviews ✅*
