# backimg Aspect-Ratio-Preserving Scaling Edition v1.3.16 · Changelog

- **Release date**: 2026-10-27
- **Baseline**: v1.3.15 (fixed edition line)
- **Core requests** (user's original words):
  1. Background images are no longer subject to the "must be smaller than the window" restriction — at any window size (fullscreen/non-fullscreen/while dragging), the image never goes out of bounds and never distorts;
  2. **Except for the last mode, "scale and fill", all other display-position modes must scale the image proportionally at its original aspect ratio, with no distortion**.

---

## I. Problem Localization (measured conclusions, not speculation)

Before the rework, a probe program was used to measure all three technical routes. The conclusions:

| Route | Can scale | Interpolation quality | alpha channel | Usable as AlphaBlend source | Conclusion |
|---|---|---|---|---|---|
| `AlphaBlend` direct scaling | Yes | **Nearest neighbor** (4x4→40x40 yields solid color blocks) | Fully preserved | — | Full-screen mosaic when enlarged, **unusable** |
| `StretchBlt` pre-scaling | Yes | HALFTONE high quality | **All zeroed out** (24000/24000 pixels alpha=0) | **No** (AlphaBlend returns 0) | Transparent background turns into black blocks, **unusable** |
| `GDI+ DrawImage + GetHBITMAP` | Yes | **High-quality bicubic** | Fully preserved | **Yes** (returns 1) | **Adopted** |

The old DLL had **no aspect-ratio-preserving scaling implementation whatsoever** — posType=5/6 merely computed the target rectangle;
the actual scaling was nearest-neighbor interpolation done by `AlphaBlend` itself. posType=0~4 simply pasted the image at its original pixel size,
so any large image overflowed the window.

---

## II. Changes in v1.3.16

### 2.1 New `DrawScale.h` (pure geometry module, independently unit-testable)

All decisions about "how large the image is drawn and where" are consolidated into **one pure function**, `ComputeDrawRect()`:

```
ComputeDrawRect(mode, imgW, imgH, winW, winH) -> DrawRect{x,y,w,h}
```

Benefits:
- The hard constraint "any scaling preserves aspect ratio" has **a single point of implementation** and cannot be computed wrong separately in scattered branches;
- Pure computation, no Win32 dependency, so unit tests can exhaustively verify it.

**Seven new mode semantics**:

| posType | Name | Scaling rule | Position |
|---|---|---|---|
| 0 | Top-left | Aspect-ratio-preserving scaling (shrunk if it doesn't fit, never enlarged) | Anchored top-left |
| 1 | Top-right | Same as above | Anchored top-right |
| 2 | Bottom-left | Same as above | Anchored bottom-left |
| 3 | Bottom-right | Same as above | Anchored bottom-right |
| 4 | Centered | Same as above | Centered |
| 5 | Scale | **Aspect-ratio-preserving scaling to fully fit the window** (follows the window in both dimensions) | Centered |
| 6 | Scale and fill | Aspect-ratio-preserving fill of the window (short-side overflow cropped) | Centered — **unchanged, as required** |

> Note: 0~4 keep the old look of "placed at original size" (small images are not enlarged, preserving sharpness);
> the only change is **automatic proportional shrinking when the image doesn't fit**, so it no longer overflows the window.
> If you want "the image scaling up and down in real time with the window", choose **5 (Scale)** or 6 (Scale and fill).

### 2.2 Scaling basis changed from "window rectangle" to "client area"

The old version used `GetWindowRect()` — that is the **window rectangle including borders**, which overstates the drawable area,
so the scaled image could exceed the actually visible area. Now uses `GetClientRect()`, the same coordinate system as the DC used for drawing.

### 2.3 Redraw decision on window size change corrected

The old condition was `size changed && posType != 0` before redrawing — in "top-left (0)" mode, window changes did not trigger a redraw.
But starting from v1.3.16, **all modes**' drawing results depend on the window size, so this exception was removed;
any mode redraws after a size change, leaving no ghost of the old size.

### 2.4 Scaling rendering switched to GDI+ high-quality interpolation (`BitmapGDI::GetScaled`)

Pipeline: GDI+ `DrawImage` (bicubic interpolation + `CompositingModeSourceCopy` to preserve transparency)
→ `GetHBITMAP` → memory DC → `AlphaBlend` **1:1** paste.

Because the source rectangle and the target rectangle are the same size, **the paste stage involves no interpolation at all**, hence no stretching distortion.

### 2.5 Scaling cache (per-thread private + LRU)

- Same image + same target size = reuse the same scaled result; no repeated scaling while dragging the window;
- 32MB budget per thread, LRU eviction when exceeded;
- **Per-thread private** (`thread_local`); GDI handle creation/use/destruction all happen on the same thread.

---

## III. Pitfalls Encountered (all written into code comments, so future maintainers won't repeat them)

### Pitfall 1: `AlphaBlend`'s source DC must be one without a palette

Measured: source DC created with `CreateCompatibleDC(NULL)` → `AlphaBlend` returns **1** (success);
created with `CreateCompatibleDC(GetDC(NULL))` → returns **0** (immediate failure, background not drawn).
All DCs carrying scaled results in the DLL uniformly use `CreateCompatibleDC(NULL)`.

### Pitfall 2: The source rectangle must fall entirely within the source bitmap

Measured: source rectangle larger than the source bitmap → `AlphaBlend` returns **0**.
So the paste stage is strictly 1:1 (source rectangle = target rectangle = scaled size).

### Pitfall 3: `SelectObject(dc, NULL)` permanently ruins that bitmap's alpha compositing capability

Measured: after a bitmap is deselected from a DC via `SelectObject(dc, NULL)`, selecting it back into any DC
makes `AlphaBlend` always return 0. The correct way is `DeleteDC` then `DeleteObject`;
do not deselect the bitmap with `SelectObject(dc, NULL)`.

### Pitfall 4: GDI+ Bitmap objects must not be shared across threads

Measured: 8 threads sharing the same GDI+ `Bitmap` for `DrawImage` segfaulted in stress-test round 6;
**a single thread passed all 40 rounds of the same stress test**. GDI+ internal state is not thread-safe.

**Root fix**: the source bitmap is now **decoded once per thread** (`ScaleCache::GetSource`);
zero GDI+ objects shared between threads. `BitmapGDI` only retains the original file bytes and no longer holds GDI+ resources.

### Pitfall 5: `CreateDIBSection`'s DC parameter must be released

Measured: `CreateDIBSection(GetDC(NULL), ...)` takes a screen DC each time without calling `ReleaseDC`,
leaking one DC handle per scaling; under high-frequency scaling, GDI resources are exhausted and it crashes.
(This was the hardest one to localize during troubleshooting: the symptom was "crashes after a few rounds", unrelated to concurrency.)

---

## IV. Verification Record

| Test | Content | Result |
|---|---|---|
| Geometry unit tests | 8 image/window size groups × 7 modes, exhaustively checking aspect-ratio preservation, no window overflow, valid positions, real-time following, degenerate inputs | **227/227 passed** |
| Rendering end-to-end | Loaded a real user PNG, through the full scaling pipeline, verifying alpha preservation + visual parity with the old version | Old/new average pixel error **2.17**, alpha error **0.00** |
| Window following | Real screenshots at four window sizes: 640×480 / 1024×768 / 1440×900 / 1920×1080 | Image **always aspect-ratio-preserving, never overflowing**, scaling up step by step with the window |
| Concurrency stress test | 8 threads × 300 iterations/round × 40 rounds, with sizes jittering to force LRU eviction | **40/40 passed, zero crashes** |

Measured rendering output images are in the delivery package's `验证输出/` directory:
- `follow_m5.png` — real captures of "Scale" mode at four window sizes (aspect-ratio-preserving, no overflow, scaling up step by step)
- `all_modes.png` — real capture comparison of the seven modes in the same window

---

## V. Known Boundaries

- **Modes 0~4 never enlarge the original image** (small images keep their original size, preserving sharpness); use 5 or 6 if you want enlargement with following.
- Each thread holds its own decoded source bitmap per image. explorer typically has only 1~3 window threads,
  so memory usage is negligible (user-measured 53 images, about 500KB each).
- Scaling cache is capped at 32MB per thread, with LRU eviction when exceeded (a momentary re-scaling cost).
- Replacing the contents of a same-named image still requires restarting explorer to take effect (same behavior as v1.3.15).
- `folderExt` (file dialog background image) remains unsupported across the entire chain in this edition (same as v1.3.15).
