#include <stdio.h>
#include <string.h>

#include "h264bsd_seq_param_set.h"
#include "h264bsd_stream.h"
#include "h264bsd_util.h"

typedef struct {
  u8 data[32];
  u32 bits;
} BitWriter;

static void PutBits(BitWriter* writer, u32 value, u32 count) {
  while (count--) {
    if ((value >> count) & 1U) {
      writer->data[writer->bits / 8U] |= (u8)(0x80U >> (writer->bits % 8U));
    }
    ++writer->bits;
  }
}

/* Writes value as an unsigned Exp-Golomb code. */
static void PutUe(BitWriter* writer, u32 value) {
  u32 code = value + 1U;
  u32 length = 0;

  while ((code >> length) > 1U) {
    ++length;
  }
  PutBits(writer, 0, length);
  PutBits(writer, code, length + 1U);
}

/* Builds a Baseline SPS RBSP with frame-only coding, POC type 2, and no VUI. */
static u32 BuildSps(u8* out, u32 level, u32 num_ref_frames, u32 width_mbs,
                    u32 height_mbs) {
  BitWriter writer;

  memset(&writer, 0, sizeof(writer));
  PutBits(&writer, 66, 8); /* profile_idc */
  PutBits(&writer, 0, 8);  /* constraint flags and reserved bits */
  PutBits(&writer, level, 8);
  PutUe(&writer, 0); /* seq_parameter_set_id */
  PutUe(&writer, 0); /* log2_max_frame_num_minus4 */
  PutUe(&writer, 2); /* pic_order_cnt_type */
  PutUe(&writer, num_ref_frames);
  PutBits(&writer, 0, 1); /* gaps_in_frame_num_value_allowed_flag */
  PutUe(&writer, width_mbs - 1U);
  PutUe(&writer, height_mbs - 1U);
  PutBits(&writer, 1, 1); /* frame_mbs_only_flag */
  PutBits(&writer, 1, 1); /* direct_8x8_inference_flag */
  PutBits(&writer, 0, 1); /* frame_cropping_flag */
  PutBits(&writer, 0, 1); /* vui_parameters_present_flag */
  PutBits(&writer, 1, 1); /* rbsp_stop_one_bit */
  memcpy(out, writer.data, sizeof(writer.data));
  return (writer.bits + 7U) / 8U;
}

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
