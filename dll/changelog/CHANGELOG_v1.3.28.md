# Changelog · v1.3.28 (2026-10-29)

**Theme: Deepseek re-review of v1.3.27's three fixes (P1 real bug + P2/P3 tidiness and UX)**

- Nature: GUI-side fix edition; DLL functionality zero changes, only the version stamp 1.3.27→1.3.28 synced
- Review source: the user sent out the《backimg_GUI_v1.3.27_代码复审报告.txt》(Deepseek, including verification of the previous round's F1/F2/F3 fixes)
- Review verification conclusion: F1 and F2 fixed correctly; F3's fix direction correct and its main scenario achieved, but it introduced Issue 1 (see below)

---

## 0. Review conclusion verification (verify first, then fix; veracity stated)

| Review issue | Verification process and conclusion | Disposition |
|---|---|---|
| **Issue 1**: `g_rot_custom_picked` not reset in `load_config` | **Confirmed**. The whole file has only 2 assignment sites (menu selects "Custom…" sets true / selects a preset sets false); the end of load_config's rotation-read block (randomVal/randomUnit/randomMin compatibility paths) goes straight to `sync_switch_ui() → rot_ui_sync()` with no reset of any kind. Trigger path: the user has previously selected custom → the bottom "Restore config" (on_restore → load_config) restores a backup with a preset level → `custom` still true → the UI wrongly shows the "Custom…" level + input box, inconsistent with the actual config | **P1 fix** |
| **Issue 2**: `hRotLabel` becomes an "assigned but never read" dead variable | **Confirmed**. In the whole file it appears only at line 2337 (declaration) and line 2825 (assignment), never read (it became dead when v1.3.26 removed the "caption text horizontal slide"). The review also noted that gcc -Wall by default does not report file-level statics that are written but never read — this matches the measurement and does not affect "zero warnings" | **P2 fix** |
| **Issue 3**: re-selecting "Custom…" while already in the custom state with 120 filled resets it to 180 | **Confirmed** (pre-existing logic; present since v1.3.25's `preset` reset; not newly introduced this round; the review only marked it "FYI") | **P3 fix** |

---

## I. GUI-side fixes (main.c, 3 places total)

### P1: load_config resets the custom-state flag after reading the rotation config

```c
/* Added at the end of load_config()'s rotation block (after clamping): */
g_rot_custom_picked = false;
```

- Semantics: this flag expresses only "the user actively chose custom during this session"; **it becomes invalid the moment the config changes** —
  when a preset level is restored (e.g. randomVal=60), the UI must faithfully return to the "Every 1 minute" level
- Trigger surface: the bottom "Restore config" (.bak restore) and any load_config call path;
  the right card's "Restore original config" goes through on_restore_prev and does not pass load_config, and does not touch the rotation config anyway, so it is unaffected

### P2: hRotLabel dead variable removed

- The file-level `static HWND hRotLabel;` is deleted; in rot_ui_init the `new_label_ex` return value is simply discarded
- From here on, the rotation UI's file-level state is only: the two sub-row control handles (hRotSubLabel/hRotSubHint) +
  the custom-state flag (g_rot_custom_picked) + the level/unit ComboState — no redundancy

### P3: re-selecting "Custom…" while already in the custom state does not reset the filled value

```c
bool already_custom = g_rot_custom_picked;   /* take the old value before setting the flag */
g_rot_custom_picked = true;
if (!already_custom) {
    /* Only "first entry into custom from a preset level/invalid value" gets the 180 non-preset starting point */
    ...
    if (preset || g_app.v_rotsec <= 0) { g_app.v_rotsec = 180; g_app.v_rotunit = ROT_UNIT_MIN; }
}
```

- The user's hand-typed 120 (exactly "Every 2 minutes") **stays at 120** after reopening the dropdown and selecting "Custom…" again
- Note the implementation order: `already_custom` must be taken before setting the flag; otherwise the flag becomes true first and the check never fires

## II. DLL side (version sync, zero functional changes)

- Internal log version stamp `ExplorerBgTool v1.3.27` → `v1.3.28`
- The header comment gains a v1.3.28 entry (three-fix summary + directly swappable with v1.3.27 note)
- Config keys (randomVal/randomUnit/randomMin compatibility), protocol, timers, and the directory self-healing behavior are **fully identical to v1.3.27**

## III. Verification

- DLL: g++ compile zero errors zero new warnings, **942,255 bytes** (same size as v1.3.25~27, only the string stamp differs),
  SHA256 `7846DEBD35F5F259A75BC45A047B13DB8B8360C1777C7CB47572EAEE183C18E7`
- Byte-signature check: the DLL's UTF-16 strings contain `v1.3.28` and do not contain `v1.3.27` ✓; plugin and build artifacts SHA256 identical ✓
- EXE: gcc -Wall zero errors zero warnings, **1,261,161 bytes**, version resource 1.3.28.0,
  SHA256 `B8EFD1936B0EB430A6859BCAFACAC07925996135603CAFA96EC4052047242DC5`
- Re-check confirms: after the P1 fix, the "Restore config" path's rot_ui_sync determination matches the restored config value;
  after the P3 fix, the menu check-mark logic (on_rot_menu) and the panel display (rot_ui_sync) use the same
  `g_rot_custom_picked || value-inference` determination, and the two no longer conflict

## IV. Archive of the previous round's (v1.3.27) fix verification

- [F1] Sub-row title show/hide follows the random switch — verified correct, kept as is
- [F2] Five file-level globals demoted to locals — verified correct; this round's P2 also catches the missed hRotLabel
- [F3] Independent state flag for the custom state — main scenario correct; this round's P1/P3 close its two edge cases
