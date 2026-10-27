#ifndef H264SWDEC_TEST_SPS_WRITER_H_
#define H264SWDEC_TEST_SPS_WRITER_H_

/* Writes minimal SPS RBSPs for unit tests of h264bsdDecodeSeqParamSet(). */

#include <string.h>

#include "basetype.h"

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

#endif /* H264SWDEC_TEST_SPS_WRITER_H_ */
