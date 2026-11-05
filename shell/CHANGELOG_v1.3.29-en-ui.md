# backimg-gui v1.3.29 — English UI Edition Changelog

**Release date**: 2026-11-06
**Scope**: the English shell itself is unchanged except version stamps — **v1.3.29 is a DLL-level release**. The bundled `ExplorerBgTool.dll` is upgraded to v1.3.29 and paired with the shell via SHA256, as always.

## What changed in the DLL (v1.3.28 → v1.3.29)

### The headline: first-open lag eliminated

Opening any File Explorer folder window after boot used to stall 1–2 seconds with a large image library (the scanned library was fully read into memory: ~1.85 GB of disk I/O + hashing + resident RAM for 148 images). v1.3.29 reworks the scan pipeline:

- **L1 — header-only scan**: each file's first 256 KB is read instead of the whole file (JPEG skips the trailing-EOI check, PNG validates IHDR then OK; files ≤256 KB keep the strict full-buffer semantics). Scan I/O drops from ~1.85 GB to ~38 MB.
- **L2 — light fingerprint**: the cache key is now `hash(header) ^ (file size × prime)` instead of a full-content hash.
- **L3 — on-demand decode**: the resident raw buffer is gone; decoding reads the file only when it is actually drawn (`Bitmap::FromFile`, lazy). Resident memory drops by ~1.85 GB. Side benefit: replacing an image file takes effect on the next rotation without restarting Explorer.
- **D1 — background preheat**: a low-priority thread starts the moment Explorer loads the DLL and does the scan off the UI thread; the window thread uses `try_lock` and never waits for the scan.
- **D3 — repaint on readiness**: windows opened before the scan finished repaint automatically once the image list is ready.

### Audit hardening (DeepSeek V4.1 Flash, rounds R2–R6; four-question guardrails applied each round)

- **S1 tail-terminator check**: partially-read large files get their trailing bytes verified (JPEG `FF D9` / PNG `IEND`) — truncated images that GDI+ silently renders with black bands are rejected before entering the rotation list.
- **R4-1**: reverse search fixes false positives from JPEGs with embedded EXIF thumbnails (their thumbnail carries its own `FF D9`).
- **R5-1 / R6**: legitimate payload files (Motion Photos — JPEG followed by an MP4, common in phone backups) are rescued via a container-signature check (`ftyp`/`PK`/`RIFF`/`SEFH`/secondary JPEG/PNG); every rejected file is logged to `plugin\ExplorerBgTool_init.log` (`[扫描剔除]` lines) so nothing disappears silently.
- Dead-file marking with a 5-minute TTL auto-revival; preheat thread gated to Explorer only (regsvr32 no longer spawns it); list-rebuild condition upgraded to path+size+mtime (same-name image replacement no longer stretches).

### Known boundaries (explicitly recorded)

- Unknown trailing data > 4 KB is still rejected (diagnosable via the log; re-saving the image clears it). Motion Photos with known container signatures are rescued.
- BMP/GIF/TIFF have no reliable terminator and are not tail-checked.
- The first frame still pays a one-time decode+scale (~0.3–0.8 s for a large image); async first-paint is a candidate for a future release.

The full Chinese changelog with the complete audit trail lives on the `main` branch: `dll/changelog/变更清单_CHANGELOG_v1.3.29.md`.

## Shell-side changes in this release

- Version stamps: `GUI_VERSION` / `DLL_VERSION` / `FILEVERSION` / `ProductVersion` → 1.3.29.
- Bundled DLL → v1.3.29 (SHA256 pairing check unchanged).
- No UI text or layout changes; all EN-0~7 fixes from v1.3.28 carry over as-is.
