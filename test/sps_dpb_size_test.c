#include <stdio.h>
#include <string.h>

#include "h264bsd_seq_param_set.h"
#include "h264bsd_stream.h"
#include "h264bsd_util.h"
#include "sps_writer.h"

struct DpbCase {
  const char* name;
  u32 level;
  u32 num_ref_frames;
  u32 width_mbs;
  u32 height_mbs;
  u32 expected_dpb_size;
};

int main(void) {
  static const struct DpbCase cases[] = {
      /* 176x144 at Level 1.0 allows 4 frames. */
      {"level DPB", 10, 0, 11, 9, 4},
      /* 176x160 exceeds Level 1.0, so the DPB falls back to num_ref_frames,
       * which is zero. The DPB must still hold one frame. */
      {"oversized picture, zero references", 10, 0, 11, 10, 1},
      {"oversized picture, two references", 10, 2, 11, 10, 2},
  };
  size_t index;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    const struct DpbCase* c = &cases[index];
    u8 buffer[32];
    strmData_t stream;
    seqParamSet_t sps;
    u32 size = BuildSps(buffer, c->level, c->num_ref_frames, c->width_mbs,
                        c->height_mbs);

    memset(&stream, 0, sizeof(stream));
    stream.pStrmBuffStart = buffer;
    stream.pStrmCurrPos = buffer;
    stream.strmBuffSize = size;
    if (h264bsdDecodeSeqParamSet(&stream, &sps) != HANTRO_OK) {
      fprintf(stderr, "%s: SPS was rejected\n", c->name);
      return 1;
    }
    if (sps.maxDpbSize != c->expected_dpb_size) {
      fprintf(stderr, "%s: maxDpbSize %u, expected %u\n", c->name,
              sps.maxDpbSize, c->expected_dpb_size);
      return 1;
    }
  }
  return 0;
}
