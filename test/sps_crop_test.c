#include <stdio.h>
#include <string.h>

#include "h264bsd_seq_param_set.h"
#include "h264bsd_stream.h"
#include "h264bsd_util.h"
#include "sps_writer.h"

/* 176x144: 11x9 macroblocks. In 4:2:0 frame coding a crop offset counts
 * two pixels, so each axis has 88 (horizontal) and 72 (vertical) units. */
#define WIDTH_MBS 11U
#define HEIGHT_MBS 9U

struct CropCase {
  const char* name;
  u32 crop[4]; /* left, right, top, bottom */
  u32 accepted;
};

int main(void) {
  static const struct CropCase cases[] = {
      {"no cropping", {0, 0, 0, 0}, 1},
      {"typical crop", {1, 1, 0, 4}, 1},
      {"two pixels left horizontally", {43, 44, 0, 0}, 1},
      {"empty width", {44, 44, 0, 0}, 0},
      {"two pixels left vertically", {0, 0, 35, 36}, 1},
      {"empty height", {0, 0, 36, 36}, 0},
      /* Offsets near 2^31 overflowed the signed check. */
      {"right offset 2^31 - 1", {0, 0x7fffffffU, 0, 0}, 0},
      {"bottom offset 2^31 - 1", {0, 0, 0, 0x7fffffffU}, 0},
      /* Offsets above 2^31 became negative in the signed check and were
       * accepted, producing a crop rectangle outside the picture. */
      {"left offset 2^32 - 2", {0xfffffffeU, 0, 0, 0}, 0},
      {"top offset 2^32 - 2", {0, 0, 0xfffffffeU, 0}, 0},
      {"left and right wrap to zero", {0x80000000U, 0x80000000U, 0, 0}, 0},
  };
  size_t index;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    const struct CropCase* c = &cases[index];
    u8 buffer[64];
    strmData_t stream;
    seqParamSet_t sps;
    u32 size = BuildSpsCropped(buffer, 10, 1, WIDTH_MBS, HEIGHT_MBS, c->crop);
    u32 accepted;

    memset(&stream, 0, sizeof(stream));
    stream.pStrmBuffStart = buffer;
    stream.pStrmCurrPos = buffer;
    stream.strmBuffSize = size;
    accepted = h264bsdDecodeSeqParamSet(&stream, &sps) == HANTRO_OK;
    if (accepted != c->accepted) {
      fprintf(stderr, "%s: %s, expected %s\n", c->name,
              accepted ? "accepted" : "rejected",
              c->accepted ? "accepted" : "rejected");
      return 1;
    }
  }
  return 0;
}
