# BasicImaging Migration Plan

Migrate image-handling code in `src/Services/` onto the shared `src/BasicImaging` library
("use the new BasicImaging module where applicable").

## Context

- BasicImaging was built for exactly this: safe thumbnailing / JPEG-PNG-PZP encode-decode for every
  Service, with a SIMD resize (SSE3/AVX2, ~7-8x scalar, byte-identical to the scalar reference —
  verified by `testbasicimaging` and the benchmark's fidelity pass).
- Audit of all 20+ services: only **HabChan** and **SuperMarket** have file→file image pipelines
  (both shell out to ImageMagick `convert`); **ImageGeneration** has one `convert` center-crop-fill
  step; the in-memory pixel→JPEG encodes route through `AmmCaptcha_getJPEGFileFromPixels` (3 sites:
  MyRemoteDesktop x2, V4L2ToHTTP) and `AmmCaptcha_getCaptchaFrame` (2 sites: MyURL + HabChan
  captchas). MyTube is video (ffmpeg — out of scope); MySearch/ShareTex/WebFramebuffer/MyLoader do
  no image file work.
- User decisions: migrate HabChan + SuperMarket + the in-memory JPEG pipeline (new BasicImaging
  memory-encode API; AmmCaptcha is the single touchpoint so its call sites stay unchanged) +
  implement EXIF orientation support in BasicImaging + move center-crop-fill ("cover") into
  BasicImaging and use it in ImageGeneration. HabChan GIF thumbnails degrading to a verbatim
  full-size copy is accepted (no GIF codec).

## Key verified facts

- `struct Image` (codecs/image.h) fields: `pixels,width,height,channels,bitsperpixel,image_size`.
  **AmmCaptcha has its own incompatible `struct Image`** (`pixels,width,height,depth,imageSize` —
  src/AmmCaptcha/imaging.h:4-11) — the two headers can never be included in one TU, so AmmCaptcha's
  must be unified onto BasicImaging's (Step 4).
- Vendored `WriteJPEGInternal(filename,pic,mem,&mem_size,quality)` (codecs/jpgInput.c:302) already
  supports a caller-buffer destination — **but has two real bugs that make the memory path unusable
  as-is**: (a) an early `if (filename==0) return 0;` guard, which its own `WriteJPEGMemory` wrapper
  triggers (dead path); (b) `empty_buffer()` (line 38) returns TRUE without resetting the buffer
  pointers, so a too-small target buffer makes libjpeg keep writing past the end (silent memory
  corruption; today unreachable only because all callers over-size buffers). Both fixed in Step 1.
- AmmCaptcha links `jpeg` directly and its consumers resolve it transitively (proven mechanism in
  this build), so linking BasicImaging into AmmCaptcha propagates to V4L2ToHTTP/MyRemoteDesktop/
  MyURL/HabChan/MyLoader with zero CMake edits there. BasicImaging links only
  jpeg/png/zstd/lz4/m — no circular dependency.
- `codecs' createImageBitBlt` is hardcoded to 3 channels and spams stderr — CoverResize uses a new
  internal channel-generic crop helper instead.

## Steps

### 1. `BasicImaging_SaveJPEGToMemory` + fixing the vendored memory writer

**1a. `src/BasicImaging/basicImaging.h`** — add after `BasicImaging_SaveJPEG`, doc-comment style of
the siblings:

```c
int BasicImaging_SaveJPEGToMemory(struct Image * img,unsigned char * target,
                                  unsigned long targetMaxSize,unsigned long * bytesWritten,
                                  unsigned int quality);
```

Writes into a **caller-provided** buffer (matches the `rqst->content` pattern — no extra copy),
returns 1 on success, 0 on bad args / no JPEG support / output larger than `targetMaxSize` (buffer
left untouched). Never overflows, never crashes.

**1b. `src/BasicImaging/basicImaging.c`** — implementation after `BasicImaging_SaveJPEG`:
clamp quality 1-100, `*bytesWritten = targetMaxSize;` then
`return WriteJPEGInternal(0,img,(char*)target,bytesWritten,(int)quality);` under `#if USE_JPG_FILES`
(else log + return 0, same pattern as SaveJPEG).

**1c. `src/BasicImaging/codecs/jpgInput.c`** — three surgical fixes to `WriteJPEGInternal`:
- Fix the dead memory path: change the early guard to reject only when BOTH modes are unusable:
  `if ( (filename==0) && ((mem==0) || (mem_size==0)) ) { return 0; }` (memory mode never touches
  filename; file mode never passes a null filename).
- Make the memory path overflow-safe: `empty_buffer` returns 0 (FALSE → libjpeg raises
  JERR_CANT_SUSPEND instead of writing past the buffer). Because libjpeg's default error handler
  would `exit()` on that, install the canonical libjpeg setjmp guard from libjpeg's own
  `example.c` (`struct jpegErrorGuard { struct jpeg_error_mgr pub; jmp_buf jump; }` +
  `error_exit → longjmp`, `setjmp` → `jpeg_destroy_compress` + return 0) **only in the
  `(mem!=0)&&(mem_size!=0)` branch**; the file path keeps `jpeg_std_error`/`jpeg_stdio_dest`
  untouched. The jmp_buf is per-call stack state → thread-safe. (Note: unlike the PZP case, where
  return codes were the natural fix, libjpeg's `error_exit` contract forbids returning — longjmp
  is the pattern libjpeg's own documentation prescribes for memory destinations.)
- `guard` declared at function top (C89 style).

**1d. Tests** — see Step 7 (the too-small-buffer case exercises the setjmp guard).

### 2. EXIF orientation in `BasicImaging_Load`

**2a. Parser — `src/BasicImaging/basicImaging.c`**, two static helpers above `BasicImaging_Load`:

- `static int readEXIFOrientation(const char * filename)` — open/read file; verify `FF D8` (SOI);
  walk JPEG segments from offset 2 (skip `FF00` fill bytes and standalone markers TEM/RSTn; stop at
  EOI/SOS): find `FFE1` APP1 with payload `"Exif\0\0"`, then parse the TIFF block: byte order
  II/MM, magic 42, IFD0 offset, iterate entries for tag `0x0112` (type SHORT, count 1) → return
  1-8, else 0. Fully bounds-checked, never crashes; returns 0 for non-JPEG/no-Exif/malformed.
  Helper byte readers `exifGet16/exifGet32` with endianness flag.
- `static struct Image * rotateImageForOrientation(struct Image * img,int orientation)` — allocate
  `BasicImaging_New(newW,newH,channels)` and permute per the standard table (dims swap W↔H for
  orientations 5-8):
  | o | meaning | src x | src y |
  |---|---|---|---|
  | 2 | flip h | W-1-x | y |
  | 3 | rot 180 | W-1-x | H-1-y |
  | 4 | flip v | x | H-1-y |
  | 5 | transpose | y | x |
  | 6 | rot 90 CW | y | H-1-x |
  | 7 | transverse | W-1-y | H-1-x |
  | 8 | rot 270 CW | W-1-y | x |
  Return the new image (caller frees the original).

**2b. Wire into `BasicImaging_Load`** after the `pixels==0` bail-out: if orientation ≥ 2, rotate;
on allocation failure keep the unrotated decode (graceful). `ThumbnailFile` uses `Load` internally
→ SuperMarket/HabChan get auto-orient automatically. Re-encoded JPEGs carry no metadata
(= convert's `-strip`). Extend `BasicImaging_Load`'s doc comment in the header.

### 3. `BasicImaging_CoverResize` (center-crop-fill)

**3a. `src/BasicImaging/basicImaging.h`** — add after `BasicImaging_Thumbnail`:

```c
struct Image * BasicImaging_CoverResize(const struct Image * img,unsigned int newWidth,unsigned int newHeight);
```

**3b. `src/BasicImaging/basicImaging.c`**:
- `static struct Image * cropImageRegion(src,x,y,w,h)` — channel-generic row-memcpy crop into a
  fresh `BasicImaging_New` (deliberately NOT codecs' `createImageBitBlt`: 3-channel-hardcoded +
  stderr spam). Bounds-checked.
- `BasicImaging_CoverResize`: `scale = max(newW/srcW, newH/srcH)` (fills the box, never distorts;
  upscales when the source is smaller), `resized = BasicImaging_Resize(img, scaledW, scaledH)`
  (gets the SIMD paths), then `cropImageRegion` centered (`(resizedW-newW)/2`, `(resizedH-newH)/2`)
  → exactly newW x newH. Free the intermediate. This reproduces ImageMagick's
  `-resize 512x512^ -gravity Center -extent 512x512`.

### 4. AmmCaptcha — struct unification + memory API + orphan removal

**4a. `src/AmmCaptcha/imaging.h`** — delete the local `struct Image` (lines 4-11), replace with
`#include "../../BasicImaging/basicImaging.h"` + comment. (`basicImaging.h` pulls only
`codecs/image.h`, not `codecs.h`, so no `createImage` declaration clash.)

**4b. `src/AmmCaptcha/imaging.c`** — field-name updates: `depth`→`channels` + set `bitsperpixel`
(`depth*8`) and `image_size` (`width*height*depth`) in `createImage` (35-37);
`createImageUsingExistingBuffer` (22): `channels=channels; bitsperpixel=bitsPerPixel; image_size=...`;
`copyImage` (56,59) `depth`→`channels`; `ReadPPM` (191,197) `depth=3`→`channels=3; bitsperpixel=24;`
and `imageSize`→`image_size`. `bitBltImage`/`WritePPM`/`destroyImage` use only width/height/pixels
— unchanged. Compile errors will catch any missed field.

**4c. `src/AmmCaptcha/main.c`** — drop `#include "jpgInput.h"`; migrate the two encode paths onto
the new API (stack `struct Image` borrowing the caller's pixels — zero-copy preserved):
- `AmmCaptcha_getCaptchaFrame` (line 84): `WriteJPEGMemory(captcha,mem,mem_size)` →
  `unsigned long written=*mem_size;` + `BasicImaging_SaveJPEGToMemory(captcha,(unsigned char*)mem,
  *mem_size,&written,90)` → `*mem_size=written;` and set `success=1` only when it returns 1
  (today the return is ignored).
- `AmmCaptcha_getJPEGFileFromPixels` (98-133): keep all validation; replace the
  `createImageUsingExistingBuffer`/`WriteJPEGMemory` body with a memset stack struct
  (`pixels,width,height,channels,bitsperpixel=channels*8,image_size=width*height*channels`) +
  `BasicImaging_SaveJPEGToMemory(...,90)` → `*mem_size=written;` return 1 / log + return 0.
- `testAmmCaptcha` (214): `WriteJPEGFile(captcha,"captcha.jpg")` → `BasicImaging_SaveJPEG(captcha,
  "captcha.jpg",75)` (same hardcoded quality).

**4d. Delete `src/AmmCaptcha/jpgInput.c` + `jpgInput.h`** (orphaned after 4c — verified no other
references).

**4e. `src/AmmCaptcha/CMakeLists.txt`** — remove jpgInput.c/h from `AMMCAPTCHA_INGREDIENTS`;
`target_link_libraries(AmmCaptcha BasicImaging m Hashmap)`;
`target_link_libraries(AmmCaptchaDynamic BasicImagingDynamic m)`; add
`add_dependencies(AmmCaptcha BasicImaging)` and `add_dependencies(AmmCaptchaDynamic BasicImaging)`
(AmmCaptcha is added before BasicImaging in the top-level CMakeLists). Drop the direct `jpeg` links
— it arrives transitively.

**4f. Consumer services (V4L2ToHTTP, MyRemoteDesktop, MyURL, MyLoader, HabChan) need no changes.**

### 5. HabChan

**5a. `src/Services/HabChan/state.c`** — include `../../BasicImaging/basicImaging.h`;
`generateThumbnail` (193-202) becomes
`return BasicImaging_ThumbnailFile(sourcePath,thumbPath,200,82);` with a comment on the preserved
best-effort contract (verbatim-copy fallback → GIF thumbs are the full-size file; thread.c:47
still serves them). `detectImageType` (155-178) and upload storage stay as-is.

**5b. `src/Services/HabChan/CMakeLists.txt`** —
`target_link_libraries(habchan pthread rt m AmmarServerDynamic AmmCaptcha BasicImaging )` +
`add_dependencies(habchan AmmarServer AmmCaptcha BasicImaging)`.

### 6. SuperMarket

**6a. `src/Services/SuperMarket/photo.c`** — include the BasicImaging header; add
`static const char * sniffImageType(const char * bytes,unsigned int size)` (magic bytes, JPEG
`FFD8FF` / PNG `89504E470D0A1A0A`, else 0 — mirrors HabChan's `detectImageType` but over the
in-memory upload). Rewrite the middle of `photoUpload_callback` (43-73):
- sniff → unknown type = `err="Μη έγκυρη εικόνα"` (keeps garbage uploads out, replaces convert's
  implicit validation; GIFs now rejected — approved change);
- temp input named `upload-<pid>-<rand>.<ext>` (real extension — `BasicImaging_Load` decodes by
  extension; the old `.tmp` suffix would always fail);
- `img = BasicImaging_Load(tmpIn)` (EXIF auto-orient from Step 2) → NULL = reject; `unlink(tmpIn)`;
- `thumb = BasicImaging_Thumbnail(img,1000)` (shrink-only fit = `-resize 1000x1000>`) → NULL =
  reject; `BasicImaging_SaveJPEG(thumb,tmpOut,82)` → 0 = unlink tmpOut + reject; success =
  `rename(tmpOut,destPath)` (fresh-file judgement, strictly better than the old FileExists hack).
- Cart/JSON handling (74-89) and `photoStream_callback` untouched; update the "via ImageMagick"
  doc comment in photo.h.

**6b. `src/Services/SuperMarket/CMakeLists.txt`** —
`target_link_libraries(supermarket pthread rt m AmmarServerDynamic BasicImaging )` +
`add_dependencies(supermarket AmmarServer BasicImaging)`.

### 7. ImageGeneration

**7a. `src/Services/ImageGeneration/main.c`** — include the header; replace the
`system("convert ... -resize 512x512^ -gravity Center -extent 512x512 ...")` block (165-170) with
`BasicImaging_Load(finalPath)` → `BasicImaging_CoverResize(img,512,512)` →
`BasicImaging_SaveJPEG(cover,"/home/user/workspace/upimage.jpg",92)` (92 = ImageMagick default
quality). Failure handling: log to stderr and continue exactly as today (downstream script still
runs) — minimal improvement, same flow.

**7b. `src/Services/ImageGeneration/CMakeLists.txt`** —
`target_link_libraries(imagegeneration pthread rt AmmarServerDynamic BasicImaging )` +
`add_dependencies(imagegeneration AmmarServer BasicImaging)`.

### 8. Library tests — `src/BasicImaging/testBasicImaging.c`

Three new test functions (existing CHECK pattern, JPEG-guarded), wired into `main()`:
- `testSaveJPEGToMemory` — 32x20 gradient into 64KB buffer → returns 1, `written` in range, `FFD8`
  magic; write buffer to /tmp + Load back → 32x20; **too-small buffer (64 bytes) → returns 0, no
  crash** (exercises the Step 1c guard); NULL-arg safety checks.
- `testCoverResize` — 200x100 → CoverResize(100,100) → 100x100, byte-exact match against
  src(50+x, y); 50x40 → CoverResize(100,100) (upscale+fill) dims check; 8x8x4 → 4x4 channels check.
- `testEXIFOrientation` — helper splices an APP1 segment right after SOI into a JPEG saved by
  SaveJPEG (little-endian TIFF: `FFE1 0022 "Exif\0\0" "II" 2A00 08000000 0100 12010300 01000000
  <orient>0000 00000000`); control (no EXIF) 32x20 unchanged; orientation-6 splice → Load returns
  20x32 and `o6(x,y) == control(y,19-x)` byte-exact (unique-pixel gradient distinguishes 6 from
  5/8 and no-op).

## CMake summary

| File | Change |
|---|---|
| src/AmmCaptcha/CMakeLists.txt | drop jpgInput.c/h; link BasicImaging / BasicImagingDynamic; add_dependencies; drop jpeg |
| src/Services/HabChan/CMakeLists.txt | + BasicImaging in target_link_libraries + add_dependencies |
| src/Services/SuperMarket/CMakeLists.txt | + BasicImaging in target_link_libraries + add_dependencies |
| src/Services/ImageGeneration/CMakeLists.txt | + BasicImaging in target_link_libraries + add_dependencies |
| V4L2ToHTTP / MyRemoteDesktop / MyURL / MyLoader | none (transitive via AmmCaptcha) |

Order: Steps 1-3 (library) → 4 (AmmCaptcha) → 5-7 (services) → 8 (tests) — Steps 2 and 3 are
independent of 1.

## Verification

1. **Build**: `cmake --build build` (whole tree, incl. AmmCaptcha + all touched services) — clean.
2. **Unit tests**: `./src/BasicImaging/testbasicimaging` — all existing byte-exact SIMD checks +
   the 3 new groups print OK (the too-small-buffer case is the setjmp-guard regression test).
3. **Benchmark sanity**: `./src/BasicImaging/benchmarkresize --iterations=5` — speedup + fidelity
   table still ~7-8x / byte-identical (guards the resize paths).
4. **Smoke tests** (services run standalone from their dirs):
   - HabChan: POST a JPEG with EXIF orientation 6 → `thumb_0.jpg` exists, ≤200px, correctly
     rotated; POST a GIF → `thumb_0.gif` is the verbatim full-size file (accepted);
     `curl .../captcha.jpg?id=0 | head -c 2` → `FFD8` (exercises AmmCaptcha memory path).
   - SuperMarket: photo upload via the supermarket.py CLI (multipart) → on-disk photo ≤1000px
     JPEG, no `error` in JSON; upload a text file → `"Μη έγκυρη εικόνα"`; EXIF-6 upload → stored
     photo dimension-swapped.
   - ImageGeneration: build only — `/home/user/workspace` paths are demo scaffolding.
   - V4L2ToHTTP / MyRemoteDesktop: not smoke-testable here (camera / X display); their JPEG path
     is the same AmmCaptcha code covered by the captcha smoke test + unit round-trip.
5. Nothing is committed; all changes stay in the working tree.

## Risks / notes

- AmmCaptcha struct unification is the biggest touchpoint — field renames are compile-checked;
  the old `depth` field actually held bytes-per-pixel in `createImageUsingExistingBuffer`, now
  correctly split into `channels` + `bitsperpixel`.
- The vendored `jpgInput.c` gets 2 behavioral fixes (dead memory path + overflow guard) — both are
  bugs today; the file path is untouched by the guard change. The setjmp here is the canonical
  libjpeg memory-destination pattern (its `error_exit` cannot return), unlike the PZP case where
  return codes were the right fix.
- ImageGeneration now decodes by filename extension (convert sniffed content): uploads whose name
  lacks `.jpg/.jpeg/.png` fail to decode and are logged. If content-sniffing parity is wanted
  later, reuse photo.c's `sniffImageType` with a scratch path (~10 lines).
- HabChan sub-200px images are no longer upscaled by the thumbnailer (IM did); HTML `height="200"`
  rendering is unchanged. Thumb quality 92→82 (IM default vs house style) — minor.
- SuperMarket rejects GIFs now (IM took frame [0]) — approved.
- `AmmCaptchaTester` is a Code::Blocks project outside CMake — after Step 4 it needs BasicImaging
  linked if rebuilt manually; not built by the top-level CMake, no action.
