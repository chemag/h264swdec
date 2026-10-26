# h264swdec: A Small H264 decoder for constrained RTC streams.


# 1. h264swdec

h264swdec is a small-footprint H264 software decoder oriented to
real-time communications (RTC) in low-resource devices (e.g. embedded
systems). The goal is to be able to decode video produced by Chrome
or OpenH264 after a successful WebRTC negotiation and RTP reassembly.

The objective is not a general-purpose H264 implementation. RTC
streams use a narrow subset of the normal video tools:

* Constrained Baseline or compatible Baseline profile.
* CAVLC entropy coding.
* I and P slices, but no B slices.
* Progressive frame coding, with no field coding or MBAFF.
* A small decoded-picture buffer and limited reference pictures (typically
only one).
* Raw I420 output for the RTC pipeline.
* Annex B access units after RTP FU-A or STAP-A reassembly.


# 2. A New Software H264 Decoder

We prefer to reuse some already-existing H264 decoder, and adapt it for
our goals, than to write it from scratch. We considered several options.

The [`oneam/h264bsd`](https://github.com/oneam/h264bsd) project is very
close to what we want. In fact, we started just adding fixed to it. The
decoder started with an old version of the old Android software H264
decoder (called "H264SwDec" or "h264bsd" in the Android repo), forked
it, and added some patches on top. We are copying their idea of
extracting the android decoder into a standalone library.

H264SwDec is a good starting point for this constrained use case: It
has a compact C implementation, a simple decoder API, native I420
output, and the Baseline-profile tools required by the expected RTC
stream. Its canonical API distinguishes between a decoded picture
and a picture ready for display. `H264SwDecNextPicture()` returns
`H264SWDEC_OK` when no display picture is available and
`H264SWDEC_PIC_RDY` when one is ready. It also supports flushing
delayed pictures from the decoded-picture buffer.

In fact, `oneam/h264bsd` did exactly this. I started with an initial
import of H264SwDec in 2013. It later added approximately 100 commits,
but only a small minority touched the native decoder.

Most of the work included in `oneam/h264bsd` is of no use for our goals.

* Adding/maintaining Emscripten, JavaScript, WebAssembly, Node, Flex,
and browser wrappers.
* Adding iOS, Windows, and POSIX applications.
* RGBA, BGRA, and YCbCrA conversion APIs and conversion buffers.
* Canvas and GPU rendering.
* Generated artifacts and multiple build systems.

Moreover, we checked the decoder compliance using the AVC CTS
dataset (avcv1). The results are poor: The decoder achieved 26
exact official-reference matches out of 135 AVCV1 vectors:
19.3% normative compliance. In particular, we noticed it
SIGSEVs with chrome RTC content.

One of the possible issues is that the `oneam/h264bsd` fork is
much older (2013) than the latest Android H264SwDec version (2018).
While some of the fixes from Android were cherry-picked, we would
need to go through the Android repo and cherry-pick all fixes. As
such, it seems a better idea to start with the latest Android
H264SwDec version (before it was removed in Android 9.0), and then
apply the interesting ideas from `oneam/h264bsd`.

What we want to retain from the `oneam/h264bsd` decoder is mostly
its integration ideas. This includes the POSIX test application,
standalone build approach, and the cropping use. Other useful core
parts are the native cropping metadata and display-order output API.
Cropping is relevant to RTC because coded frame sizes are
macroblock-aligned even when the visible resolution is not.

Once we decided to start adapting a SW decoder for embedded RTC use
from scratch, why not consider H264SwDec replacement, libavc? AOSP
introduced `libavc` in 2015 (Android 6.0) as its newer software H264
implementation. It coexisted with H264SwDec for several Android releases,
and became the maintained software codec after H264SwDec was removed.

`libavc` is substantially broader than this project requires:

* It contains both an H264 decoder and encoder.
* Its decoder advertises Baseline, Main, and High profiles through Level 5.2.
* It implements both CAVLC and CABAC.
* It supports I, P, and B slices.
* It includes multi-core decoding.
* It has architecture-specific ARM, ARM64, x86, x86-64, and historical MIPS
  paths.
* It carries a larger API, more state, and more integration complexity.

Those capabilities are valuable for a general Android media stack, but do not
help a constrained RTC decoder that negotiates Baseline video without B-frames.
They increase the code, data, testing, and maintenance footprint that must be
brought onto a constrained target.

The decision is therefore to harden H264SwDec for the actual RTC bitstream
rather than adopt a much broader codec implementation.


# 3. Planned development

Our work will include the following:

* We will start with a pristine copy from the latest H264SwDec code before
it was removed from Android. After that, we will convert it into a standalone
library, while keeping each change reviewable:
* Remove Android, Stagefright, OMXDL, and unused architecture-specific code.
* Add a portable native build.
* Add a POSIX decoder and targeted tests.
* Add configurable allocation hooks.
* Run the AVCV1 conformance campaign against the untouched imported decoder.
* Fix correctness and undefined-behavior findings one issue per commit.
* Add Zephyr and RTC integration.
* Qualify actual Chrome and OpenH264 streams, including packet loss, IDR
  recovery, cropping, SPS or PPS changes, and resolution changes.

The acceptance target is reliable decoding of the negotiated RTC subset, not
complete support for every H264 profile and coding tool.


# 4. Compliance

h264swdec is compliant with the H264 Baseline and Constrained Baseline
profiles, as measured by the [H.264.1 AVCV1 conformance suite (2016
release)](http://handle.itu.int/11.1002/2000/9486).

| Profile              | AVCV1 vectors | Bit-exact | Failures |
|----------------------|---------------|-----------|----------|
| Baseline             | 40            | 40        | 0        |
| Constrained Baseline | 37            | 37        | 0        |

Constrained Baseline is the H264 profile required by WebRTC (RFC 7742,
`profile-level-id=42e01f`) and produced by Chrome through OpenH264. It is
Baseline without flexible macroblock ordering (FMO), arbitrary slice order
(ASO), and redundant slices. Of the 40 Baseline vectors, 37 use none of those
tools, and 3 use FMO.

* 31 vectors match the official reference YUV byte for byte. Two conformance
  packages ship an alternate reference: `CVFC1_Sony_C_nogray.yuv` is the
  cropped output, and `CVSE2_Sony_B-norpt.yuv` outputs each decoded frame once
  instead of repeating frames as signaled by SEI `pic_struct`. Those are the
  matching references for `AVCFC-1` and `AVCSE-1`.
* 9 vectors ship no reference YUV. Their output is byte-identical to FFmpeg's
  native H264 decoder, which matches the official reference on all 28
  Constrained Baseline vectors that have one.
* The vectors cover levels 1.0 through 3.1, at CIF resolution or smaller.

The other 95 AVCV1 vectors use the Main or Extended profile (CABAC, B slices,
interlaced coding, weighted prediction, or SP slices). They are outside this
decoder's scope. The decoder rejects 93 of them with `H264SWDEC_STRM_ERR`
instead of producing partial output, and 2 that use no unsupported tool decode
bit-exactly.

All 135 vectors decode with no crash, timeout, or AddressSanitizer or
UndefinedBehaviorSanitizer report. The decoder is also fuzzed with libFuzzer;
see `fuzz/README.md`.


# 5. Memory Sizing

The Zephyr port allocates all decoder memory from one static heap, sized at
build time by `H264SWDEC_ZEPHYR_HEAP_BYTES`. The heap size is not derived from
a profile level. It must cover the largest stream the application accepts, and
there is no per-call or per-stream setting.

| Configuration                     | `H264SWDEC_ZEPHYR_HEAP_BYTES` |
|-----------------------------------|-------------------------------|
| Zephyr port default               | 3,145,728 (3 MiB)             |
| RT1170 WebRTC demo (`03_libpeer`) | 393,216 (384 KiB)             |


## 5.1. What the Decoder Allocates

On a 32-bit Cortex-M target, the decoder allocates approximately:

```
heap_bytes = 7,372 + 220 * N + (D + 1) * (384 * N + 47) + 16 * (D + 1)
```

where:

* `N` is the picture size in macroblocks: `ceil(width / 16) *
  ceil(height / 16)`.
* `D` is the number of DPB frames. The decoder uses `max_dec_frame_buffering`
  from the SPS VUI when present. Otherwise it uses the level's maximum DPB
  size (`MaxDpbMbs` in Annex A) divided by the picture size, capped at 16.
* `384 * N + 47` is one I420 frame plus alignment padding. The DPB holds
  `D + 1` of them: `D` stored pictures plus the picture being decoded.
* `220 * N` is per-macroblock state, and 7,372 bytes is the decoder
  instance, one SPS with VUI, and one PPS.

The model matches the board: at 320 by 240 with `D = 1` it predicts 303,898
bytes, and the RT1170 measured 303,912 bytes in use during a 30-minute Chrome
call. The struct sizes come from `arm-zephyr-eabi-gcc -mcpu=cortex-m7`; a
64-bit host needs more for the pointer-bearing structures.

Most of the memory is the DPB, so `D` matters as much as the resolution.
Chrome's software H264 encoder, OpenH264, produced a one-frame DPB in the
30-minute call: at 320 by 240, Level 3.1 alone would allow `D = 16`, and only
`D = 1` matches the measured peak.


## 5.2. What the Current Values Cover

The demo's 393,216 bytes cover 320 by 240 only because the stream uses a
one-frame DPB. The value corresponds to a picture size and DPB depth, not to
a level:

| Heap size         | `D = 1`                 | `D = 2`       | `D = 4`       | `D = 16`     |
|-------------------|-------------------------|---------------|---------------|--------------|
| 393,216 (demo)    | 390 MBs, e.g. 320x240   | 281 MBs       | 180 MBs       | 57 MBs       |
| 3,145,728 (port)  | 3,176 MBs, e.g. 640x480 | 2,287 MBs     | 1,466 MBs     | 464 MBs      |

For example, 320 by 240 (300 macroblocks) does not fit the demo heap with
`D = 2`, and CIF (352 by 288, 396 macroblocks) does not fit even with
`D = 1`. Neither value covers 1280 by 720.

| Resolution | Macroblocks | `D = 1`   | `D = 2`   | `D = 4`   |
|------------|-------------|-----------|-----------|-----------|
| 320x240    | 300         | 303,898   | 419,161   | 649,687   |
| 352x288    | 396         | 398,746   | 550,873   | 855,127   |
| 640x360    | 920         | 916,458   | 1,269,801 | 1,976,487 |
| 640x480    | 1,200       | 1,193,098 | 1,653,961 | 2,575,687 |
| 1280x720   | 3,600       | 3,564,298 | 4,946,761 | 7,711,687 |


## 5.3. Sizing for a Level

To accept any conforming stream at a given level, size the heap for the worst
case over every picture size the level allows, assuming no VUI limit on `D`.
For each level the worst case is at its maximum frame size:

| Level | Max frame size (MBs) | Worst case `D` | Heap bytes | MiB   |
|-------|----------------------|----------------|------------|-------|
| 1.0   | 99                   | 4              | 219,547    | 0.21  |
| 1.1   | 396                  | 2              | 550,873    | 0.53  |
| 1.2   | 396                  | 6              | 1,159,381  | 1.11  |
| 1.3   | 396                  | 6              | 1,159,381  | 1.11  |
| 2.0   | 396                  | 6              | 1,159,381  | 1.11  |
| 2.1   | 792                  | 6              | 2,310,949  | 2.20  |
| 2.2   | 1,620                | 5              | 4,096,630  | 3.91  |
| 3.0   | 1,620                | 5              | 4,096,630  | 3.91  |
| 3.1   | 3,600                | 5              | 9,094,150  | 8.67  |
| 3.2   | 5,120                | 4              | 10,964,487 | 10.46 |
| 4.0   | 8,192                | 4              | 17,538,567 | 16.73 |

WebRTC's mandatory profile, `profile-level-id=42e01f`, is Constrained Baseline
Level 3.1. A heap that accepts any stream Chrome may send at that level
therefore needs about 9 MB. To extend the decoder to another level or
resolution:

1. Choose the largest stream to accept. Either take the worst case for its
   level from the table above, or, if the sender is known to cap the DPB
   through VUI, use the formula with that `D` and the largest resolution.
2. Add headroom for the allocator. Zephyr's `sys_heap` metadata was about 200
   bytes in the demo, but an SPS change frees and reallocates the DPB, which
   can fragment the heap.
3. Set `H264SWDEC_ZEPHYR_HEAP_BYTES`, and place the heap in memory large
   enough to hold it. On the RT1170 that means external SDRAM.
4. Confirm the peak on the target with `sys_heap_runtime_stats_get()` on
   `h264swdec_heap`. The demo logs it as `decoder_peak`.

To avoid sizing for a whole level, the receiver can instead limit what the
sender produces, by negotiating a lower level or a `max-fs` limit in the SDP
(RFC 6184).

If a stream needs more memory than the heap holds, the decoder fails cleanly
with `H264SWDEC_MEMFAIL` when it activates the SPS. It does not crash, but the
stream cannot be decoded. The decoder does not enforce the level's frame size
limit, so a non-conforming stream that exceeds its signaled level is bounded
only by the heap.



# Appendix 1. H264SwDec Upstream Provenance

The pristine import is the complete AOSP subtree at:

`media/libstagefright/codecs/on2/h264dec`

| Field               | Value                                                                |
|---------------------|----------------------------------------------------------------------|
| AOSP repository     | `platform/frameworks/av`                                             |
| AOSP tag            | `android-8.1.0_r81`                                                  |
| AOSP tag object     | `3bf07f0e33ec483012a8d62a24507599aa2dd244`                           |
| AOSP commit         | `0774f0bec35dfd108ab8195df9e659a7913a781b`                           |
| Imported subtree    | `0b4e5dd257c1a2141d5046f10b5d379b1bab922f`                           |
| Local import commit | `5825711f4f7d7d22eaaeffc96a81ecdd53fc25ad`                           |
| Archive SHA-256     | `9cc1fa186204dd2af1ac447da49467fa660f76e4b4f0b0d37d61247f5039f39f`   |

The upstream source is available at:

<https://android.googlesource.com/platform/frameworks/av/+/android-8.1.0_r81/media/libstagefright/codecs/on2/h264dec/>

Android 8.1 release 81 is the final Android 8.1 release tag. The H264SwDec
subtree is still present there and is unchanged from the first Android 8.1
release. The subtree is absent from Android 9.


The import commit contains the entire upstream subtree without cleanup or local
changes. In particular:

* Existing formatting and trailing whitespace are preserved.
* Android, Stagefright, assembly, OMXDL, and test-bench files are retained.
* Original file modes are preserved.
* No standalone build files or local documentation are in the import commit.

`git diff --check` reports pre-existing whitespace in upstream assembly and
source files. Those findings are expected and must not be fixed in the pristine
import. Cleanup belongs in later, reviewable commits.
