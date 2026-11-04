# Changelog · v1.3.26 (Version-sync Edition)

- **Date**: 2026-10-29
- **Nature**: a minor version driven by GUI UI changes; DLL functionality zero changes, only the version stamp synced
- **Previous version**: v1.3.25 (Second-level Rotation + Directory Self-healing Edition)
- **Driver**: user red box — "the two custom-time boxes are squeezed into the switch row; there is a lot of room below,
  moving them down lets the time display more fully, and the two words 'minutes' and 'seconds' also display more fully"

---

## I. GUI side (backimg Panel v1.3.25 → v1.3.26)

### E1 Timed rotation custom controls moved to a dedicated sub-row (user red box; the only UI change this round)

- **Before the change (v1.3.25)**: the right side of the random switch row squeezed four things right-to-left —
  `[preset dropdown S(112)][unit box S(56)][input box S(42)][caption text]`,
  the input box narrowed to 42 logical pixels fitting only 5 digits, and the word "Seconds" in the 56-wide unit box was also pressed against the edge
- **After the change (v1.3.26)**:
  - The top random row keeps only `[caption text][preset dropdown S(112)]`; the caption text is fixed and no longer slides horizontally
  - Below the random row, **a permanent "Custom interval" sub-row** (height S(28), layout used +28 in sync):
    `Custom interval  [input box S(80)]  [unit box S(88)]`
  - Input box S(80): fits 6 digits, far above the 86400 (one day) cap, no more "not fully displayed" concern
  - Unit box S(88): **the word "Minutes"/"Seconds" displayed in full**, the current unit known without opening the dropdown
- **Show/hide rules** (rot_ui_sync):
  - Controls (input box + unit box): shown when `custom level && random on`; grayed out and hidden when random is off (same as v1.3.25)
  - Sub-row hint `Select "Custom…" above and fill the rotation interval here`: shown when `random on && not the custom level`;
    hidden together when random is off (there is nothing to fill then; the hint would only mislead)
  - The sub-row space is permanently reserved; switching levels **does not cause the layout to jump open/collapse**
- **Unchanged**: the 7-level preset dropdown, the data model v_rotsec/v_rotunit, EN_CHANGE real-time sync,
  save/read/summary logic, the automatic fallback to seconds display for non-whole-minute values — all as before

### Incidental

- resource.rc version resource 1.3.26.0; build.cmd VER=1.3.26
- The embedded DLL resource and plugin\ExplorerBgTool.dll come from the same source (v1.3.26 synced version)

## II. DLL side (ExplorerBgTool.dll v1.3.25 → v1.3.26)

- **Zero functional changes**. Only two items:
  1. Initialization log first-line version stamp `v1.3.25` → `v1.3.26`
  2. Self-healing log line prefix `v1.3.25 :` changed to the neutral `提示 :` (avoiding a version number nailed into the log copy)
- Config keys (randomVal / randomUnit / randomMin compatibility), protocol, and timer behavior are fully identical to v1.3.25;
  **the two DLL versions can be swapped directly**
- File size unchanged: 942,255 bytes (version stamp replaced at the same length)

## III. Artifact verification

- ExplorerBgTool.dll: **942,255 bytes**, SHA256
  `1D4D7E882E350FD8775739FDA82658A2C17D2DA5DDF48329650B3C00DBC2F766` (first 32 hex `1D4D7E882E350FD8775739FDA82658A2`)
- backimg图形板_v1.3.26.exe: **1,261,123 bytes**, zero errors zero warnings (gcc -Wall)
- The DLL bytes are identical in plugin\ and build\; the shell's embedded resource comes from the same source as plugin

## IV. Upgrade method

1. Overwrite the old version with the pair `backimg图形板_v1.3.26.exe` and `plugin\ExplorerBgTool.dll`
2. Open the panel → save config → restart Explorer (or click "Restart Explorer" on the panel)
3. Verify: the runtime log's first line should read `ExplorerBgTool v1.3.26`; title bar `v1.3.26 (paired DLL v1.3.26)`

---

*v1.3.26 changelog | 2026-10-29 | 1 UI change (E1 sub-row), DLL version sync*
