# Changelog · v1.3.27 (Deepseek V4 Pro Review-Fix Edition)

- **Date**: 2026-10-29
- **Nature**: the user sent v1.3.26's full changes out for a Deepseek V4 Pro review; the three reported issues were verified item by item, then fixed
- **Previous version**: v1.3.26 (Rotation Custom Controls Moved to Sub-row Edition)
- **Review report**: `(local path omitted) backimg_GUI_v1.3.26_代码审查报告.txt` (Deepseek V4 Pro)

## 0. Review conclusion verification (veracity first, fix approach second)

| Review issue | Verification conclusion | Disposition |
|---|---|---|
| Issue 1: the sub-row title "Custom interval" does not show/hide with the random switch | **Confirmed** — after creation hRotSubLabel has no ShowWindow control at all; with random off, the controls/hint are all hidden, leaving only an orphaned title row | **F1 fix** |
| Issue 2: five global variables are assigned but never read | **Partially confirmed** — g_rot_cx and g_rot_sub_y are indeed dead variables (assignments with no readers); but g_rot_label_y / g_rot_edit_x / g_rot_unit_x are read within rot_ui_init. Also, the claim "it would trigger gcc -Wall's -Wunused-but-set-variable" is inaccurate — that warning only covers local variables; an assignment to a file-level static already counts as a reference, so in practice there is no warning | **F2 cleanup** (all five demoted to rot_ui_init locals, more thorough than the review suggested) |
| Issue 3: a custom value exactly equal to a preset level jumps back to the preset | **Confirmed** (pre-existing since v1.3.25) — the custom state is inferred purely from the value; typing 120 by hand, which hits "Every 2 minutes" exactly, made the input box vanish on the spot. The review also noted that the widened input box makes it easier to hit | **F3 fix** (added an independent state flag per the review's suggestion) |

## I. GUI side (backimg Panel v1.3.26 → v1.3.27)

### F1 Sub-row title show/hide follows the random switch (review issue 1)

- rot_ui_sync gains `ShowWindow(hRotSubLabel, v_random ? SW_SHOW : SW_HIDE)`
- With random off, the sub-row has no content at all: title/input box/unit box/hint all hidden, no orphaned text left behind

### F2 Dead variable cleanup (review issue 2)

- The five file-level globals left over from v1.3.25 — g_rot_cx / g_rot_label_y / g_rot_sub_y /
  g_rot_edit_x / g_rot_unit_x — after v1.3.26 removed the "caption text horizontal slide" logic
  had no cross-function readers (or readers only within rot_ui_init) — **all demoted to
  rot_ui_init locals**; at file level only the two control handles hRotSubLabel / hRotSubHint
  remain + the g_rot_custom_picked flag
- Recompiled with gcc -Wall: verified zero warnings, consistent with the CHANGELOG's claim

### F3 Custom state switched to an independent state flag (review issue 3)

- New `static bool g_rot_custom_picked`:
  - User clicks "Custom…" in the preset menu → set true; clicks any preset level → set false
  - rot_ui_sync's determination changes to `custom = g_rot_custom_picked || (cur < 0 && v_rotsec > 0)`;
    when the flag is set, the preset dropdown always shows "Custom…" and the input box stays visible
  - At startup load the flag is false; the display is inferred purely from the value (if the config is a preset, the preset level is faithfully shown)
- **Effect**: when the user's hand-typed 60/120/300 etc. exactly equals a preset's seconds, the input box no longer
  vanishes on the spot and the level no longer jumps back to the preset; the saved value is unchanged (value semantics fully equivalent to the preset; only the display does not jump)
- on_rot_menu's menu check-mark logic synchronized to the same determination (previously the menu would also check 120 on
  "Every 2 minutes", inconsistent with the panel display)
- Save path verification needs no change: save reads from the visible input box, and after F3 the input box is always visible in the custom state — self-consistent

## II. DLL side (ExplorerBgTool.dll v1.3.26 → v1.3.27)

- **Zero functional changes**; only the initialization log's first-line version stamp 1.3.26 → 1.3.27 synced with the shell
- Config keys / protocol / timer behavior fully identical to v1.3.26; the two DLL versions can be swapped directly
- File size unchanged: 942,255 bytes

## III. Artifact verification

- ExplorerBgTool.dll: **942,255 bytes**, SHA256
  `85E19DF9FD4F36FB9E28EAB7A83B4415B7B53AB4C375CB41E58D2CAEBDE2F86B` (first 32 hex `85E19DF9FD4F36FB9E28EAB7A83B4415`)
- backimg图形板_v1.3.27.exe: **1,261,161 bytes**, gcc -Wall zero errors zero warnings
- The DLL bytes are identical in plugin\ and build\; the shell's embedded resource comes from the same source as plugin

## IV. Upgrade method

1. Overwrite the old version with the pair `backimg图形板_v1.3.27.exe` and `plugin\ExplorerBgTool.dll`
2. Open the panel → save config → restart Explorer (or click "Restart Explorer" on the panel)
3. Verify: runtime log first line `ExplorerBgTool v1.3.27`; title bar `v1.3.27 (paired DLL v1.3.27)`

---

*v1.3.27 changelog | 2026-10-29 | Deepseek V4 Pro review's three issues: F1/F2/F3 all fixed*
