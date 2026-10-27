#include <stdio.h>
#include <string.h>

#include "h264bsd_cfg.h"
#include "h264bsd_seq_param_set.h"
#include "h264bsd_stream.h"
#include "h264bsd_util.h"
#include "sps_writer.h"

#define MAX_WIDTH_MBS (H264SWDEC_MAX_FRAME_WIDTH / 16)
#define MAX_HEIGHT_MBS (H264SWDEC_MAX_FRAME_HEIGHT / 16)
#define MAX_AREA_MBS (H264SWDEC_MAX_FRAME_SIZE / 256)

struct SizeCase {
  const char* name;
  u32 width_mbs;
  u32 height_mbs;
  u32 accepted;
};

static u32 Decode(u32 width_mbs, u32 height_mbs) {
  u8 buffer[32];
  strmData_t stream;
  seqParamSet_t sps;
  /* Level 5.1 allows any size up to 36,864 macroblocks. */
  u32 size = BuildSps(buffer, 51, 1, width_mbs, height_mbs);

  memset(&stream, 0, sizeof(stream));
  stream.pStrmBuffStart = buffer;
  stream.pStrmCurrPos = buffer;
  stream.strmBuffSize = size;
  return h264bsdDecodeSeqParamSet(&stream, &sps);
}

static int Check(const char* name, u32 width_mbs, u32 height_mbs,
                 u32 expected) {
  u32 accepted = Decode(width_mbs, height_mbs) == HANTRO_OK;

  if (accepted != expected) {
    fprintf(stderr, "%s: %ux%u MBs %s, expected %s\n", name, width_mbs,
            height_mbs, accepted ? "accepted" : "rejected",
            expected ? "accepted" : "rejected");
    return 1;
  }
  return 0;
}

int main(void) {
  const struct SizeCase cases[] = {
      {"smallest picture", 1, 1, 1},
      {"maximum width", MAX_WIDTH_MBS, 1, 1},
      {"maximum height", 1, MAX_HEIGHT_MBS, 1},
      {"width over the limit", MAX_WIDTH_MBS + 1, 1, 0},
      {"height over the limit", 1, MAX_HEIGHT_MBS + 1, 0},
      /* 176x8274880, the size of a fuzzer-generated SPS. */
      {"fuzzer height", 11, 517180, 0},
      /* pic_width_in_mbs_minus1 of 2^32 - 2, which wraps if one is added
       * before the check. */
      {"wrapping width", 0xffffffffU, 1, 0},
  };
  size_t index;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    if (Check(cases[index].name, cases[index].width_mbs,
              cases[index].height_mbs, cases[index].accepted) != 0) {
      return 1;
    }
  }

  /* The area limit only matters when it is smaller than the maximum width
   * times the maximum height. Then, at full width, the tallest picture that
   * fits the area is below the height limit, so the area alone decides. */
  if ((unsigned long long)MAX_WIDTH_MBS * MAX_HEIGHT_MBS > MAX_AREA_MBS) {
    u32 height = MAX_AREA_MBS / MAX_WIDTH_MBS;

    if (Check("maximum area", MAX_WIDTH_MBS, height, 1) != 0 ||
        Check("area over the limit", MAX_WIDTH_MBS, height + 1, 0) != 0) {
      return 1;
    }
  }
  return 0;
}
