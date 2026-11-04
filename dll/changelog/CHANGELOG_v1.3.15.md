# backimg Fixed Edition v1.3.15 · Changelog (R5 Final)

- **Release date**: 2026-10-23 (three rounds of revision on the same day, R2 → R3 → R4, incorporating two external audits + one re-review)
- **Version number note**: What you are using is the author's original v1.3.14 (the DLL's own version number is 1.1.0.1).
  This release is named v1.3.15, which is an unused number on the DLL version line, falling exactly after the author's version.
  Historically, the GUI panel used the numbers v1.3.15~v1.3.17, and the DLL fix line used v1.3.16~v1.3.20;
  both are unrelated to this naming (the old lines have been deprecated and archived).
- **Contents**: All cumulative fixes since the author's version, **30 in total**
  (11 feature fixes + 6 R2 audit revisions + 4 R3 re-review revisions + 4 R4 re-review revisions + 3 R5 re-review revisions + 2 documentation corrections).

## I. Feature Fix List (11 items)

| # | Fix | Description |
|---|---|---|
| 1 | Removed two modal popups in LoadSettings | The true cause of the white screen: the explorer UI thread was frozen by the popup. Missing directory/missing image now only writes to the runtime log |
| 2 | Removed FreeLibraryAndExitThread | On double initialization it would kill itself on the explorer UI thread |
| 3 | m_isInitHook set before initialization | Initializes only once; whether it succeeds or fails, it will never enter a second time |
| 4 | Hook "all-or-nothing" | Enabled only if all 6 hooks succeed; on failure, rolled back one by one; result written to ExplorerBgTool_init.log |
| 5 | ImgIndex out-of-bounds protection | After the image list shrinks, stale indexes would directly crash explorer |
| 6 | GetIniString uninitialized fix | When file reading failed, it would originally allocate memory with garbage values |
| 7 | Two memory leak fixes | GetWindowClassName leaked 1,560 bytes per window; GetWindowTitle leaked + lost one character |
| 8 | Dirty configuration protection | posType=abc no longer throws an exception that takes down explorer |
| 9 | Draw session nesting counter | Prevents another window inserted mid-session from mistakenly closing the draw session |
| 10 | Format whitelist | Added *.jpeg *.bmp *.gif *.tif *.tiff (webp unsupported, no decoder; gif takes the first frame) |
| 11 | RestoreDC restores by exact ticket number | No longer uses -1 to pop "the most recent one" |

## II. R2 Audit Revisions (6 items)

Based on 《审计报告_对比上游原版.md》 (produced by Minimax-M3.1 Flash, verified item by item by WorkBuddy (GLM) before adoption):

