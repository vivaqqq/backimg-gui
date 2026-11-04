# backimg Panel (Shell) · Version Iteration Report (Master Volume)
**(Python v1.0.0 → C v1.3.14 full history · Continuation v1.3.15 ~ v1.3.28)**

- **Report date**: 2026-10-28 (master combined volume; continuation v1.3.25 ~ v1.3.28 added 2026-10-29)
- **About this volume**: this volume continues from the original "Version Iteration Report_v1.3.15.md"; after completing the thirteen chapters for v1.3.16 ~ v1.3.28 it was merged into the master volume,
  and the original separate volume has been deleted, with no content omitted. The shell had **zero interface changes** in v1.3.15~v1.3.17 (its design was finalized in v1.3.14);
  the first actual interface change came only in v1.3.18 (removal of the status bar "Elevated" pill) — each version is listed below to keep the iteration history from appearing to have a gap.
- **This directory**: the complete v1.3.28 shell source set — main.c (~5,120 lines), resource.rc, resource.res, app.ico,
  build.cmd, config.ini, plus the bundled DLL copy ExplorerBgTool.dll (resource.rc references it by the same filename; required for recompilation, do not delete)
- **Division of work**: the shell (C + Win32, single file) and the plugin DLL (C++ + MinHook) are two languages with two builds; sources are archived separately;
  for the DLL-side iteration history see `../dll/changelog/iteration-report-master.md`

---

## Chapter 1: v1.3.28 (2026-10-29 · Current) — Deepseek Re-Review Fix Release (Zero Interface Changes)

