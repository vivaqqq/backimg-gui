# backimg Panel English UI · Changelog (CHANGELOG v1.3.28-en-ui)

| Item | Content |
| --- | --- |
| Baseline | Chinese v1.3.28 (backimg Panel, including all 14 rounds of Chinese-edition iterations) |
| Scope of changes | **Shell UI text and layout constants only**; zero changes to code logic; the DLL not touched by a single byte (still v1.3.28, shared with the Chinese edition, keeping the SHA256 pairing check consistent) |
| Artifacts | `backimg-gui_en_v1.3.28.exe` + `backimg-gui_v1.3.28_english-ui.zip` |
| Source | `shell/main.c` (english-ui branch, synced at release) |

---

## EN-0 Translation baseline (WorkBuddy / GLM)

- All 307 unique strings (377 occurrences) translated into English: short English for UI labels to prevent overflow, natural English for logs
- `resource.rc` localized to the English locale (040904B0 / 0x0409), version info fully in English
- Passed compilation on the first try; exe binary scan: zero Chinese string residue (UTF-16/UTF-8 dual encoding)

## EN-1 ~ EN-3 Three rounds of layout corrections by the human developer (on-machine screenshots with red/blue/green box annotations)

> All three rounds below were **raised by the human developer during on-machine acceptance**; WorkBuddy located the code and implemented:

**Round 1 (Basic config page + Image manager page):**

| # | Issue | Fix |
| --- | --- | --- |
| 1 | `Browse...` button text touching the edge | Button 60→68px; the input box's right edge reduced by 12px to yield |
| 2 | The folderExt row's gray caption text truncated at the right edge | Condensed to "Folder windows only; tick adds Open/Save dialogs too" |
| 3 | `Custom image f...` card title truncated | Label width 110→150px (the [folder] chip follows dynamically via text_width) |
| 4 | `Ignore errors` row caption borderline | Condensed to "No popup on missing image or error; keep ticked" |

**Round 2 (Image manager page + Per-folder background page):**

| # | Issue | Fix |
| --- | --- | --- |
| 5 | `Refresh list` button truncated | Width 70→88px |
| 6 | `Open image folder` clipped at both ends | Width 100→122px, moved right to yield spacing |
| 7 | `Per-folder bac` title truncated | Label width 100→180px |
| 8 | `Add/Update` button clipping characters at the edges | Width 68→76px, right-aligned position shifted outward in sync |
| 9 | `Image file nam` label truncated | Width 80→110px (the [Inside Image] chip follows dynamically) |
| 10 | `elete selecte` clipped at both ends | `Delete selected` 70→100px |
| 11 | `Refresh list` (rules page) squeezed | Moved right to 122px, width 70→86px |
| 12 | Rule caption sentence truncated at the right edge | Condensed to "Select a rule on the left to autofill; Add/Update overwrites it." |

**Round 3 (Per-folder background page details):**

| # | Issue | Fix |
| --- | --- | --- |
| 13 | `Add/Update` text touching the rounded border | Button 76→88px, padding on both sides between text and border |
| 14 | Zero spacing between the button and the Folder path input box | Input box right edge reduced by 12px, restoring the standard 8px spacing |

## EN-4 Deepseek V4.1 Flash 1017 audit fixes (2026-11-04; on-machine run + three-page screenshots + minimum-window-width comparison + source locating)

> Audit report: `backimg英文版UI审查_2026-11-04\审查报告.md` (P1 five items / P2 four items + one missed / P3 six items / P4 three items)

**P1-1 Five card titles hard-cut (not even an ellipsis) + hint line truncated:**

| Location | Old width | New width |
| --- | --- | --- |
| Display options (main.c:2401) | S(80) | S(150) |
| Image position (main.c:2470) | S(90) | S(140) |
| Live configuration (main.c:2529) | S(110) | S(170) |
| Timed rotation (main.c:2827, right-aligned) | S(56)/x=cx-S(64) | S(140)/x=cx-S(148) |
| Custom interval (main.c:2837) | S(64) | S(150) |
| Custom interval hint line (main.c:2852) | S(230), starting at edit_x | S(424), changed to start at the row's sx |

