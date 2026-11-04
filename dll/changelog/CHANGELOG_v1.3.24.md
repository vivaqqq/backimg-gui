# Changelog — v1.3.24 (the shell v1.3.24 + DLL v1.3.24) · Audit Loop Wrap-up Edition

**Release date**: 2026-10-29
**Baseline**: backimg_GUI_v1.3.23 (desktop copy kept as-is, unmodified)
**Nature**: Hunyuan 4 independent re-review wrap-up edition — the user switched to Hunyuan 4 for a fourth independent audit. Conclusion:
**P0 zero, P2 zero** (the first double-zero across the first four rounds), with only 3 P3 items (2 one-line defenses + 1 "awareness only"
design trade-off). Item-by-item verification: N1 confirmed (but the trigger conditions are stricter than the report stated, see below), N2 confirmed,
N3 is a design trade-off left unchanged. **After this round of fixes, the audit loop is wrapped up** — audit severity declined monotonically across
the four rounds (P0: 1→0→0→0; P2: many→1→1→0), and each round confirmed that the previous round's fixes had no regression; convergence is judged complete.
**Zero UI changes, zero configuration changes.**

---

## N1 [P3 · Rule write buffer enlargement + read/write unit alignment]

- **Location**: the shell main.c on_rule_add (write side) + rule list refresh / on_rule_sel (read side ×2)
- **Original report statement**: the img source is at most 4096 WCHARs, UTF-8 can reach 12288 bytes, yet imgc is only 1024
  bytes → overly long Chinese names (about 341 characters or more) are silently truncated, and the rule points to a nonexistent image;
  the write side 1024 [bytes] / read side 1024 [WCHARs] units are asymmetric.
- **Verification supplement (key fact the report did not account for)**: img comes from **file names** enumerated by FindFirstFile;
  NTFS caps a single file name at 255 characters — at the UTF-8 worst case of 4 bytes/character = 1020 bytes + NUL
  = 1021 ≤ 1024, **legitimate file names actually fit, so the truncation path cannot be triggered by legitimate input**.
  This item's true identity is a fragile pattern of "the magic number 1024 hugging the theoretical boundary, with no headroom comment".
- **Fix**:
  - Write side `imgc[1024]` → `imgc[12288]` (with the boundary analysis comment above);
  - Read side, two places `char img[1024]` → `[12288]`, `WCHAR imgw[1024]` → `[4096]`,
    aligned with the write side's unit (otherwise enlarging the write side would create a new asymmetry of "writable but not fully readable");
  - The enlargement produces no overly long ini lines: legitimate file names are ≤255 characters, actual written values ≤1020 bytes,
    and the ini line buffer (4096 characters) is not strained by this.

## N2 [P3 · Hook defense] MyFillRect: null check on lprc

- **Location**: DLL dllmain.cpp MyFillRect
- **Problem**: the FillRect contract requires lprc to be non-null, but this function is a global hook; if some caller violates
  the contract and passes NULL, and the original function merely returns 0 on that path, the subsequent `lprc->left` becomes a
  dereference crash point newly introduced by the DLL.
- **Fix**: `if (savedDC)` → `if (savedDC && lprc)` — the hook must be more conservative than the original function.

## N3 [Design trade-off · awareness only, unchanged]

BitmapGDI keeps each image's full original bytes resident; the atlas's total memory has no cap (N images = N × file size).
There is a 256MB per-file cap + a bad_alloc backstop, so it does not crash, it only consumes memory. Keeping the status quo: adding a total soft cap
would refuse loads or force on-demand decoding, and the latter sacrifices the thread-safety model — not worth the cost; defer until real
memory pressure actually appears.

---

## Four-round audit convergence curve (basis for the wrap-up)

| Round | Auditor | P0 | P2 | P3 | Nature of findings |
|---|---|---|---|---|---|
| v1.3.20 | Deepseek | 1 | many | — | Deterministic bug (blank button) + a real regression |
| v1.3.21 | Deepseek re-review | 0 | 1 | 3 | 1 substantive hardening + comment/magic-number consolidation |
| v1.3.22 | Kimi | 0 | 1 | 4 | All null-check-type memory boundaries |
| v1.3.23 | Hunyuan 4 | **0** | **0** | 3 | 2 one-line defenses + 1 awareness item |

P0/P2 declined monotonically to double zero; each round's re-check confirmed the previous round's fixes had no regression and broke nothing — none of the "fix A, break B"
loop signature; the independent audit conclusions of four different models corroborate one another. Judged converged; the audit loop ends here;
future audits only when new features/new-version changes occur.

## Verification record

| Item | Result |
|---|---|
| Compilation | DLL/EXE zero errors, only pre-existing benign warnings |
| Byte signature | DLL/EXE contain v1.3.24, zero v1.3.23 residue; EXE-embedded DLL byte comparison confirmed as the new v1.3.24 version |
| test_geometry / test_render / test_filter | 227/227, 116/116, all pass |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | all pass / all pass / pass / 5 rounds zero errors / all pass |
| test_imageheader | 13/16 (verbatim identical to the v1.3.20/21/22/23 historical versions; the ImageHeader.h diff vs the previous version is empty) |

## Fingerprints

- ExplorerBgTool.dll: **936,501 bytes**, SHA256 `481EE8B899D4D68BEBFAE58E16731A2FEF1B579DD380CEEBF17639AE5A8FBFC5`
- backimg图形板_v1.3.24.exe: 1,251,835 bytes, SHA256 `8112533E6AB2039B054033C630596B15D3694020C5A00104D3696103C5611DB3`