| Item | v1.3.27 | v1.3.28 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.27" | **"1.3.28"** |
| resource.rc version block | 1,3,27,0 | 1,3,28,0 |
| Bundled DLL | v1.3.27 (942,255 B) | **v1.3.28 (942,255 B)**: zero functional change, version-stamp sync only (see `../dll/changelog/v1.3.28.md`) |
| **Shell changes** | F1/F2/F3 | **P1/P2/P3** (the user sent v1.3.27 out for a Deepseek re-review; after the previous round's three fixes were verified correct, three new issues were found): **P1** (real bug) after `load_config` reads the rotation config, add `g_rot_custom_picked = false;` — the state bit added in the previous round's F3 missed the config-reload path, so when "Restore config" at the bottom restores a preset profile from the .bak file, the UI would wrongly stay on the "Custom…" profile; **P2** removed the dead variable `hRotLabel` (after v1.3.26 deleted the horizontal-shift logic its assignment had no reader; it slipped through when F2 cleaned the five globals), and new_label_ex's return value is no longer saved; **P3** when already in custom state (the user has typed a value), clicking "Custom…" again **does not reset the filled-in value** — previously the `preset` reset in place since v1.3.25 would knock 120 back to 180; now the 180 starting point is given only "when first entering custom from a preset profile/an invalid value" |
| Data/config model | v_rotsec + v_rotunit / randomVal + randomUnit | **Completely unchanged** |
| build.cmd | VER=1.3.27 | VER=1.3.28 |

**Nature**: review-fix release (zero interface changes) — the re-review report first verified v1.3.27's F1/F2/F3 (conclusion:
F1/F2 correct, F3's main scenario achieved), then raised three new issues; after item-by-item verification confirmed **all of them valid**, all were fixed
(verification details in Section 0 of `../dll/changelog/v1.3.28.md`). P1 is a real functional
bug (narrow trigger surface but inconsistent logic); P2/P3 are tidiness and experience issues, handled together.

Verification: DLL g++ zero errors zero new warnings (recompiled for the version-stamp sync); EXE gcc -Wall zero errors
zero warnings; EXE version resource 1.3.28.0; plugin DLL byte-identical to the build output
(SHA256 7846DEBD35F5F259…83C18E7, 942,255 bytes).

## Chapter 2: v1.3.27 (2026-10-29) — Deepseek V4 Pro Review Fix Release (Minor Interface Logic Adjustments)

| Item | v1.3.26 | v1.3.27 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.26" | **"1.3.27"** |
| resource.rc version block | 1,3,26,0 | 1,3,27,0 |
| Bundled DLL | v1.3.26 (942,255 B) | **v1.3.27 (942,255 B)**: zero functional change, version-stamp sync only (see `../dll/changelog/v1.3.27.md`) |
| **Shell changes** | E1 sub-row split | **F1/F2/F3** (fixed after item-by-item verification of the three issues in the user-submitted Deepseek V4 Pro review report): **F1** the sub-row title "Custom interval" now shows/hides together with the random toggle (previously, when random rotation was off, an orphaned title line remained); **F2** the five position globals left over from v1.3.25 (g_rot_cx/label_y/sub_y/edit_x/unit_x) had no cross-function readers after the horizontal-shift logic was deleted, so all were demoted to rot_ui_init local variables (the review only called out dead variables; this release cleans more thoroughly; the review's claim of "will definitely trigger -Wall warnings" was inaccurate — a file-level static assignment counts as a reference, and testing showed there were no warnings in the first place); **F3** the custom state now uses an independent state bit g_rot_custom_picked (set when clicking "Custom…", cleared when clicking a preset profile), so when the user manually types 60/120/300 or other values that happen to match preset seconds while in custom state, it no longer jumps profiles and the input box no longer disappears — the original approach suggested by the review; the display ambiguity from pure numeric back-derivation since v1.3.25 is hereby closed out |
| Data/config model | v_rotsec + v_rotunit / randomVal + randomUnit | **Completely unchanged** |
| build.cmd | VER=1.3.26 | VER=1.3.27 |

**Nature**: review-fix release — the user sent v1.3.26 out for a Deepseek V4 Pro review, which reported three issues;
after verifying their validity item by item, all were fixed (verification details and the "partially valid" clarification in
Section 0 of `../dll/changelog/v1.3.27.md`). The rotation conversion core logic
was confirmed bug-free by the review ("logic is self-consistent, safe to use"); this round touched only the display layer and state determination.

Verification: DLL g++ zero errors zero new warnings (recompiled for the version-stamp sync); EXE gcc -Wall zero errors
zero warnings; EXE version resource 1.3.27.0; plugin DLL byte-identical to the build output
(SHA256 85E19DF9FD4F36FB…EBDE2F86B, 942,255 bytes).

## Chapter 3: v1.3.26 (2026-10-29) — Rotation Custom Controls Moved to a Sub-Row (Fourth Interface Change)

| Item | v1.3.25 | v1.3.26 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.25" | **"1.3.26"** |
| resource.rc version block | 1,3,25,0 | 1,3,26,0 |
| Bundled DLL | v1.3.25 (942,255 B) | **v1.3.26 (942,255 B)**: zero functional change, version-stamp sync only (see `../dll/changelog/v1.3.26.md`) |
| **Shell changes** | D1 narrowed input box + unit dropdown squeezed into the toggle row | **E1** (user red-box request) — the custom interval input box + unit box move to a **dedicated sub-row** below the random row (height S(28), always counted into the layout), with widths freed up: input box S(42)→**S(80)** (fits 6 digits), unit box S(56)→**S(88)** (the word "Seconds" displays in full); the top row keeps only `[Timed rotation][Preset dropdown]`, and the caption text is fixed, no longer shifting horizontally; when not in custom state the sub-row shows the hint "After selecting "Custom…" above, fill in the rotation interval here" (hidden together when random rotation is off); show/hide happens only within the sub-row, **no layout jump when switching presets** |
| Data/config model | v_rotsec + v_rotunit / randomVal + randomUnit | **Completely unchanged** |
| build.cmd | VER=1.3.25 | VER=1.3.26 |

**Nature**: pure interface layout adjustment — v1.3.25 squeezed three controls into the random toggle row (input box 42,
unit box 56, "Seconds" clipped at the edge); the user pointed out "there is a lot of space below; moving them down would let the time display show
more fully". This round touched only rot_ui_init / rot_ui_sync and the layout reservation; rotation logic, config keys,
and save/load paths were not changed at all.

Verification: DLL g++ zero errors zero new warnings (recompiled for the version-stamp sync); EXE gcc -Wall zero errors zero warnings;
EXE version resource 1.3.26.0; plugin DLL byte-identical to the build output
(SHA256 1D4D7E882E350FD8…DBC2F766, 942,255 bytes).

## Chapter 4: v1.3.25 (2026-10-29) — Second-Level Rotation + Directory Self-Healing Release (Third Interface Change)

| Item | v1.3.24 | v1.3.25 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.24" | **"1.3.25"** |
| resource.rc version block | 1,3,24,0 | 1,3,25,0 |
| Bundled DLL | v1.3.24 (936,501 B) | **v1.3.25 (942,255 B)**: D1 second-level rotation + D2 directory self-healing (see `../dll/changelog/v1.3.25.md`) |
| **Shell changes** | N1 boundary hardening | **D1** (user red/green-box request) — the timed rotation custom time input box narrowed S(60)→S(42) (fits only 5 digits, 86400), with a new "Minutes/Seconds" unit dropdown added on the right (S(56), right next to the preset dropdown), and the caption text yields and shifts left in custom state; data model v_rotmin→**v_rotsec (seconds) + v_rotunit (display unit)**; new config keys `randomVal` (seconds) + `randomUnit`, with the compatibility key `randomMin` still written in sync; unit-switching semantics: switching to minutes applies only for whole minutes, otherwise it rounds down and writes a log entry; rot_ui_sync's display fallback writes v_rotunit back in sync (preventing "90 seconds being computed as 5400 seconds by the minute unit"); the copy summary's randomVal row displays for readability |
| DLL-side fixes | N2 hook null checks | **D2** (user real-machine bug) — when the custom directory pointed outside the program directory, backgrounds occasionally all went missing: N1 folder path normalization / N2 transient retry on enumeration (3×200ms; zero delay for genuinely empty directories) / N3 empty-list 3-second self-healing reload (all registered windows get a RETRY_TIMER_ID timer; after recovery the timer is cleared + a full redraw; newly opened windows are covered by the same fallback) |
| build.cmd | VER=1.3.24 | VER=1.3.25 |

**Nature**: after the audit loop wrapped up at v1.3.24, the project returned to its "user-demand-driven" rhythm — ① interface demand
(user red/green boxes: shrink the custom box + a second/minute unit dropdown, so the rotation interval can be freely set down to seconds,
"even 1 minute feels too slow"); ② a real-machine bug (custom directory pointing to G:\手机备份\Image occasionally failed
to display; root cause and the three-layer fix in Chapter 13 of the DLL master volume).

Verification: DLL g++ zero errors zero new warnings; EXE gcc -Wall zero errors zero warnings; DLL byte signature contains
v1.3.25 / randomVal / "每 %d 秒" ("every %d seconds", UTF-16 check); EXE version resource 1.3.25.0;
plugin DLL byte-identical to the build output (SHA256 B6A4966A…152474).

## Chapter 5: v1.3.24 (2026-10-29) — Hunyuan 4 Re-Review Closing Release (Zero Interface Changes)

| Item | v1.3.23 | v1.3.24 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.23" | **"1.3.24"** |
| resource.rc version block | 1,3,23,0 | 1,3,24,0 |
| Bundled DLL | v1.3.23 (936,501 B) | **v1.3.24 (936,501 B, same name, different content)**: N2 hook null checks (see `../dll/changelog/v1.3.24.md`) |
| **Shell changes** | K1~K4 four hardening fixes | **N1** — on the write side of on_rule_add, imgc[1024]→[12288] (including an analysis comment on the NTFS 255-character filename-length boundary); the two img/imgw spots on the read side enlarged in sync to align; verification confirmed that legal filenames cannot actually trigger truncation, so this is boundary hardening rather than a reproducible bug |
| DLL-side hardening | K5 session accepts only a valid DC | N2 null check for MyFillRect's lprc (the hook is more conservative than the original function) |
| build.cmd | VER=1.3.23 | VER=1.3.24 |

The auditor was Hunyuan 4 (the fourth independent audit): **the first time P0/P2 were both zero**, with only 3 P3 items; across four audits
P0: 1→0→0→0 and P2: many→1→1→0 converged monotonically, and each round confirmed that the previous round's fixes had no regression — **the audit
loop closes here**; future audits only when new features/new version changes are made.

Verification: EXE version resource 1.3.24.0; byte signature contains v1.3.24, zero v1.3.23 residue;
bundled DLL byte-comparison confirmed as the new v1.3.24; all self-checks passed (see Chapter 12 of the DLL master volume).

## Chapter 6: v1.3.23 (2026-10-29) — Kimi Independent Re-Review Hardening Release (Zero Interface Changes)

| Item | v1.3.22 | v1.3.23 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.22" | **"1.3.23"** |
| resource.rc version block | 1,3,22,0 | 1,3,23,0 |
| Bundled DLL | v1.3.22 (936,501 B) | **v1.3.23 (936,501 B, same name, different content)**: K1~K5 re-review hardening (see `../dll/changelog/v1.3.23.md`) |
| **Shell changes** | Zero code changes | **K1~K4 four hardening fixes** — K1 (P2) null checks for remove_folder_section's APPENDW2 realloc + the same-origin initial malloc (K1b) + the folded malloc (K2); K3 wWinMain's memset moved before CreateMutexW (the mutex handle is no longer zeroed); K4 winsize.ini width capped at 7680. Normal-path behavior unchanged |
| DLL-side hardening | R1~R4 | K5 MyBeginPaint accepts only a valid DC (the rare timing where BeginPaint returns NULL no longer treats an abnormal scene as normal painting) |
| build.cmd | VER=1.3.22 | VER=1.3.23 |

The auditor was Kimi (a fresh perspective, a read-only full read of the source): **zero new findings on the DLL side** (v1.3.22's cross-thread
hardening was independently judged mechanically closed); all 5 shell hidden risks were verified valid item by item, plus 1 more same-origin
initial malloc null check was fixed that the audit had not listed separately. The webp image-set counting discrepancy is archived as a known
trade-off (not a bug, unchanged).

Verification: EXE version resource 1.3.23.0; byte signature contains v1.3.23, zero v1.3.22 residue;
bundled DLL byte-comparison confirmed as the new v1.3.23; all self-checks passed (see Chapter 11 of the DLL master volume).

## Chapter 7: v1.3.22 (2026-10-29) — Deepseek Re-Review Hardening Release (Zero Interface Changes)

| Item | v1.3.21 | v1.3.22 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.21" | **"1.3.22"** |
| resource.rc version block | 1,3,21,0 | 1,3,22,0 |
| Bundled DLL | v1.3.21 (936,501 B) | **v1.3.22 (936,501 B, same name, different content)**: R1~R4 re-review hardening (see `../dll/changelog/v1.3.22.md`) |
| **Shell changes** | G1/G2/G3 three fixes | **Zero code changes** — only version increments (GUI_VERSION/DLL_VERSION/resource.rc/build.cmd/main.c header entry); all changes are on the DLL side |
| DLL-side hardening | — | R1 thread-ownership hardening for cross-thread destruction (governed by the registering owner's tid; normal-path behavior unchanged); R2 snapshot invariant comments solidified; R3 clamping magic number changed to (size_t)INT_MAX; R4 comments on the global shared semantics of timers |
| build.cmd | VER=1.3.21 | VER=1.3.22 |

Verification: EXE version resource 1.3.22.0; byte signature contains v1.3.22, zero v1.3.21 residue;
bundled DLL byte-comparison confirmed as the new v1.3.22; all self-checks passed (see Chapter 10 of the DLL master volume).

## Chapter 8: v1.3.21 (2026-10-28) — Deepseek Audit Fix Release (Zero Interface Layout Changes)

| Item | v1.3.20 | v1.3.21 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.20" | **"1.3.21"** |
| resource.rc version block | 1,3,20,0 (FILEVERSION was in fact 1,3,18,0, missed in v1.3.20) | 1,3,21,0 (incidentally fixing the missed FILEVERSION) |
| Bundled DLL | v1.3.20 (935,871 B) | **v1.3.21 (936,501 B)**: D1~D5 audit fixes (see `../dll/changelog/v1.3.21.md`) |
| **Shell fixes** | — | **G1**: the "Timed rotation" dropdown button face was always blank — after creating the button, `rot_ui_init` missed the backfill `g_cb_rot.btn = g_app.hwndRotBtn;`, so the entire echo block in `combo_state_set` was skipped by `if (cb->btn)` (compared with g_cb_clsid/g_cb_img, both of which backfill; only this one was missed); **G2**: added `on_rotmin_edit_change()` hooked to EN_CHANGE (clamps 0~1440 and writes back to v_rotmin live, empty string treated as 0, no write-back so no loop) + `on_rot_menu` now calls `update_summary()` after a preset is chosen (the summary panel has no randomMin row, so that half-sentence in the report was inaccurate; "feedback upon preset selection" is fulfilled via the timestamp refresh); **G3**: `save_config` writes the final clamped value back to the input box |
| Forward declarations | — | Added `update_summary` / `on_rotmin_edit_change` (C11: declare before use) |
| Interface layout | — | **Zero changes** (G1 only makes the button face show the text it should have shown all along) |
| build.cmd | VER=1.3.20 | VER=1.3.21 |

Verification: EXE version resource 1.3.21.0; byte signature contains v1.3.21, zero v1.3.20 residue;
bundled DLL byte-comparison confirmed as the new v1.3.21; test_render 116/116, test_filter 4/4 (all truly passing after adding the RGBA standard sample image),
test_random chi-square 37.2, test_race_loop 12,000 runs zero errors (see Chapter 9 of the DLL master volume).

## Chapter 9: v1.3.20 (2026-10-28) — Random Feature Redo (Second Interface Change Since v1.3.14 Was Finalized)

| Item | v1.3.19 | v1.3.20 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.19" | **"1.3.20"** |
| resource.rc version block | 1,3,19,0 | 1,3,20,0 |
| Bundled DLL | v1.3.19 (921,713 B) | **v1.3.20 (935,871 B)**: A true random source / B per-window bookkeeping / C timed rotation (see `../dll/changelog/v1.3.20.md`) |
| **Interface change** | Zero changes | **Added on the right of the random row: a "Timed rotation" owner-drawn dropdown + a "Custom…" integer-minutes input box**: new_btn_dropdown + TrackPopupMenu (the same painting approach as "UI scaling", with a check mark on the current preset); ES_NUMBER blocks decimals at the input source; the whole group grays out when random rotation is off; presets: No rotation/1/2/5/10/30 minutes/Custom |
| New controls | — | ID_CB_ROT (dropdown button), ID_EDIT_ROTMIN (input box), IDM_ROT_FIRST+0..6 (menu), ComboState g_cb_rot |
| Config | — | Added `randomMin` (integer minutes, 0 = no rotation, clamped to 1440); wired into load/save/backup report/Emergency off |
| build.cmd | VER=1.3.19 | VER=1.3.20 |

Verification: EXE version resource 1.3.20.0; byte signature contains v1.3.20, zero v1.3.19 residue;
bundled DLL and plugin copy SHA256 identical (`f4fbc1ad…`); all self-checks passed (see Chapter 8 of the DLL master volume).

## Chapter 10: v1.3.19 (2026-10-28) — Companion DLL Kimi Re-Review Fix Release (Zero Interface Changes)

**This release has zero shell interface changes**; the value lies in the companion DLL generation change:

| Item | v1.3.18 | v1.3.19 |
|---|---|---|
| GUI_VERSION / DLL_VERSION | "1.3.18" | **"1.3.19"** |
| resource.rc version block | 1,3,18,0 | 1,3,19,0 / "1.3.19 (companion DLL v1.3.19 audit-fix build)" |
| Bundled DLL | v1.3.18 (919,131 B) | **v1.3.19 (921,713 B)**: all 14 items of Kimi K3.1 Pro's re-review held; 13 fixed + 1 compromise (see `../dll/changelog/v1.3.19.md`) |
| main.c header version comment | v1.3.18 entry | **Added v1.3.19 entry** (zero-interface-change statement) |
| build.cmd | VER=1.3.18 | VER=1.3.19 |

Verification: EXE version resource FileVersion 1.3.19.0; byte signature contains v1.3.19, zero v1.3.18 residue;
bundled DLL and plugin copy SHA256 identical (`8c3ef576…`).

## Chapter 11: v1.3.18 (2026-10-28) — Removal of the "Elevated" Pill + Companion DLL Audit Re-Verification Release

### Positioning

**The first actual interface change since v1.3.14 was finalized**: at the user's request, the 4th status bar pill
"Elevated/Not elevated" was removed — it displayed the shell process's own administrator status, static and unrelated to the DLL registration state;
sitting right next to "DLL not registered", it was highly misleading (the user would think registration/elevation had gone wrong). All other interface elements unchanged.
Corresponds to the DLL-side v1.3.18 audit re-verification fixes (Qwen audit verified item by item; see `../dll/changelog/v1.3.18.md`).

### Change Details

| Item | v1.3.17 | v1.3.18 |
|---|---|---|
| GUI_VERSION | "1.3.17" | **"1.3.18"** |
| DLL_VERSION | "1.3.17" | **"1.3.18"** |
| Built-in resource ExplorerBgTool.dll | 919,310 B (audit-fix build) | **919,131 B (audit re-verification fix build)** |
| resource.rc version block | 1,3,17,0 / "1.3.17 (companion DLL v1.3.17 audit-fix build)" | 1,3,18,0 / "1.3.18 (companion DLL v1.3.18 audit-fix build)" |
| Status bar pills | 4: DLL registration / DLL file / image count / **Elevated** | **3: DLL registration / DLL file / image count (the "Elevated" pill removed; paint_status adjusted accordingly)** |
| main.c header version comment | v1.3.17 entry | **Added v1.3.18 entry** (explaining why the pill was removed) |
| main.c interface/layout logic | — | **One deletion** (4,799 → 4,800 lines: removed 6 lines of pill drawing, added explanatory comments, net +1 line) |
| build.cmd | VER=1.3.16 (historically not followed) | **VER=1.3.18 (incidentally fixing the historical omission)** |

### Verification

- windres + gcc `-O2 -mwindows -municode -std=c11 -Wall`: zero errors
- The bundled DLL, the plugin\ DLL, and the build\ DLL match SHA256 at all three locations (`9656e27e…`);
  EXE byte-signature check contains v1.3.18, zero v1.3.17 residue
- EXE version resource: FileVersion 1.3.18.0, ProductVersion "1.3.18 (companion DLL v1.3.18 audit-fix build)"
- Regression self-checks (companion DLL side): test_render / test_leak / test_race_loop all pass
  (test_leak rounds 2/3 at +0MB prove the hash-key cache hits across rebuilds; GDI handle sawtooth stable)

---

## Chapter 12: v1.3.17 (2026-10-28) — Companion DLL Audit Fix Release

### Positioning

Interface has **zero logic changes**: version numbers incremented (including adding the DLL_VERSION missed in v1.3.16), the "Scaling" preset semantics caption expanded,
and the built-in DLL replaced with the audit-fix build. Corresponds to 11 DLL-side fixes (9 audit issues + the TLS destructor crash landmine + GDI handle exhaustion);
see `../dll/changelog/v1.3.17.md`.

### Change Details

| Item | v1.3.16 | v1.3.17 |
|---|---|---|
| GUI_VERSION | "1.3.16" | **"1.3.17"** |
| DLL_VERSION | "1.3.15" (missed in v1.3.16, audit issue #6) | **"1.3.17"** (fixed) |
| Built-in resource ExplorerBgTool.dll | 912,114 B (aspect-ratio-scaling build) | **919,310 B (audit-fix build)** |
| resource.rc version block | 1,3,16,0 / "1.3.16 (companion DLL v1.3.16 aspect-ratio-scaling build)" | 1,3,17,0 / "1.3.17 (companion DLL v1.3.17 audit-fix build)" |
| main.c header version comment | v1.3.16 entry | **Added v1.3.17 entry** (listing the DLL-side fix highlights) |
| posType behavior caption | One line: "Always scales at the original image's aspect ratio without distortion, and follows the window live" | **Expanded**: clarifies that v1.3.15's "Scaling" was stretch-to-fill (distorting), that from v1.3.16 it is aspect-ratio full-fit (may leave margins on all sides), and that to cover fully use "Scale and Fill" — corresponds to DLL audit issue #5 |
| main.c interface/layout logic | — | **Zero changes** (4,789 → 4,799 lines; all +10 lines are comments and caption text) |
| build.cmd | VER=1.3.16 | VER=1.3.17 |

### Verification

- windres `--codepage=65001` + gcc `-O2 -mwindows -municode -std=c11 -Wall`: zero errors
- **Bundled DLL byte-by-byte verification**: the EXE resource section fully contains the 919,310 B audit-fix build (confirmed by byte search)
- Smoke test: running in an empty directory auto-unpacks `plugin\`; the unpacked DLL SHA256 = `d62a5ac6…` matches the delivered build byte for byte; startup verification passes
- Regression self-checks (`自检程序\` in the delivery package): geometry 227/227, image header parsing 54/54, corrupt-file rejection 4/4,
  Size garbage values 0/200, infinite redraw 1000→1, rendering alpha error 0.00, concurrency stress 12/12 zero crashes, bounded memory residue

---

## Chapter 13: v1.3.16 (2026-10-27) — Companion DLL Aspect-Ratio-Scaling Release

### Positioning

Interface has **zero logic changes**: version increments, a **new behavior caption line** added below the posType block (this is the first actual
content change to main.c since v1.3.15), and the built-in DLL replaced with the aspect-ratio-scaling build. Corresponds to the major new DLL-side feature
(aspect-ratio-preserving scaling + follows the window live; see Chapter 4 of `../dll/changelog/iteration-report-master.md`).

### Change Details

| Item | v1.3.15 | v1.3.16 |
|---|---|---|
| GUI_VERSION | "1.3.15" | **"1.3.16"** |
| DLL_VERSION | "1.3.15" | "1.3.15" (**missed** — the DLL version number shown by the GUI wasn't updated; fixed in v1.3.17) |
| Built-in resource ExplorerBgTool.dll | 891,738 B (fix build R5) | **912,114 B (aspect-ratio-scaling build)** |
| resource.rc version block | 1,3,15,0 / "1.3.15 (companion DLL v1.3.15 fix build)" | 1,3,16,0 / "1.3.16 (companion DLL v1.3.16 aspect-ratio-scaling build)" |
| posType behavior caption (new) | None | **One new line**: "The background image is always scaled at the original image's aspect ratio, without distortion, and follows the window live; 'Scaling' = the whole image fully fitted (may leave margins on all sides); 'Scale and Fill' = aspect-ratio cover with the overflow cropped" |
| main.c interface/layout logic | — | **Zero changes** (4,776 → 4,789 lines; all +13 lines are caption text) |
| build.cmd | VER=1.3.15 | VER=1.3.16 |

### Verification

- windres + gcc zero errors; **bundled DLL byte-by-byte verification**: the EXE fully contains the 912,114 B aspect-ratio-scaling build
- Smoke test: running in an empty directory auto-unpacks; the unpacked DLL SHA256 = `a2b9a8a7…` matches the aspect-ratio-scaling build byte for byte; startup verification passes
- Rendering captures in the delivery package `验证输出\` ("Scaling" mode follow-the-window shots at four window sizes)

### Leftovers (fixed in v1.3.17)

- DLL_VERSION missed (audit issue #6)
- The "Scaling" preset semantics changed relative to v1.3.15 (stretch-to-fill → aspect-ratio fit) but the caption didn't say so clearly (audit issue #5)

---

## Chapter 14: v1.3.15 (2026-10-23) — Companion DLL Generation-Change Release

### Positioning

Interface has **zero logic changes**: only version increments, version-resource sync, replacing the built-in DLL from the author's original with fix build R5,
and adding the code_page declaration.

### Change Details

| Item | v1.3.14 | v1.3.15 |
|---|---|---|
| GUI_VERSION | "1.3.14" | "1.3.15" |
| DLL_VERSION | "1.1.0.1" | "1.3.15" |
| Built-in resource ExplorerBgTool.dll | 221,696 B (author's original) | **891,738 B (fix build R5)** |
| resource.rc version block | 1,3,14,0 / "1.3.2 (companion DLL v1.1.0.1)" | 1,3,15,0 / "1.3.15 (companion DLL v1.3.15 fix build)" |
| #pragma code_page(65001) | None | **Added** (the Chinese in the version info is no longer misread via the system ACP) |
| main.c interface/layout logic | — | **Zero changes** (4,776 lines) |
| build.cmd | VER=1.3.14 | VER=1.3.15 |

### Verification

- windres + gcc zero warnings; bundled DLL byte-by-byte verification (891,738 B fully contained)
- Smoke test: unpacked DLL SHA256 = `fe8ae612…` matches the fix build; startup verification passes
- **Real machine (2026-10-23)**: the panel starts normally; backgrounds in all windows correct ✅; rotation/adding-removing images/newly opened windows/random rotation all correct ✅

### Pitfall Archive (hit for the third time; formally recorded as an iron rule)

Invoking gcc directly from Git Bash with `-o` plus a Chinese filename → the terminal corrupts the filename encoding, producing an exe with a mojibake name.
**Iron rule: MinGW command-line output filenames always use an ASCII temporary name + Python os.replace to rename to the Chinese name.**

---

## Chapter 15: Shell ↔ DLL Pairing Master Table

| Shell version | Date | Companion DLL | DLL SHA256 (first 32 chars) | Status |
|---|---|---|---|---|
| v1.3.14 (old package, kept on Desktop) | 2026-10-02 | author's original 221,696 B | — | Old line |
| v1.3.15 | 2026-10-23 | Fix build R5 891,738 B | `fe8ae612c1519dcb6e37c56355f33218` | Kept intact on the Desktop as `backimg_GUI_v1.3.15\` |
| v1.3.16 | 2026-10-27 | Aspect-ratio-scaling build 912,114 B | `a2b9a8a7db34c2dba743f0768042ce28` | Kept intact on the Desktop as `backimg_GUI_v1.3.16\` |
| v1.3.17 | 2026-10-28 | Audit-fix build 919,310 B | `d62a5ac6467eea601e448076466fd634` | Kept intact on the Desktop as `backimg_GUI_v1.3.17\` |
| v1.3.18 | 2026-10-28 | Audit re-verification fix build 919,131 B | `9656e27e3bad4358fbffaa55bfc0f6f8` | Kept intact on the Desktop as `backimg_GUI_v1.3.18\` |
| v1.3.19 | 2026-10-28 | Kimi re-review fix build 921,713 B | `8c3ef57614a11ba3767905ece70d8158` | Kept intact on the Desktop as `backimg_GUI_v1.3.19\` |
| v1.3.20 | 2026-10-28 | Random-feature redo build 935,871 B | `f4fbc1ad7197c24b4b76fcd92ccd68b7` | Kept intact on the Desktop as `backimg_GUI_v1.3.20\` |
| v1.3.21 | 2026-10-28 | Deepseek audit-fix build 936,501 B | `260828d95bf40795d5f73877c292dd54` | Kept intact on the Desktop as `backimg_GUI_v1.3.21\` |
| v1.3.22 | 2026-10-29 | Deepseek re-review hardening build 936,501 B | `aff84abc49dea9214ff07f3bc13004fd` | Kept intact on the Desktop as `backimg_GUI_v1.3.22\` |
| v1.3.23 | 2026-10-29 | Kimi independent re-review hardening build 936,501 B | `8b111243652dfc18fa5261f416fbf0f2` | Kept intact on the Desktop as `backimg_GUI_v1.3.23\` |
| v1.3.24 | 2026-10-29 | Hunyuan 4 re-review closing build 936,501 B | `481ee8b899d4d68bebfae58e16731a2f` | Kept intact on the Desktop as `backimg_GUI_v1.3.24\` |
| v1.3.25 | 2026-10-29 | Second-level rotation + directory self-healing build 942,255 B | `b6a4966a772724977cfa4763e7182115` | Kept intact on the Desktop as `backimg_GUI_v1.3.25\` |
| v1.3.26 | 2026-10-29 | Version-sync build 942,255 B | `1d4d7e882e350fd8775739fda82658a2` | Kept intact on the Desktop as `backimg_GUI_v1.3.26\` |
| v1.3.27 | 2026-10-29 | Version-sync build (three review fixes) 942,255 B | `85e19df9fd4f36fb9e28eab7a83b4415` | Kept intact on the Desktop as `backimg_GUI_v1.3.27\` |
| **v1.3.28 (this package, current)** | 2026-10-29 | **Version-sync build (three re-review fixes) 942,255 B** | `7846debd35f5f259a75bc45a047b13db8` | **Current** (Desktop `backimg_GUI_v1.3.28\`) |

The panel's "bundled vs plugin-directory DLL mismatch" check **passes by construction** in this package — the bundled resource and the file in plugin\ are the very same file.
If upgrading from an older version: **both** the exe and `plugin\ExplorerBgTool.dll` must be replaced; replacing only one triggers the verification prompt.

---

## Chapter 16: Inherited History (Python v1.0.0 → C v1.3.14)

Below is the v1.3.14 master version iteration report **archived verbatim in full**, covering all 31 versions of the shell from its Python/tkinter predecessor through C v1.3.14:

---

# backimg Panel · Master Version Iteration Report (Python v1.0.0 → v1.2.9 · C v1.3.1 → v1.3.14)

- **Project goal**: rewrite the original Python/tkinter backimg Panel (v1.2.9) as a **pure C + Win32 API single-file EXE**, a one-to-one reproduction of the HTML design mockup ("backimg_图形板_现代化界面预览.html"), with behavior identical to the original.
- **Report date**: 2026-10-02
- **Version range**:
  - **Python/tkinter version (predecessor)**: v1.0.0 → v1.2.9, **17 versions** total (completed within the single day 2026-08-04); records in `backimg_GUI_v1.2.9\backimg_GUI_v1.2.9迭代说明.txt`
  - **C/Win32 version (rewrite)**: v1.3.1 (first released 2026-10-01) ~ v1.3.14 (latest delivery 2026-10-02), **14 versions** total
- **Current latest**: `C:\Users\Administrator\Desktop\v1.3.14\backimg图形板_v1.3.14.exe` (521KB single file, depending only on system DLLs)
- **Development iron rules** (observed throughout): no GUI automation clicking, no mouse simulation, no automatic triggering of registration/deregistration/File Explorer restart.

---

## 1. Version Overview Table

### Python/tkinter version (predecessor, v1.0.0 → v1.2.9, 17 versions, completed on 2026-08-04)

| Version | Type | Theme |
|------|------|------|
| v1.0.0 | Initial feature release | First graphical configuration panel: auto-locates the plugin directory, detects DLL/BHO registration and administrator status, graphical editing of config.ini (comments preserved), one-click register/deregister/restart File Explorer, image preview, folder-specific background blocks |
| v1.0.1 | Bug fix | Fixed the post-packaging missing-tkinter error: injected the official Tcl/Tk runtime into the build environment and repackaged |
| v1.0.2 | Bug fix | Fixed two crashes at startup (ttk.Label not supporting -height/-relief; missing mainloop) |
| v1.1.0 | Feature | Resource self-containment: DLL + ini embedded in the exe; first run unpacks to plugin/, filling only what's missing without overwriting user config |
| v1.1.1 | Performance | Elevation switched to a ctypes direct call of ShellExecuteExW; register/deregister/restart buttons went from 3~5-second freezes to <1 second (background thread + re-entry guard + watchdog) |
| v1.1.2 | Security hardening | Register/deregister now call regsvr32 directly, eliminating the cmd injection risk; DLL SHA256 verification, reverse verification of registry paths, backup/restore of config, log persistence |
| v1.1.3 | UI fix | Status bar wraps overly long text automatically (dynamic wraplength computed via a bound Configure) |
| v1.1.4 | Review adoption | DLL integrity also verified at startup, forming double protection with the registration-time check |
| v1.1.5 | Rename + icon | Renamed to "backimg图形板"; added title bar/taskbar/exe icons |
| v1.1.6~1.1.8 | Icon polishing | Three iterations of white-background removal on the AI-generated image (accidentally deleting the little sun → crop-and-enlarge rejected → finished with algorithm v4) |
| v1.1.9 | Final icon | Abandoned the AI-generated image; switched to the user's own screenshot as the icon (white-background removal + squaring + multi-frame ICO); icon copyright belongs to the user |
| v1.2.0 | Review adoption | 7 robustness optimizations: single-instance lock, confirmation before taskkill, permission-aware registry detection, path special-character interception, automatic log rotation, etc. |
| v1.2.1 | UI feature | Added a "Currently effective config (live)" summary panel at the bottom (9 summary rows + 3 buttons, auto-refreshed via variable trace) |
| v1.2.2 | Bug fix | Default window 720×700→880×900; window size remembered in winsize.ini on close; fixed a startup crash caused by an indentation trap |
| v1.2.3 | Review adoption | Fixed large-image preview lag and a PIL Image leak (of 6 review items only 1 was a real issue; the rest were false positives, not adopted) |
| v1.2.4 | Feature borrowing | Landed 5 items borrowed from Maplespe's official source: Emergency off/restore, registry detection extended to 4 locations, warning when folderExt is enabled, built-in CLSID dropdown, copy summary with SHA256 |
| v1.2.5~1.2.7 | Three consecutive UI-scaling fixes | tk scaling ineffective (abandoned) → font-only scaling without window scaling → final proportional scaling of the whole UI (fonts + geometry + padding + preview image) |
| v1.2.8 | UI layout optimization | Compressed redundant columns/gaps (slider row merged into "70", browse button merged into the folder row); summary long paths auto-wrap |
| v1.2.9 | Review adoption | Fixed silent fallback on winsize.ini corruption (now logs a warning) + fault tolerance in resource-directory probing (of 6 review items, 2 real issues adopted) |

After the Python version completed its historical mission it was replaced wholesale by the C rewrite (starting at v1.3.0); its functional spec and behavior served as the one-to-one reproduction baseline for the C version.

### C/Win32 version (rewrite, v1.3.1 → v1.3.14, 14 versions)

| Version | Date | Theme | EXE size | main.c lines |
|------|------|------|---------|------------|
| v1.3.1 | 10-01 | Code-level defect fixes (memory safety/logic/performance) | 490KB | 3162 |
| v1.3.2 | 10-02 | UI polish round 1: item-by-item restoration against the design mockup | 496KB | 3669 |
| v1.3.3 | 10-02 | UI polish round 2: light owner-draw for input boxes/slider | 504KB | 4046 |
| v1.3.4 | 10-02 | UI polish round 3: Doubao review 6 items + a real bug | 506KB | 4123 |
| v1.3.5 | 10-02 | UI polish round 4: 7 user on-machine annotations | 508KB | 4215 |
| v1.3.6 | 10-02 | On-machine annotations 4 spots: bottom bar right-alignment/tab redo | 506KB | 4205 |
| v1.3.7 | 10-02 | On-machine annotations 3 spots: the true root cause in WM_CTLCOLORBTN | 506KB | 4240 |
| v1.3.8 | 10-02 | On-machine annotations 3 spots: right-card rearrangement/flexible toggle row spacing | 507KB | 4266 |
| v1.3.9 | 10-02 | On-machine annotations 3 spots: layout planning/owner-drawn dual dropdowns | 509KB | 4370 |
| v1.3.10 | 10-02 | GDI+ global anti-aliasing + segmented whole-button coloring | 513KB | 4436 |
| v1.3.11 | 10-02 | Root-cause fix for the stray "GDI+ Decoded" text on the preview card | 515KB | 4483 |
| v1.3.12 | 10-02 | Doubao audit 7 items: security/robustness fixes such as DLL integrity verification | 518KB | 4678 |
| v1.3.13 | 10-02 | Root-cause fix for slider/summary ghosting + 3 verification observation points | 519KB | 4729 |
| v1.3.14 | 10-02 | [Major] Root-cause fix of 4 bugs in the ini write chain (save crash/ini corruption) | 521KB | 4773 |

Every version passed the same verification pipeline: `-Wall` zero-warning compile → 5 screenshot comparisons (tab0/tab1/tab2/1044 real-machine width/900 narrow window) → empty-directory unpack SHA256 match (DLL 221696B / config.ini 1988B) → smoke test of the Desktop delivery build.

---

## 2. Change Summaries per Version

### v1.3.1 — Code-level defect fixes (baseline hardening)
Baseline v1.3.0 (identical features and UI; this release changes no appearance). Concentrated fix of 30+ code defects:
- **Memory safety (7 high-severity items)**: `read_text_file`'s BOM path not appending NUL caused out-of-bounds reads across the whole INI chain; `path_join` truncated without terminating; about 10 wide/multi-byte conversions without terminators; silent truncation of long INI values corrupting the config, etc. — new safe helpers `sfmt`/`wtou8`/`u8tow` replaced them across the board.
- **Logic defects (13 items)**: elevation registration falsely judged successful, thread handle leak, GDI+ with no failure path, preview HBITMAP lifetime, verification timer spinning idle, window position restored off-screen, etc.
- **Performance**: slider/toggle high-frequency triggering of full directory scans → image-count cache; deleted 128KB of dead code.
- **UI consistency + INI compatibility**: all controls unified to the YaHei font; INI tolerant parsing of `#`/`;` comments, `yes/on`, `key = value`.

### v1.3.2 — UI polish round 1 (100% restoration against the design mockup)
Per the Doubao review checklist: key chip class `backimgChip`, summary key ellipsis `backimgSumKey`, readout chip, right-aligned hints, posType segmented container, light-gray log header, dashed divider between toggles, summary two-column grid, image page 16:9 preview + pmeta, layered area base colors (toolbar/status bar/bottom bar white, panels gray), solid round dot on status pills, fonts rebuilt on scaling.
**Key architectural fix**: the window shrank step by step on scaling switches (nominal baseline nominal_w/h separated from layout dimensions).
**New pitfall**: the mini button style bit 0x10 conflicted with the segmented `(idx<<4)` → odd segments were wrongly painted with a gray background; changed to 0x100 (the owner-draw userdata bitmap must be planned as a whole).

### v1.3.3 — UI polish round 2 (light owner-draw for inputs/slider)
- Three EDIT controls stripped of the native WS_BORDER; new `backimgEditFrame`: corner radius 7, blue focus frame, idle `#F7F8FA`/focused white background.
- Native TRACKBARs fully removed; new pure owner-drawn `backimgSlider` (6px rounded track + 14px round thumb, full keyboard support).
- Tab hover; buttons unified to 32px height; summary key column 128→108px.
- **Narrow-window clipping (layout-level bug)**: minimum resizable size raised to 880×970; the effective-flow block is skipped entirely when space is insufficient (graceful degradation).

### v1.3.4 — UI polish round 3 (6 review items + a real bug)
- posType second row's 3 segments horizontally centered; flow block elastically fills the card bottom; summary buttons fixed at two rows (3+2).
- UI scaling: native COMBOBOX → owner-drawn dropdown button (`BTN_STYLE_DROPDOWN 0x400`) + TrackPopupMenu with 8 presets.
- **Real bug**: input boxes always showed the focused white background in idle state — `EditFrameProc`'s `WM_CTLCOLOREDIT` used `userdata==lParam` to test focus, which was always true; changed to a `GetFocus()` check.

### v1.3.5 — UI polish round 4 (7 user on-machine annotations)
Input text hugging the top (single-line EDIT top-aligned → vertically centered by font line height; three places benefit); left card elastic fill (35/35/30 split); tab semibold + 1px separator under rows; **the toolbar input box truly overlapped the browse button by 8px** (w-S(236) overran → changed to S(252) with an 8px gap); summary buttons back to one row (auto fallback to 3+2 in narrow windows); flow allocation changed to pure proportion with no cap, eliminating the concentrated hole; toggle round thumb given a shadow + 1px outline (38×22, aligned with the design mockup).

### v1.3.6 — On-machine annotations 4 spots (tab architecture redo)
- The bottom bar's 7 buttons split into two groups: first 4 left-aligned, last 3 (refresh status/backup/restore) right-aligned to the window's far right (adding the previously overlooked `.actions spacer`).
- Segmented buttons given borders in both states (the user wanted "button boundary definition").
- Flow tip bar fixed at 26px (in v1.3.5 it was stretched to 40px+ into a big gray bar).
- **Tabs abandon SysTabControl32**: the system 3D border couldn't be removed under owner-draw → switched to 3 owner-drawn buttons (`BTN_STYLE_TAB 0x800`, bits 4-7 store the page number), deleting the entire TabHotProc/TCN_SELCHANGE chain.

### v1.3.7 — On-machine annotations 3 spots (unearthing the true root cause of background erasure)
- **All rounded buttons showed system gray right-angle blocks at their corners**: the main window lacked a `WM_CTLCOLORBTN` branch, so owner-draw buttons first had their whole rectangle erased by the system with the COLOR_BTNFACE gray brush → return a brush matching the parent container's background color; all buttons cured at the root.
- Flow block pinned to the card bottom (after capping the three spacings, the remainder is recorded in `add_s` and stuffed into the block's top margin).
- Segments lose the white background (unselected segments show gray through, borders kept).

### v1.3.8 — On-machine annotations 3 spots (right-card rearrangement)
- Summary button row moved down: first compute the flow block geometry anchored to the card bottom (flow_top), then pin the button row S(8) directly above the flow block.
- Segmented container's gray background changed to white (the user asked for background removal in round two; on-machine feedback overrides the design mockup's #EDF1F7).
- Toggle row spacing made elastic: 70% of the left card's leftover space injected into toggle row spacing; row content vertically centered.

### v1.3.9 — On-machine annotations 3 spots (layout planning + dual owner-drawn dropdowns)
- **Real bug (left over from v1.3.8)**: the flow block height was back-derived from the full extra while the block consumed only the capped portion → the excess sank to the bottom as about 90px of blank space. Fix: plan uniformly before laying out the right card (40% injected into summary grid row spacing, the rest to the flow block's three spots); block height computed from the **actually consumed amount**.
- The "last refresh" time was clipped at the tail: label margin S(10)→S(24).
- The folder-specific background page's two native COMBOBOXes → the owner-drawn dropdown trio (`ComboState` state table + 2x realloc growth + `IDM_CB_FIRST=600`), incidentally fixing the dropdown text alignment (center mistakenly passed → left).

### v1.3.10 — GDI+ global anti-aliasing + segmented whole-button coloring
- **Root-cause-level fix**: all rounded corners went through GDI `RoundRect`/`Ellipse` with no anti-aliasing → `draw_rounded_rect` rewritten as GDI+ paths (4×Arc + SmoothingModeAntiAlias); added `fill_ellipse`/`stroke_ellipse` replacing 6 GDI circles; the whole program's rounded corners upgraded in one stroke; falls back to GDI when GDI+ is unavailable or the radius < 2.
- posType selected segment changed to **blue background, white text** whole-button coloring (the mockup had white background, blue text; overridden per user on-machine feedback).
- **Two pitfalls**: MinGW lacked `GdipFillPathI` and other `*I` declarations, so the REAL versions were used; `argb_of()` had the byte order reversed (COLORREF and GDI+ ARGB have opposite R/B), turning the whole UI orange — caught immediately by the screenshot smoke pipeline.

### v1.3.11 — Root-cause fix for the stray "GDI+ Decoded" text on the preview card
- Root cause: the preview stage's meta-info row trio (filename/size/decode-method chips) had been created together since v1.3.2; in the empty state only the "GDI+ Decoded" chip remained hanging forever.
- Fix: trio initially hidden + new `preview_meta_update(name, dim, show)` for unified management — after an image is successfully selected, displayed with flex layout by actual text width; the whole row hides when the list is refreshed or the image is lost.
- Pitfall: the `S()` macro is `#undef`'d at line 2707; helpers after that must call `pt_scaled()` directly; forward declarations must precede all callers.

### v1.3.12 — Doubao audit, 7 items (fixed after item-by-item verification)
Per Doubao's "v1.3.11 Code Audit Report", all 7 allegations were checked against the source and **all held**:
- **P1 Missing DLL integrity verification**: added `sha256_mem()` (Crypt API in-memory chunked hashing); at startup the SHA256 of the EXE's bundled resource DLL is compared with the plugin directory's DLL, with a warning on mismatch; before clicking "Register (elevated)" it is compared once more — on mismatch a confirmation box pops up, and choosing No aborts (preventing a swapped-out DLL from registering as usual).
- **P2-1 Custom directory "no reaction after picking"**: after picking a directory, if custom isn't checked, an "Enable it now?" prompt appears; choosing Yes checks it and switches the list (preserving the strict logic of "list = actually effective directory").
- **P2-2 Multi-line log squeezed into one entry**: `log_view_add()` splits on `\n` and adds each line to the list (a LISTBOX is one item per line and doesn't understand `\r\n`).
- **P3 ×4**: ① manually edited paths trigger EN_CHANGE to immediately invalidate the image-count cache + refresh the list; ② previewing images >30MB first clears the old image + logs a hint + shows a wait cursor; ③ `write_text_file`/`ini_commit` return bool, so a failed save pops an error box instead of falsely reporting success (incidentally adding a write-side malloc check), with folder-specific rule writing/deletion aligned in sync; ④ the copy summary gains three diagnostic lines: DLL SHA256 / DLL size / plugin CLSID.
- Pitfalls: invoking gcc directly from Git Bash turned Chinese filenames into mojibake (switched to build.cmd/cmd.exe); `sed -i` converted the CRLF batch file to LF, garbling cmd parsing (converted back to CRLF with python).

### v1.3.13 — Ghosting root-cause fix + 3 verification observation points
Based on user on-machine annotations (red box = slider ghosting, blue box = summary garbled text) + 3 observation points from Doubao's "v1.3.12 Verification Report":
- **Slider ghosting (root-cause level)**: `backimgSlider`'s `WM_ERASEBKGND` returns 1 without erasing the background, while `WM_PAINT` draws only the 6px track — the parts of the round thumb and its shadow protruding above and below the track were **never repainted by anyone**, so on fast drags the anti-aliased residue rings at old positions accumulated frame by frame into ghosts (the defect had lurked since the slider's birth in v1.3.3, becoming more visible after AA in v1.3.10). Fix: each frame first fills the whole block with the card background + double buffering (eliminating flicker). **Proven by a programmatic stress test**: after 120 full-stroke round trips (`TBM_SETPOS` messages, not mouse simulation), v1.3.12 showed 119 stray colors/two residue rings; v1.3.13 showed zero residue.
- **Summary overlapping text (same class of root cause)**: `SumValProc` drew text in TRANSPARENT mode and never laid a background, so at high-frequency text changes new characters stacked on old ones. Fix: the four same-class hazards SumVal/SumKey/Chip/StepNo now uniformly lay the parent container's background color before drawing.
- **3 verification observation points (verified valid)**: ① conditional refresh on EN_CHANGE (when custom isn't checked, don't rescan the directory on every keystroke); ② same for `on_browse_folder`; ③ folder-specific rules allow empty-image rules (aligned with Python `_adv_add`; verified from the Python source: it only blocks empty paths) — the "please select an image filename" block was removed, keeping the existence double-check when img is non-empty.

### v1.3.14 — [Major] Root-cause fix of four bugs in the ini write chain (nailed with gdb)
User testing: pick a custom directory → click Save Config → freeze and exit; config.ini written corrupted (section names lose `]`, key values lost, Chinese comments turn to mojibake, content duplicated twice); the DLL reports "missing image directory"; posType/imgAlpha don't take effect. **All same-origin** — `ini_write_value()`'s text-rebuild chain had never worked correctly since v1.3.0:
- **Bug A**: after truncating the section header in place to extract the section name, the truncated line was written to the output → all `[section]` headers lost `]`, and all subsequent section recognition failed;
- **Bug B**: key lines were truncated with `*eq=0` for comparison and never restored → all keys before the target key in the same section lost `=value`;
- **Bug C**: the concatenation buffer `malloc(32)` bytes was paired with `out_cap=32` characters, so any ini over 16 characters **wrote past the end of the heap** → smashing heap metadata crashed the save; hitting an adjacent block concatenated other buffers' contents (log bytes `04:25:49.323][pid=`) into the wide string written into the ini (the source of the mojibake); whether it exposed was pure heap-layout luck;
- **Bug D**: appended line endings mixed LF/CRLF.
- A+B stacked → section recognition failed → every save **appended** the whole 7-key group to the end of the file ("content duplicated twice"). Also fixed `mk_kv_line`'s `%S` converting via ACP, which garbled Chinese paths (changed to explicit u8tow); the embedded default config.ini was remade (folder defaults to empty, historical `\r\r` clusters cleared).
- **Verification innovation**: gdb + a function-level harness (no UI, running only the ini chain) first reproduced the crash (`Heap block ... modified ... past requested size of 20`) and then proved the fix — no crash, in-place replacement, correct Chinese UTF-8.

---

## 3. Major Bugs and Root-Cause Compendium (by root-cause layer)

A notable characteristic of this project is that many bugs presented as "interface appearance issues" while the actual root causes lay in the message mechanism/layout algorithms/drawing primitives:

| Layer | Bug | True root cause | Fixed in |
|------|-----|--------|---------|
| Message mechanism | Rounded buttons showed system gray right-angle corner blocks | Missing `WM_CTLCOLORBTN`; DefWindowProc returned COLOR_BTNFACE and erased the background | v1.3.7 |
| Message mechanism | Input boxes always white in idle state | In `WM_CTLCOLOREDIT`, the `userdata==lParam` focus test was always true | v1.3.4 |
| Message mechanism | Tabs' 3D border couldn't be removed | SysTabControl32's border is drawn by the system, out of owner-draw's reach | v1.3.6 (abandoned in favor of owner-draw) |
| Drawing primitives | Jagged rounded corners across the UI | GDI RoundRect/Ellipse has no anti-aliased rasterization | v1.3.10 (globally replaced with GDI+ paths) |
| Drawing primitives | Whole UI turned orange | `argb_of` reversed the R/B byte order of COLORREF→ARGB | v1.3.10 |
| Layout algorithms | Window shrank step by step on scaling switches | Nominal baseline not separated from the actual client area; WM_SIZE wrote back the baseline | v1.3.2 |
| Layout algorithms | Flow block sank leaving about 90px blank | Block height back-derived from the full margin while the block consumed only the capped portion | v1.3.9 |
| Layout algorithms | Input box truly overlapped the browse button by 8px | `w-S(236)`'s right edge passed the button's start | v1.3.5 |
| Bit flags | Odd segments wrongly painted as mini buttons | Style bit 0x10 conflicted with `(idx<<4)` | v1.3.2 |
| Memory safety | Out-of-bounds reads in the INI chain/config corruption | MultiByteToWideChar with explicit length didn't append NUL; long values truncated | v1.3.1 |
| Display details | Single-line EDIT text hugging the top | Single-line EDITs default to top alignment; the outer frame stretched to full height | v1.3.5 |
| Display details | Empty-state preview card hung with "GDI+ Decoded" | The pmeta trio was created but its visibility never managed | v1.3.11 |
| Drawing primitives | Slider drag ghosting/residue rings | Owner-drawn control returned 1 from WM_ERASEBKGND but WM_PAINT didn't lay a background; the thumb's protruding area was never repainted; AA residue accumulated frame by frame | v1.3.13 |
| Drawing primitives | Summary values overlapped into garble while dragging the slider | SumVal drew TRANSPARENT text without ever laying a background; new characters stacked on old ones | v1.3.13 |
| Security | A swapped-out DLL could register as usual | Startup verification only hashed, never compared; registration had no consistency confirmation | v1.3.12 |
| Display details | Multi-line log squeezed into one entry with odd characters | A LISTBOX is one item per line and doesn't understand `\r\n` control characters | v1.3.12 |
| Robustness | Failed saves falsely reported success | write_text_file silently returned (void) when CreateFile failed | v1.3.12 |
| Memory safety | Save config froze and exited + ini fully corrupted | ini_write_value's concatenation buffer confused malloc bytes/WCHAR units, writing past the heap (lurking since v1.3.0) | v1.3.14 |
| Encoding | Chinese paths written to the ini as mojibake | %S converted char* via ACP instead of UTF-8 | v1.3.14 |
| Text rebuild | ini section names losing ]/key values lost/whole blocks duplicated | After truncating to extract the section name it still appended the original truncated line; *eq=0 never restored; section recognition failing caused the whole group to be appended on every save | v1.3.14 |

**Methodology lessons**: ① don't patch control by control; prefer fixing the low-level drawing primitives (one fix in v1.3.10 benefited the whole program); ② when the user says "the color is wrong", locate it with per-pixel profiling rather than guessing by eye; ③ the screenshot smoke pipeline can catch byte-order-level errors immediately; ④ **iron rule for owner-drawn controls — if WM_ERASEBKGND returns 1, WM_PAINT must lay a background across the entire client area** (v1.3.13 cured the two appearance-bug classes of ghosting/overlapping text in one stroke and cleared all four same-pattern hazards at once); ⑤ fix effects must be provable — v1.3.13 obtained before/after comparative evidence via a programmatic stress test (120 TBM_SETPOS round trips + pixel color counting) rather than eyeballing "looks like it's gone".

---

## 4. Technical Architecture Evolution

### 1. Evolution line: native controls → full owner-draw
| Native control | Replacement | Version |
|---------|---------|------|
| SysTabControl32 | 3 owner-draw buttons (BTN_STYLE_TAB) | v1.3.6 |
| TRACKBAR | `backimgSlider` pure owner-draw (with keyboard support) | v1.3.3 |
| COMBOBOX ×3 (scaling/CLSID/image name) | Owner-drawn dropdown button + TrackPopupMenu + ComboState state table | v1.3.4 / v1.3.9 |
| Native EDIT border | `backimgEditFrame` container + borderless embedded EDIT | v1.3.3 |

### 2. Owner-drawn control family (final form)
`backimgCardPanel` (card/whiteboard/line-base/segment-base multi-mode), `backimgChip` (key/readout), `backimgSumKey`, `backimgDivider` (dashed), `backimgStatusBar` (status pills), `backimgStepNo`, `backimgFlowTip`, `backimgEditFrame`, `backimgSlider` + BTN_STYLE bitmap-driven owner-draw buttons (0x100 mini / 0x200 icon_ok / 0x400 dropdown / 0x800 TAB).

### 3. Drawing primitives (after v1.3.10)
`draw_rounded_rect` (GDI+ path AA, fill/stroke in one) → `fill_rounded_rect`/`fill_ellipse`/`stroke_ellipse` all AA-ified; `argb_of()` handles COLORREF↔ARGB byte-order conversion; automatic fallback to GDI on failure.

### 4. Layout system
- Left card: elastic distribution (70% to toggle row spacing + 30% to the three block gaps); compresses in reverse when space is insufficient.
- Right card: **card-bottom anchoring + unified planning** — first compute the bottom-pinned flow geometry, then back-derive and pin secondary elements; block height counted by actual consumption.
- Global: the S() scaling macro converts with the UI scaling factor; graceful degradation in narrow windows (flow block skipped, summary buttons fall back to 3+2, summary single column).

### 5. Scale changes
main.c 3162 → 4729 lines (+1567 lines, a large share of it owner-draw and layout code); EXE 490 → 519KB; every version's delivery package fixedly contains: source + resource.rc/res + build.cmd + EXE + version iteration report + delivery notes + 5 verification screenshots.

### 6. Security and robustness completion (from v1.3.12)
Aligned with the Python version's existing security capabilities: DLL SHA256 double verification (startup + before registration), explicit errors on failed saves, copy summary with the diagnostic trio (SHA256/size/CLSID), prompt before decoding large images.

---

## 5. Verification System (applied consistently across all 14 versions)

1. **Compilation**: MinGW-w64 GCC 16.2.0 (UCRT), `-O2 -mwindows -municode -std=c11 -Wall`; all 14 versions zero warnings; windres with explicit `-O coff` (a pitfall where the new version's raw resource format isn't recognized by `ld`).
2. **Screenshot pipeline**: Python win32gui + PIL (PrintWindow/BitBlt client-area cropping), fixed 5 sets — tab0/tab1/tab2 + 1044 real-machine width + 900 narrow window; targeted zoomed checks (button corners, toggle arcs, segment selected state).
3. **Ghosting stress test** (from v1.3.13): after programmatically firing 120 `TBM_SETPOS` full-stroke round trips + `WM_HSCROLL` at the slider (the same repaint path as `load_config`, not mouse simulation), take screenshots and count the pixel colors in the region — v1.3.12 (pre-fix) 119 colors/residue rings; v1.3.13 78 colors, all expected.
4. **Embedded-resource verification**: running in an empty directory auto-unpacks; plugin/ExplorerBgTool.dll (221696B) + config.ini (1988B) match the source files' SHA256 byte for byte.
5. **Smoke test**: startup verification log (DLL SHA256 comparison, read-only registry detection) and clean exit.
6. **Iron rules**: no mouse/keyboard simulation at any point; registration/deregistration/File Explorer restart never triggered; popup menus and elevation interactions left for user testing.

---

## 6. Current Status and Known Limitations

**Current status**: v1.3.13 delivered to `C:\Users\Administrator\Desktop\v1.3.13\`; appearance highly consistent with the design mockup (a few items intentionally deviate per user on-machine feedback), behavior aligned with the original v1.2.9, all known defects closed; all worthwhile items from Doubao's two audit/verification rounds digested (v1.3.12's seven fixes + v1.3.13's three observation points).

**Remaining limitations** (all intentional decisions or low-impact items):
- Popup menus (scaling/CLSID/image name) use the system native menu style; the design mockup defines no menu skin.
- Summary key names show ellipsis within the key column — consistent with the mockup; the folder's long path value truncates in the narrow column (the full path is visible in the edit box and status bar).
- When the window is stretched very tall, the left card's spacing enlarges (expected behavior of elastic filling).
- The summary buttons' single-row layout automatically falls back to two rows below a 940px window width; by design.
- The flow tip bar's dashed outline keeps GDI PS_DOT (the dotted dash's jaggies are imperceptible).
- Elevation interactions such as register/deregister/restart File Explorer require user testing (iron rules forbid automatic triggering).
- Large-image decoding is still synchronous on the main thread (v1.3.12 already has the >30MB prompt + wait cursor); background-thread decoding is deferred until needed (the version number will jump to v1.4.0 then).
- The slider's double buffering creates a ~320×30 memory bitmap per WM_PAINT; testing shows no perceptible latency.

---

*The Python-version portion of this report is summarized from "backimg_GUI_v1.2.9迭代说明.txt"; the C-version portion is summarized from each version's "版本迭代报告.md" (v1.3.1~v1.3.13, located in each version's delivery directory); details defer to the original reports.*

---

*Master volume note: Chapters 1~15 (v1.3.28 ~ v1.3.15 / pairing relations) are this volume's continuation (2026-10-28/29); Chapter 16 is the v1.3.14 report archived verbatim, with details deferring to each version's original report. For the DLL-side master volume see `../dll/changelog/iteration-report-master.md`.*

---

## Chapter 17: v1.3.28 Addendum (2026-11-04, Version Number Unchanged)

**Nature**: proposed via the human developer's on-machine annotations (yellow/green boxes), located and implemented by WorkBuddy (GLM); version number unchanged, the DLL untouched by a single byte.

### Addendum 1: Summary card key names drop the ini English prefixes, becoming pure Chinese (yellow box)

- **Before**: the right card's "Currently effective config" had 9 key names carrying ini key prefixes (`folderExt · 文件对话框`, `imgAlpha · 不透明度`…); the 108px key column couldn't fit "prefix + Chinese", and in testing `folderExt · 文…`, `imgAlpha · 不透...` truncated in patches.
- **Fix**: all key names dropped the English prefixes, keeping only Chinese (文件对话框 / 忽略错误 / 随机显示 / 自定义目录 / 显示位置 / 不透明度 / 自定义路径 / 图片张数 / DLL 状态); key column 108→80, giving the width to the value column — long path values like `G://手机备份//I...` can show a bit more.
- **Knock-on benefit**: the "summary key names show ellipsis within the key column" item in the master volume's "6. Current Status and Known Limitations" **no longer occurs** from now on (all key names are shorter than the key column).

### Addendum 2: Smart column widths for the image management page list (green box)

- **Before**: the name column was fixed at "full width-84-8", not counting the ~17px of client width consumed by the vertical scrollbar; with 146 images the list grew a horizontal scrollbar and the size column was clipped by the right edge — `11.1 MB` left only `11.1 ME`.
- **Fix**: after `refresh_image_list()` finishes filling, smart column widths were added — the size column adapts to content (minimum 60px), and the name column adapts to the longest filename capped at the remaining width: with short names the size column shifts left and displays fully, with no horizontal scrollbar; with overlong names the name column caps and the horizontal scrollbar takes over.

### Technical Notes (for future maintainers)

- **After line 3227's `#undef S` lies a "no-S-macro hygiene zone"**: the S macro (S(val)=pt_scaled(val), defined at line 2313) is explicitly undef'd at line 3227; new code after it must call `pt_scaled()` directly. The first draft of Addendum 2 mistakenly used `S()`, which compiled with an implicit-declaration error; it was changed to call `pt_scaled()` directly.
- The Chinese build's addendum and the English build (backimg-gui_en_v1.3.28.exe) do not affect each other: the English source trio is backed up in `backimg-gui-release\en-source-backup\`; the English exe (built 16:54, containing all fixes EN-1~6) was untouched by this round of changes, and the release package MD5 was re-verified as matching.

### Verification

- MinGW compile `Build OK` (-Wall zero warnings); the new key names (图片张数/不透明度) are embedded in the exe, with zero residue of the old key name (folderExt · 文件对话框).
- Pending the human developer's on-machine re-check: ① the right card's 9 key names in pure Chinese without truncation; ② on the image management page, the size column complete for short names and the scrollbar taking over for long names; ③ no regression on the other pages.

*This addendum was executed by WorkBuddy (GLM); the details are synced with the release process of the English build's changelog `CHANGELOG_v1.3.28-en-ui.md`.*

---

## Chapter 18: Pre-Release Dual-Build Final Check (2026-11-04, WorkBuddy / GLM Independent Audit)

**Nature**: the final independent check after all fixes were completed in both the Chinese and English builds, before release. The human developer set the acceptance standard — "just make it work properly, no nitpicking" (the GUI shell is not a resident system component), so this round focuses on correctness and usability, not style-level quibbles.

### Check Matrix (Four Layers)

| Layer | Check item | Chinese build | English build |
|---|---|---|---|
| 1. Compilation | Clean compile (-Wall fully on) | `Build OK`, **0 warnings** | `Build OK`, **0 warnings** |
| 2. Source integrity | main.c byte-by-byte comparison against backup | Matches `cn-main-with-addendum.c` | Archived source contains the EN-7 smart column widths patch (signature comment + `pt_scaled(60)`×4) |
| 3. Binary content | Binary string spot checks | [图片张数]✓ [应急禁用]✓ old prefix [folderExt · 文件对话框]=0 | [Emergency off]✓ [Fill]✓ Chinese UI [保存配置]=0 |
| 4. Dual-build isolation | Cross-contamination | No English UI bleed-in | No Chinese UI residue |

### Technical Notes

- **MD5 difference clarification**: two compiles of the same source produce exes of identical size but different MD5 — caused by the compile timestamp embedded in the PE header; this is normal. Content consistency is judged by string spot checks and source comparison. The Desktop root's `backimg图形板_v1.3.28.exe` has been synced to the latest compile.
- **Build-file state consistency**: during the final check it was found that the shell source directory's `resource.rc` (locale 0409) and `build.cmd` (English output name) were still in the English build's state (a result of rotating the English patch in); they have been restored from the D-drive master copy to the Chinese build (0804 + the `backimg图形板` output name). The English trio (main.c / resource.rc / build.cmd, containing EN-1~7) is fully preserved in `backimg-gui-release\en-source-backup\`, to be taken from there when publishing the English source branch.
- **Embedded resources note**: the `config.ini` and `ExplorerBgTool.dll` embedded in both builds' exes are the same copies (the DLL is the untouched v1.3.28 original; config.ini has had its development paths cleared), consistent with the established boundary of "not touching the DLL by a single byte".

### Verdict

**Both the Chinese and English builds passed all four layers of checks — no code errors, no warnings, no cross-contamination — and are ready for release.** Release content: the Chinese zip and the English zip (named `_english-ui`) hung side by side on the GitHub Release; the English source pushed to the `english-ui` branch.

*This final check was executed by WorkBuddy (GLM); the acceptance standard was set by the human developer ("just make it work properly").*