**P1-2 Unit dropdown `Seconds` crushed by the arrow:** dropdown width S(88)→S(96)

**P1-3 Summary card key names truncated across the board (the most readability-damaging spot in the English edition):**
- The 9 key names drop the ini key prefix: `folderExt · File dialogs` → `File dialogs` etc. (the ini keys remain cross-referenceable in config.ini; readability first)
- Key column keyw2 S(108)→S(96); the freed width goes entirely to the value column (long values like `not set → using plugin folder/Image` benefit)

**P1-4 At the minimum window width 880, the bottom bar's 7 buttons overlap each other (the Chinese edition at the same width has no overlap; introduced by the longer English copy):**
- `WM_GETMINMAXINFO` minimum tracking width 880→**1020** (the bottom bar buttons are text_width-dynamic; estimated total demand ~990px; 1020 chosen for headroom)

**P1-5 In a narrow window the random chip is covered by the `Timed` label:** solved along with the minimum-width raise (after 880→1020 the left card width 466→486; the chip and the right-side label no longer enter the same area)

**P2 Full-width punctuation zeroed out (7 spots, including 1 narrow-string twin caught by the post-audit final sweep):**
- `（%d%%）` → `(%d%%)` ×4 (summary line/slider readout/measurement string/summary export)
- `config.ini：%s` → `config.ini: %s`
- Popup `usysdiag。` → `usysdiag.`
- Title bar `backimg Panel  v1.3.28` double space → single space