| # | Revision | Corresponding audit item |
|---|---|---|
| R2-1 | **DllMain length guard + towlower**: when the process path is shorter than 12 characters, substr underflows and throws an exception; throwing an exception inside DllMain is tantamount to killing the host process | §4.1 (P0) |
| R2-2 | **Process gating**: only explorer.exe and regsvr32.exe are allowed; other processes return FALSE and are unloaded immediately by the system | §4.2 (P0) |
| R2-3 | **Unregistration additionally deletes leftover key**: Drive\shellex\FolderExtensions\{CLSID} (registered by the author's version, not removable by the original unregistration) | §4.2 (P1) |
| R2-4 | **LoadSettings differential rebuild**: if the file set hasn't changed, nothing is deleted or rebuilt, eliminating "full re-decoding on every window open" and the associated release race | §4.3 (P0) |
| R2-5 | **Corrupt-file rejection**: gated by GetLastStatus + width/height; corrupt files disguised as images are no longer accepted into the list | §3.10 (P1) |
| R2-6 | SaveDC failure no longer clips + AppendLoadLog deduplication state placed under lock | §3.11 / §4.7 |

## III. R3 Re-review Revisions (4 items)

Based on 《审计报告_R2复审_对比上游.md》 (the re-review confirmed all five R2 items correct, and pointed out that the use-after-free was only half fixed):

| # | Revision | Corresponding re-review item |
|---|---|---|
| R3-1 | **Use-after-free eradicated (P0)**: imageList changed to `vector<shared_ptr<BitmapGDI>>`. On rebuild, the new list is decoded outside the lock and the table is swapped in a moment inside the lock; on the drawing side, a "token" (shared_ptr) is taken out inside the lock and drawing happens after the lock is released — with the token in hand, the object can never be destroyed prematurely; stepping into a dangling object is impossible by mechanism | Re-review §三 R2-4 / §五 P0 |
| R3-2 | **m_duiList switched to std::map** (node-based): insertion/deletion never invalidates iterators held by other threads, eliminating the rehash-invalidation class of problems | Re-review §五 P1 |
| R3-3 | **Process gating de-magicked**: the file name is taken after the last backslash for a full-name comparison, no longer relying on "exactly 12 characters" | Re-review §五 P2 |
| R3-4 | **Self-test programs delivered with the package** (three exes in selftest\ + all harness sources) | Re-review §六 |

## IV. R4 Re-review Revisions (4 items)

Based on 《审计报告_R3复审_对比上游.md》 (the re-review confirmed R3-1 is real by calibrated probe measurement and R3-3 entirely correct;
pointed out that R3-2 was only half fixed, plus two self-test delivery defects):

| # | Revision | Corresponding re-review item |
|---|---|---|
| R4-1 | **m_duiList removed entirely, replaced with thread_local** (P1): this table is keyed by thread ID and each thread only touches its own record; a shared container is pure unnecessary coupling — with thread_local, "looking up the table" becomes "looking at yourself": zero locks, zero UB. Better than the locking approach suggested by the re-review: there is no "holding a lock across AlphaBlend" problem, and the code is shorter | Re-review §三 R3-2 / §六 P1 |
| R4-2 | **Regression self-test sample images generated on-site**: harness18 draws sample.png on the fly with GDI+, no longer depending on external files — running directly in an empty directory passes 9/9 (the re-review measured 7 failures when running the original package directly) | Re-review §四 Defect 1 |
| R4-3 | **harness15/18 exit order corrected**: bitmaps are cleaned up before GdiplusShutdown (aligned with the race harness) — fixing an exit-time segfault (the re-review measured exit=139) | Re-review §四 Defect 2 |
| R4-4 | s_lastFiles write moved inside the lock (consistent read/write discipline) | Re-review §五 P3 |

## V. R5 Re-review Revisions (3 items, all documentation/comment-level, no logic changed)

Based on 《审计报告_R4复审_对比上游.md》 (the re-review confirmed all four R4 items landed, the thread_local approach is better than suggested,
and there are no new P0/P1):

| # | Revision | Corresponding re-review item |
|---|---|---|
| R5-1 | **Per-HWND ownership wording corrected (P2)**: "does not include features evolved only in the author's version" changed to "**features lost relative to the v1.3.14 you are using**", noting two layers of fact: the inner layer of the author's version's MyData is a map<HWND,data> (the re-review's citation is accurate); the GitHub upstream is inherently single-window — the gap is inherited from the lineage baseline, not a regression introduced by the fixing process | Re-review §四 |
| R5-2 | harness18 sample image readiness check moved to after on-site generation (eliminating the self-contradictory message of "reporting missing first, then generating") | Re-review §六 P3-1 |
| R5-3 | MyData given a TLS comment stating "must remain a trivial type" (introducing a non-trivial member in the future would require re-evaluating TLS destruction timing) | Re-review §六 P3-4 |

R5 items not adopted and why: race probe upgrade (the detection sensitivity limit was calibrated in the R3 re-review and its conclusion faithfully cited;
no unverified third-party probe is being crammed into the delivery package); init.log cleanup by selftest (harmless, it is troubleshooting evidence);
per-HWND restoration (a plan is ready. **The real-machine gate was passed on 2026-10-23**: backgrounds in all windows displayed normally, so the restoration plan is shelved — see §七 lost features entry.)

## VI. Verification Record (R5 final state)

- Seven compiles, all zero errors (DLL + three self-test programs, each R5 comment-level change rebuilt once)
- **Concurrency stress test (R4 final state)**: the rebuild thread flipped the file set nonstop, forcing the "table swap" path 2,823 times; the drawing thread dereferenced in the real image-fetch manner **301 million times — zero dangling dereferences, zero dirty data**, clean exit
- Multi-format self-test **6/6 PASS** (all 7 extensions accepted, *.webp rejected, disguised .png corrupt file rejected, large image accepted, empty directory/dirty config regressions; differential early return measured at 1ms)
- Regression self-test **9/9 PASS ×3** (once each with sample images and running directly in an empty directory, empirically verifying the fix for re-review defect 1; one more run after the R5 message-order correction)
- Binary final inspection: 7 wildcard present, version line present, no MessageBoxW / FreeLibraryAndExitThread, x64, all 4 COM exports present
- **Real-machine confirmation (2026-10-23)**: after the user installed it and opened multiple folder windows, background images all displayed normally — the per-HWND real-machine gate passed, single-record write finalized
- **Real-machine confirmation · 2 (2026-10-23)**: rotating images, adding/removing image files, opening new windows, random rotation — all correct, all real-machine observation points closed

## VII. Known Boundaries (honest disclosure)

- **Features lost relative to the v1.3.14 you are using** (please be aware before upgrading): per-directory image switching (OnDocComplete),
  **per-HWND multi-window tracking** — the inner layer of the author's v1.3.14's MyData is a `map<HWND,data>`,
  so the same UI thread can track multiple folder windows at once; this edition's lineage (GitHub upstream source) is inherently single-window
  (one record per thread), and is implemented that way. **This is not a regression introduced by the fixing process; it is a difference inherited from the lineage baseline.**
  On Win11 multi-tab, if tabs share the UI thread, switching tabs may lose the background — please test after installing;
  if it does happen, a restoration plan is ready (thread_local map<HWND,…>, preserving the lock-free advantage)
  **[2026-10-23 real-machine update: backgrounds measured normal across multiple windows, this risk ruled out, restoration plan shelved]**
- **folderExt (file dialog background image) is unsupported across the entire chain in this edition**: the registrar no longer creates the FolderExtensions key,
  and process gating only accepts explorer.exe / regsvr32.exe — setting folderExt=true in config.ini will have no effect.
  This is not "a blacklist was dropped"; that upstream feature path is entirely absent from this edition's lineage. Drawing backgrounds inside File Explorer windows is unaffected.
- **Replacing the contents of a same-named file** requires restarting explorer to take effect (if the set of file names is unchanged, no rebuild — same as upstream);
  adding/removing image files takes effect immediately
- After a differential rebuild, the meaning of ImgIndex drifts with changes in the file set — an old window may "switch to a different image" rather than keep the original
  (out-of-bounds clamping as a backstop, no crash; a small visible behavioral quirk that also exists when upstream reloads)
- No version resource (the version number is not visible in File Explorer properties); refer to the first line of init.log for the version
- In src/, ShellLoader.cpp/.h are GBK-encoded, the rest are UTF-8 — confirm the encoding before editing ShellLoader

## VIII. Build Command (reproducible)

```
<MinGW-w64安装目录>\bin\g++.exe -std=c++17 -O2 -shared -DUNICODE -D_UNICODE -Wall ^
  -Wl,--image-base,0x10000000 -o ExplorerBgTool.dll ^
  dllmain.cpp ShellLoader.cpp WinAPI.cpp ^
  minihook\hook.cpp minihook\buffer.cpp minihook\trampoline.cpp ^
  minihook\hde\hde64.cpp minihook\hde\hde32.cpp Source.def ^
  -lgdiplus -lmsimg32 -lshlwapi -lgdi32 -luser32 -lole32 -luuid -static
```
(Execute inside the src\ directory; --image-base is for reproducible builds — without it the linker picks a random base address each time despite identical code.
The R2 re-review independently reproduced a byte-identical DLL with this command.)

## IX. Fingerprint (R5 final state)

- ExplorerBgTool.dll: 891,738 bytes (same as R4 — all three R5 items are comment/documentation-level, zero code change;
  hash differences come only from the PE timestamp)
- SHA256: `fe8ae612c1519dcb6e37c56355f332189bba3e537bac36c4a17e3c1deacf453d`
