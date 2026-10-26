---
type: "reference"
title: "H264SwDec fuzzing"
description: "How to build, run, and triage the H264SwDec libFuzzer targets."
tags: ["h264swdec", "fuzzing", "libfuzzer", "sanitizers"]
author: "Chema Gonzalez"
generated_by: "Metamate"
date: "2026-10-26"
---

# 1. Overview

The fuzz targets drive the public decoder API with arbitrary bytes. Every
target and the decoder library are built with AddressSanitizer,
UndefinedBehaviorSanitizer, and libFuzzer coverage instrumentation.

The decoder must reject any input it cannot decode by returning an error. It
must never crash, read or write out of bounds, trigger undefined behavior,
hang, or return a picture whose buffer does not cover the frame it reports.

The structure follows the h264nal fuzzers: one small `LLVMFuzzerTestOneInput()`
per entry point, a corpus seeded from real bitstreams, and saved crash inputs
replayed as regression tests.

# 2. Targets

| Target                    | Input handling                                    |
|---------------------------|---------------------------------------------------|
| `h264swdec_annexb_fuzzer` | Whole input as one Annex B stream, then DPB flush |
| `h264swdec_rtc_fuzzer`    | One NAL unit per call, RTC recovery policy        |

`h264swdec_annexb_fuzzer` matches the POSIX decoder: output reordering is
enabled and the DPB is flushed at the end of the input.

`h264swdec_rtc_fuzzer` matches the RT1170 WebRTC receiver: output reordering
is disabled and each NAL unit is a separate `H264SwDecDecode()` call. After a
decoder error it resets the decoder, drops NAL units until the next IDR, and
replays the most recent SPS and PPS before that IDR.

`H264SwDecDecode()` removes emulation prevention bytes in place, so it writes
to `pStream`. Both targets decode from a private copy of each buffer, as a
caller that keeps the original bytes must do.

Both targets read every byte of each output picture and stop the process if
the decoder violates its API contract:

* `pStrmCurrPos` outside the input buffer on a successful call.
* A picture with a null buffer, or one output before any headers.
* Stream information with dimensions that are not whole macroblocks.
* A crop rectangle outside the coded frame.
* More output pictures than the DPB can hold.

`H264SwDecGetInfo()` reports the active SPS. When a new SPS arrives, it can
change before every picture of the previous sequence has been output, and
`H264SwDecPicture` carries no dimensions. The targets therefore cache the
stream information when the decoder returns
`H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY`, after draining the pictures still pending
under the old information, and size every output picture from that cache.
Callers must do the same.

# 3. Build

The fuzzers need Clang. They are off by default.

```
CC=clang cmake -S . -B build-fuzz -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DH264SWDEC_BUILD_FUZZERS=ON
cmake --build build-fuzz
ctest --test-dir build-fuzz --output-on-failure
```

`H264SWDEC_BUILD_FUZZERS=ON` instruments every target in the build, so the
regular unit tests also run under the sanitizers.

CTest runs each fuzzer once over the files in `test/data/` and
`fuzz/regressions/`, without mutation. This keeps fixed crashes fixed.

# 4. Run

Keep the corpus and crash artifacts outside the source tree. The H.264.1
conformance bitstreams are good seeds.

```
mkdir -p run/seeds run/corpus/annexb run/artifacts/annexb
cp /path/to/h264_conformance/avcv1/* test/data/*.h264 run/seeds/
cd run
../build-fuzz/fuzz/h264swdec_annexb_fuzzer corpus/annexb seeds \
    -fork=24 -ignore_crashes=1 -max_total_time=3600 -max_len=262144 \
    -timeout=10 -rss_limit_mb=2048 -artifact_prefix=artifacts/annexb/
```

New interesting inputs go to the first directory. Seeds are only read.
Use the same command with `h264swdec_rtc_fuzzer` and its own directories for
the RTC target.

To shrink a grown corpus:

```
../build-fuzz/fuzz/h264swdec_annexb_fuzzer -merge=1 corpus/annexb.min \
    corpus/annexb
```

# 5. Triage

Reproduce a crash by passing its artifact as the only argument:

```
build-fuzz/fuzz/h264swdec_annexb_fuzzer run/artifacts/annexb/crash-<sha1>
```

Minimize it before saving it:

```
build-fuzz/fuzz/h264swdec_annexb_fuzzer -minimize_crash=1 -runs=100000 \
    -exact_artifact_path=minimized.h264 run/artifacts/annexb/crash-<sha1>
```

For each fix:

* Fix one defect per commit.
* Add the minimized input to `fuzz/regressions/` with a descriptive name that
  ends in `.h264`. CTest picks it up automatically.
* Confirm that the input fails before the fix and passes after it.