**P3 Wording (5 of the audit's 6 suggestions adopted):**

| Original translation | Changed to | Reason |
| --- | --- | --- |
| `Panic off` (6 linked spots) | `Emergency off` | "panic" in an English context means "program crash" |
| `Fit & fill` (3 linked spots) | `Fill` | Semantically collided with the previous level `Fit` |
| `Inside Image` | `In Image folder` | The literal translation lacked a noun |
| `png/jpg/bmp are most stable` ×2 | `png/jpg/bmp recommended` | Wording + the two spots were inconsistent |
| Registry warning sentence | `The DLL registered in the registry differs from the one in the current plugin folder...` | The original sentence was missing a component |
| `Reveal` | **Kept** | The audit marked it "optional"; constrained by the 48px button width, and "Show in Explorer" would require re-laying out the whole row |

**P4 Legacy issues (present in the Chinese edition too):**
- P4-2 The preview meta line is measured with the regular font while the control is bold → measurement switched to the bold font (long file names no longer stick to the size)
- P4-3 The exe's embedded config.ini carried a dev path `folder=D:\backimg_GUI_v1.3.17\Image` → cleared (users who delete config.ini and re-extract no longer get an invalid path)
- P4-1 The per-folder background page's first-open count `0 rules`, requiring a manual refresh → **not fixed** (historical behavior present in the Chinese edition too; the change involves page-init ordering, left for upstream to handle uniformly)

## Known behaviors (not bugs)

- The DLL-side `ExplorerBgTool_init.log` runtime log remains Chinese (the DLL is shared with the Chinese edition as v1.3.28, keeping the pairing check and the hook chain stable)
- Audit fix suggestion No. 5 (a dev-time self-check comparing `new_label_ex`'s fixed width vs the measured width) not implemented; listed as an optional improvement
- The window minimum width raised from 880 to 1020 (scaling factor linked); the bottom bar no longer overlaps at extremely narrow widths

## Verification record (2026-11-04)

- Build: MinGW-w64, `Build OK`, zero errors
- exe binary scan: zero Chinese string residue (UTF-16/UTF-8); zero full-width punctuation residue (literal level); zero dev-path `backimg_GUI_v1.3.17` residue
- English string embedding confirmed: `Restart Explorer`×6, `Emergency off`, `Live configuration`, etc.
- Release package: the exe inside the zip matches the latest on-disk build by MD5; config.ini contains no private paths; no winsize.ini/gui_log.txt runtime garbage

## Review checklist (for the next acceptance round)

- [ ] Three pages at 100% scaling, all card titles fully displayed
- [ ] Switch to the `Custom…` level; the unit dropdown fully displays `Seconds`
- [ ] The summary card's 9 key names fully readable; no more `Fil…` / `Ignor…`
- [ ] Drag the window to 1020 wide; the bottom bar's 7 buttons do not overlap
- [ ] In a narrow window the random row's chip is not covered by `Timed`
- [ ] The count of full-width punctuation in the UI and popups is 0
- [ ] The title bar and the `DLL:` log line have no double spaces
- [ ] The `Emergency off` confirmation popup, the `In Image folder` chip, and the `recommended` count display correctly

---

## EN-5 Deepseek V4.1 Flash 1017 second-round re-review fixes (2026-11-04)

> Re-review report: `backimg英文版UI审查_第二轮_2026-11-04\复核报告.md` (extracted the official zip and tested live + read real control rects via EnumChildWindows + item-by-item source locating)
> Re-review conclusion: round one's P1 five items all fixed, P2 seven spots zeroed, P3 adopted 5/6, P4-2/P4-3 fixed — **the main UI is judged release-ready**; the remaining 4 visual details (R1-R4) all fixed this round, plus a root-cause fix for P4-1.

| # | Issue | Fix |
| --- | --- | --- |
| R1 | `Fit & fill` remained in `POS_NAMES[6]` (main.c:311) and the caption line (main.c:2507), inconsistent with the previous round's CHANGELOG — both were occurrences missed by the first-round replacement | `POS_NAMES[6]` → `Fill` (linked with the position button and the summary's `%d · %s`, same source); caption line → `"Fill" = fill the window and crop the overflow` |
| R1 rider | The caption line's single-line height only fits the first sentence; users never see the semantic explanation of the `Fit`/`Fill` levels | Caption label line height 14→**30** (two lines); the layout below shifts down 16px as a whole |
| R2 | The `random` chip drops its trailing letter (`rando`): chip width = measured text + padding×2 + 2; the measurement is off by 1-2px | Margin `+2` → `+4` (main.c:1962 new_chip; all auto-width chips benefit) |
| R3 | At the minimum width 1020, the activation procedure's step 2 caption is cut (`...swapping the I`) | Copy shortened: `Only needed once, or after swapping the DLL` |
| R4 | A 45-character long file name pushes the `GDI+ decode` chip 33px past the card's right edge (caused by P4-2's bold measurement — file names no longer shrink) | `preview_meta_update` gains a **file-name width cap**: when exceeded, truncate character by character and append `…` (looping on the bold measured width); the trio always stays inside the card |
| P4-1 overturned | The per-folder background page's first open shows `0 rules`: this round provides the root cause (the startup order browse_plugin→refresh_rule_list runs before load_config, so the ini is not loaded at the first refresh) and a one-line fix | Adopted: append `refresh_rule_list();` at the end of `load_config()` (same rationale as refresh_image_list); the previous round's "leave it unfixed" verdict is hereby overturned |

## EN-5 review checklist (for the third acceptance round)

- [ ] The last level of the position segment shows `Fill`; the summary line shows `posType · 6 · Fill`
- [ ] The caption label complete in two lines: the first sentence on aspect-ratio-preserving scaling + the semantics of the `Fit` / `Fill` levels
- [ ] The `random` chip displays fully (including the trailing letter m)
- [ ] The activation procedure's step 2 caption complete (`...swapping the DLL`)
- [ ] Select a 45-character long file name: the preview meta trio stays fully inside the card, the file name truncated with `…`
- [ ] A fresh launch straight into the per-folder background page: the rule list immediately shows existing entries (no more 0 rules)
- [ ] Regression: Save config / Register / Restart Explorer / Emergency off popups all normal

---

## EN-6 The human developer's fourth round of on-machine feedback (2026-11-04, blue/green/yellow box annotations)

