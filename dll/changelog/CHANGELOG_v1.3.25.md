# Changelog — v1.3.25 (the shell v1.3.25 + DLL v1.3.25) · Second-level Rotation + Directory Self-healing Edition

**Release date**: 2026-10-29
**Baseline**: backimg_GUI_v1.3.24 (desktop copy kept as-is, unmodified)
**Nature**: driven by two user requests —
① [UI] The timed rotation custom-time input box narrowed + a new "Minutes/Seconds" unit dropdown added (user red/green boxes);
② [DLL fix] A real on-machine bug of occasional full-window no-background when using a custom image directory outside the program directory (e.g. G://MyPics)
(user repro path: set to G: → not displayed; set back to the plugin directory →
restored; set to G: again → restored again. Not deterministic; transient in nature).

---

## I. The shell (main.c / resource.rc / config.ini)

### D1 [UI] Rotation interval granularity down to seconds + unit dropdown

- **Key points of the user's exact words**: the custom time box (red arrow) "shrink it a little, just enough to show all the digits";
  add a box (green arrow) after it to select "seconds or minutes"; optimize the sizes of the three boxes, without blocking the content in front.
- **Data model**: `v_rotmin` (integer minutes) → **`v_rotsec` (seconds, the single authoritative unit, 0 = no rotation,
  cap 86400 = one day)** + `v_rotunit` (display unit in the custom state: 0 = minutes 1 = seconds).
- **Layout** (right side of the random row, right to left):
  - The preset-level dropdown button unchanged (starting at cx, width S(112));
  - New **unit dropdown** "Minutes/Seconds": width S(56), immediately left of the preset dropdown (cx-S(58));
  - Custom input box **S(60)→S(42)**: fits only 5 digits (86400), attached to the left of the unit box;
  - The "Timed rotation" caption text, in the custom state, moves left to the left of the input box (cx-S(64) → cx-S(168));
    in the non-custom state its position is unchanged; neither intrudes into the left-side switch text area.
- **Unit-switch semantics** (on_rotunit_menu / rot_ui_sync):
  - Switch to "Seconds": the actual interval in seconds is unchanged, only the display changes;
  - Switch to "Minutes": switch the display only if the seconds are an exact multiple of 60; otherwise **round up to minutes and
    write a log line "interval rounded up to N minutes"** (the input box is an ES_NUMBER integer; "90 seconds" cannot be
    faithfully displayed in minutes — better to say so than to silently jump the value);
  - rot_ui_sync display-fallback consistency: when v_rotunit=minutes but the seconds are not a whole minute, the display automatically
    falls back to seconds **and writes v_rotunit back** — otherwise save/EN_CHANGE would miscalculate "90 seconds"
    as 5400 seconds in minutes (a hidden danger found during coding, already fixed).
- **Preset levels**: No rotation / Every 1 minute / Every 2 minutes / Every 5 minutes / Every 10 minutes /
  Every 30 minutes / Custom… (options unchanged; internally stores seconds 0/60/120/300/600/1800/-1).
  Selecting "Custom…" keeps the starting point at 3 minutes (180 seconds); the user can immediately switch to seconds.
- **EN_CHANGE real-time sync** (on_rotmin_edit_change renamed on_rotval_edit_change):
  convert to seconds per the current unit, clamped to 0~86400.
- **Config keys**:
  - New keys `randomVal` (seconds) + `randomUnit` (min/sec, for panel echo only);
  - The compatibility key `randomMin` is still written in sync (= ceil(seconds/60); on the old DLL a seconds-level interval
    rotates per 1 minute, so no functionality is lost);
  - Reading prefers randomVal; on a missing key it falls back to randomMin×60 (seamless upgrade of old configs).
- **Copy summary**: the `randomMin : N minutes` line becomes `randomVal : N minutes (N seconds)` /
  `N seconds` / `No rotation` (on_copy_summary).
- **Resources/version**: GUI_VERSION / DLL_VERSION = 1.3.25; resource.rc
  FILEVERSION 1,3,25,0; the embedded config.ini template gains comments for the three keys.

## II. DLL (dllmain.cpp)

### D1 [Config] Rotation interval granularity down to seconds

- `std::atomic<int> rotateMin` → `rotateSec`; RotateTimerProc / ApplyRotateTimer
  period changed to `seconds × 1000ms` (within the SetTimer cap; one day = 86,400,000ms < 0x7FFFFFFF).
- LoadSettings parsing: randomVal (seconds) preferred, randomMin×60 compatible; clamped to 0~86400.
- Rotation signature (ApplyRotateTimerAll's "reinstall only if changed") unit synchronized to seconds.
- Initialization log/loading log display "every N minutes / every N seconds" for readability.

### D2 [Fix] Custom image directory occasionally not displayed (three layers of defense N1/N2/N3)

- **Cause analysis** (checked item by item along the v1.3.24 code path):
  LoadSettings makes a one-shot verdict: `IsDirExisting` passes (GetFileAttributes only proves
  the metadata is reachable) → enumerating the 8 suffixes yields no handle at all → clears imageList and returns,
  **with no remediation path afterward** — already-open windows do not reload; only the next newly opened window retries;
  while "the directory exists but FindFirstFile cannot get a handle" is a real transient (disk not ready/volume
  mounted late after reboot/AV transiently locking the directory), which exactly explains the user's "occasionally not displayed, fiddling with the settings fixed it".
- **N1 path normalization**: folder trimmed of leading/trailing whitespace/quotes/extra trailing backslash (drive roots like "G:\" preserved);
  a hand-edited ini value such as `G://MyPics\` no longer creates judgment ambiguity.
- **N2 enumeration transient retry**: when not a single file was enumerated and the directory cannot even list `*` at that moment
  (DirListable probe) → retry in place 3×200ms, then re-enumerate; **a truly empty directory can list `*`,
  never reaches the retry, and does not slow window opening** (the worst 600ms happens only on the abnormal path).
- **N3 empty-list self-healing reload**: if the above still fails → `s_retryWanted=true` and attach a 3-second one-shot
  `RETRY_TIMER_ID('BGT1')` timer to all registered windows
  (LoadRetryTimerProc → re-runs LoadSettings(true)); after recovery, clear every window's
  retry timers and InvalidateRect for a full repaint; on continued failure, retry every 3 seconds (one
  GetFileAttributes + one enumeration, negligible overhead) until the directory recovers.
  When MyCreateWindowExW registers a new window, if the list is empty and s_retryWanted, likewise attach
  the retry timer first — the scenario of a slow disk becoming ready after an explorer restart also recovers automatically.
- **Wrap-up alignment**: load succeeds/file set unchanged → s_retryWanted=false; if after decoding not a single
  image made it into the list (all files happened to be transiently unreadable), also treat it as failure and arm the self-healing.
- **Exception boundary**: LoadRetryTimerProc swallows exceptions in try/catch throughout, the same discipline as RotateTimerProc,
  never propagating back into explorer.

---

## Verification record

| Item | Result |
|---|---|
| DLL compilation | g++ -std=c++17 -O2, zero errors, only the pre-existing benign `#pragma comment` warning (present in all historical versions) |
| EXE compilation | gcc -std=c11 -O2 -Wall, zero errors zero warnings |
| Byte signature | DLL contains v1.3.25 / randomVal / "每 %d 秒" (UTF-16 check); EXE version resource 1.3.25.0 |
| Unit consistency | after rot_ui_sync writes back v_rotunit, display value × unit ≡ actual seconds (EN_CHANGE / save / read-back — three paths share one conversion) |

## Fingerprints

- ExplorerBgTool.dll: **942,255 bytes**, SHA256
  `B6A4966A772724977CFA4763E7182115E623425FE8941C3785EE8F0673152474`
- backimg图形板_v1.3.25.exe: 1,260,178 bytes, SHA256
  `6C42E44008C1870C1105D3B402A466B921A5B996E02CE246960A26D7CB4F3C7E`
