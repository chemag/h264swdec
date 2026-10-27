#ifndef H264SWDEC_TEST_SPS_WRITER_H_
#define H264SWDEC_TEST_SPS_WRITER_H_

/* Writes minimal SPS RBSPs for unit tests of h264bsdDecodeSeqParamSet(). */

#include <stddef.h>
#include <string.h>

#include "basetype.h"

typedef struct {
  u8 data[64];
  u32 bits;
} BitWriter;

static inline void PutBits(BitWriter* writer, u32 value, u32 count) {
  while (count--) {
    if ((value >> count) & 1U) {
      writer->data[writer->bits / 8U] |= (u8)(0x80U >> (writer->bits % 8U));
    }
    ++writer->bits;
  }
}

/* Writes value as an unsigned Exp-Golomb code. */
static inline void PutUe(BitWriter* writer, u32 value) {
  u32 code = value + 1U;
  u32 length = 0;

  while ((code >> length) > 1U) {
    ++length;
  }
  PutBits(writer, 0, length);
  PutBits(writer, code, length + 1U);
}

/* Builds a Baseline SPS RBSP with frame-only coding, POC type 2, and no VUI.
 * crop is NULL for no cropping, or the left, right, top, and bottom
 * frame_crop offsets. Values must be below 2^32 - 1. Returns the size in
 * bytes, at most 64. */
static inline u32 BuildSpsCropped(u8* out, u32 level, u32 num_ref_frames,
                                  u32 width_mbs, u32 height_mbs,
                                  const u32* crop) {
  BitWriter writer;
  u32 size;
  u32 index;

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
  PutBits(&writer, 1, 1);            /* frame_mbs_only_flag */
  PutBits(&writer, 1, 1);            /* direct_8x8_inference_flag */
  PutBits(&writer, crop != NULL, 1); /* frame_cropping_flag */
  if (crop != NULL) {
    for (index = 0; index < 4; ++index) {
      PutUe(&writer, crop[index]);
    }
  }
  PutBits(&writer, 0, 1); /* vui_parameters_present_flag */
  PutBits(&writer, 1, 1); /* rbsp_stop_one_bit */
  size = (writer.bits + 7U) / 8U;
  memcpy(out, writer.data, size);
  return size;
}

static inline u32 BuildSps(u8* out, u32 level, u32 num_ref_frames,
                           u32 width_mbs, u32 height_mbs) {
  return BuildSpsCropped(out, level, num_ref_frames, width_mbs, height_mbs,
                         NULL);
}

#endif /* H264SWDEC_TEST_SPS_WRITER_H_ */