| # | Issue | Root cause | Fix |
| --- | --- | --- | --- |
| 🔵 | The posType caption line shows only the first sentence (`Always scaled...window live`); the `Fit`/`Fill` semantics explanation is entirely invisible | The static control created by `new_label_ex` uses the `SS_LEFTNOWORDWRAP` style — **word wrapping explicitly forbidden**; adding height to the text does not help | The label's style changed to `SS_LEFT` (wrapping allowed); the caption reorganized into two lines with `\n`: line 1 the aspect-ratio-preserving scaling behavior, line 2 the semantics of the `Fit` / `Fill` levels (compressed to fit within single-line width) |
| 🟢 | The Opacity area's readout `101 / 255(75%)` hugs the card bottom and is clipped in half | The layout is an elastic spacing system (gapA/B/C auto-stretch/shrink), but the "used height" formula **did not include the two caption lines raised by EN-1** (+32px), so the spacing did not shrink as needed | The `used` formula adds the caption line heights (`+ S(2) + S(30)`); the elastic spacing auto-shrinks 16px — i.e. "everything above shifts up", done automatically by the layout engine |
| 🟡 | (The developer's suggested alternative) Move the Image position area up as a whole | — | Not used separately: the elastic spacing shrink already freed enough room; no hard move needed |

## EN-6 review checklist (for the fifth acceptance round)

- [ ] The posType caption complete in two lines: line 1 the aspect-ratio-preserving scaling behavior; line 2 `"Fit" = whole image visible; "Fill" = fill and crop the overflow`
- [ ] The Opacity readout (e.g. `191 / 255(75%)`) fully displayed inside the card, no longer clipped
- [ ] Normal spacing between the Opacity and Custom image folder cards, no overlap
- [ ] Regression: the random rotation sub-row, Emergency off, the summary card, and the seven position levels all normal

---

## Final pre-release code re-check (WorkBuddy / GLM, 2026-11-04)

> At the repo owner's request, WorkBuddy performed a code-level re-check of all English-edition changes before release (no other model dispatched).

**Method**: a full-diff block-by-block check against the Chinese master (v1.3.28 baseline) + baseline/current dual-build warning-set comparison + binary final verification.

**Result:**

| Check item | Result |
| --- | --- |
| Full diff | 5215 lines → 5240 lines; 232 string-change blocks + 12 structural-change blocks, reviewed block by block |
| Format placeholders (%s/%d/%lu/%S/%.1f/%ld) | All 232 old/new blocks compared one by one, **zero lost, zero misaligned** |
| Structural changes | 10 spots as expected (width/position constants, min width, refresh_rule_list, the R4 truncation block, the SS_LEFT style block, the used formula); **2 comment flaws** (see below) |
| Compile warnings (-Wall fully on) | Baseline (Chinese master) 0; English edition **0** — the zero-warning discipline holds |
| Chinese/full-width punctuation in string literals (source level) | **0** (authoritative determination) |
| Binary UTF-16 2-byte pattern count | 19 hits of `（` are **random byte noise** (the expected value for a 1.27MB binary is ~19), not real content; 4-byte character-level Chinese patterns 0 |
| zip integrity | The exe matches the latest build by MD5; config.ini contains no private paths; no runtime garbage |

**The two known comment flaws (zero runtime impact, not release-blocking):**
1. main.c:2343 — the bulk translation mistakenly altered one historical comment ("说明文字横移"→"caption\n * slide"; two comment lines merged into one)
2. main.c:2937 — same kind (the "从预设档/无效值首次进入自定义" fragment turned into English)

> Cause: these two are **quoted fragments inside comments** (quote pairs spanning lines were mistaken for literals by the string regex). Purely at the comment level, with zero impact on compilation and running; if it bothers anyone, they can be flipped back to Chinese in passing next round.

**Verdict: no syntax errors, no lost placeholders, no new warnings, no Chinese UI residue — meets the release condition.**

## EN-7 Smart column widths (the human developer's fifth round of on-machine feedback, 2026-11-04, green box annotation)

**Background**: after the Chinese edition's same-round addendum (Chapter 17 of the master changelog) discovered the image list's horizontal clipping problem, the human developer confirmed on the English-edition build that it exists there too — with short file names the size column is still clipped at the right edge (`11.1 MB` displays as `11.1 ME`).

- **Root cause** (same as the Chinese edition): `refresh_image_list()`'s fixed column widths did not account for the client-area width taken by the vertical scroll bar; with many images the list gains a horizontal scroll bar and the size column is clipped at the right edge.
- **Fix**: the Chinese edition's smart-column-width logic ported as-is to the tail of the English source's `refresh_image_list()` — the size column adapts to its content (floor 60px), and the name column adapts to the longest file name, capped at the remaining width. Short file names → the size column displays fully, no horizontal scroll bar; overly long names → the name column caps and the scroll bar takes over.
- **Technical detail**: this function sits in the hygiene zone after `#undef S` at line 3227; the patch uses direct `pt_scaled()` calls (the same treatment as the Chinese edition's addendum 2).

**Build**: 2026-11-04 17:19 `Build OK` (zero warnings with -Wall); the zips and the exe in the `en-package` directory updated in sync (MD5 `01e9e6ce`).

**EN-7 review checklist (for the sixth acceptance round)**:
- [ ] Image manager page: with short file names the size column displays fully (`11.1 MB` whole), no horizontal scroll bar
- [ ] With long file names (≥45 characters) the name column caps, the horizontal scroll bar appears, and the size column still displays fully
- [ ] Column widths re-adapt correctly after Refresh list / switching directories
- [ ] No regression on the other pages (Basic config / Per-folder background)

## Final Dual-Version Pre-release Audit (WorkBuddy / GLM, 2026-11-04)

**Scope**: an independent final check on BOTH the Chinese and English builds after all fixes (EN-1~7 + CN addendum), immediately before publishing. The human developer set the acceptance bar: *"works correctly is enough -- no nitpicking"*, on the grounds that this GUI panel is not a resident system component. Accordingly this audit targets correctness and usability, not style.

### Check matrix (four layers)

| Layer | Check | Chinese build | English build |
|---|---|---|---|
| 1. Compile | fresh build with `-Wall` | `Build OK`, **0 warnings** | `Build OK`, **0 warnings** |
| 2. Source integrity | byte-level diff vs backup | identical to `cn-main-with-addendum.c` | archive contains the EN-7 smart-column patch (marker comment + `pt_scaled(60)` x4) |
| 3. Binary content | embedded-string spot checks | `图片张数` present, `应急禁用` present, legacy prefix `folderExt · 文件对话框` = 0 | `Emergency off` present, `Fill` present, Chinese UI string `保存配置` = 0 |
| 4. Isolation | cross-contamination | no English UI bleed | no Chinese UI residue |

### Technical notes

- **On MD5 mismatches**: two builds from identical source may differ in MD5 because the PE header embeds the build timestamp; sizes stay equal. Content equivalence was judged by string spot-checks and source diffs. The desktop-root `backimg图形板_v1.3.28.exe` was re-synced to the latest build.
- **Build-file self-consistency**: the audit caught that `resource.rc` (lang 0409) and `build.cmd` (English output name) in the working dir were still the English ones from the EN patch round; both were restored from the D: master (0804 + `backimg图形板` output name). The full English trio (main.c / resource.rc / build.cmd, with EN-1~7) is preserved in `backimg-gui-release\en-source-backup\` and will be used for the `english-ui` branch.
- **Embedded resources**: both exes embed the same `config.ini` (dev path cleared) and the untouched v1.3.28 `ExplorerBgTool.dll`, honoring the "DLL untouched" boundary.

### Verdict

**Both builds pass all four layers -- no code errors, zero warnings, no cross-contamination. Cleared for release.** Release plan: Chinese and English zips (`_english-ui` suffix) side by side on the GitHub Release, English sources pushed to the `english-ui` branch.

*Audit performed by WorkBuddy (GLM); acceptance criteria set by the human developer ("it just has to work").*
