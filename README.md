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
