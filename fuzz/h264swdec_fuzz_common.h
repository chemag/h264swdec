#ifndef H264SWDEC_FUZZ_COMMON_H_
#define H264SWDEC_FUZZ_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "H264SwDecApi.h"

/* Upper bound on decoder calls for one buffer. Each call consumes at least
 * one NAL unit, so this only stops inputs that would otherwise stall. */
#define H264SWDEC_FUZZ_MAX_STEPS 65536U
#define H264SWDEC_FUZZ_MAX_STALLS 4U

enum {
  H264SWDEC_FUZZ_OK = 0,
  H264SWDEC_FUZZ_DECODE_ERROR = -1,
  H264SWDEC_FUZZ_STALLED = -2,
};

static volatile uint8_t h264swdec_fuzz_sink;

static void H264SwDecFuzzIgnoreError(const char* feature,
                                     const char* source_file, u32 source_line,
                                     void* user_data) {
  (void)feature;
  (void)source_file;
  (void)source_line;
  (void)user_data;
}

/* Decoder plus the stream information that applies to the pictures it
 * outputs. H264SwDecGetInfo() reports the active SPS, which can change before
 * every picture of the previous sequence has been output, so the information
 * is only refreshed when the decoder returns H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY,
 * after the pictures still pending under the old information are drained. */
typedef struct {
  H264SwDecInst decoder;
  H264SwDecInfo info;
  int have_info;
  uint32_t pic_id;
} H264SwDecFuzzContext;

/* Reads every byte of an output picture, so the sanitizers check that the
 * decoder handed back a buffer that covers the frame it reported. */
static void H264SwDecFuzzConsumePicture(const H264SwDecFuzzContext* context,
                                        const H264SwDecPicture* picture) {
  const uint8_t* data = (const uint8_t*)picture->pOutputPicture;
  uint64_t luma_size;
  uint64_t frame_size;
  uint64_t index;
  uint8_t sum = 0;

  if (!context->have_info || data == NULL) {
    __builtin_trap();
  }

  luma_size = (uint64_t)context->info.picWidth * context->info.picHeight;
  frame_size = luma_size + luma_size / 2U;
  for (index = 0; index < frame_size; ++index) {
    sum = (uint8_t)(sum + data[index]);
  }
  h264swdec_fuzz_sink = sum;
}

static void H264SwDecFuzzDrain(const H264SwDecFuzzContext* context, u32 flush) {
  H264SwDecPicture picture;
  uint32_t pictures;

  /* The DPB holds at most 16 pictures plus the current one. */
  for (pictures = 0; pictures < 32U; ++pictures) {
    if (H264SwDecNextPicture(context->decoder, &picture, flush) !=
        H264SWDEC_PIC_RDY) {
      return;
    }
    H264SwDecFuzzConsumePicture(context, &picture);
  }
  __builtin_trap();
}

/* Drains pictures that belong to the old stream information, then caches and
 * validates the new information. */
static void H264SwDecFuzzUpdateInfo(H264SwDecFuzzContext* context) {
  const CropParams* crop = &context->info.cropParams;

  if (context->have_info) {
    H264SwDecFuzzDrain(context, 0);
  }
  if (H264SwDecGetInfo(context->decoder, &context->info) != H264SWDEC_OK ||
      context->info.picWidth == 0 || context->info.picHeight == 0 ||
      (context->info.picWidth & 15U) != 0 ||
      (context->info.picHeight & 15U) != 0) {
    __builtin_trap();
  }
  if (context->info.croppingFlag &&
      (crop->cropOutWidth == 0 || crop->cropOutHeight == 0 ||
       crop->cropOutWidth > context->info.picWidth ||
       crop->cropOutHeight > context->info.picHeight ||
       crop->cropLeftOffset > context->info.picWidth - crop->cropOutWidth ||
       crop->cropTopOffset > context->info.picHeight - crop->cropOutHeight)) {
    __builtin_trap();
  }
  context->have_info = 1;
}

static int H264SwDecFuzzInit(H264SwDecFuzzContext* context,
                             u32 no_output_reordering) {
  memset(context, 0, sizeof(*context));
  context->pic_id = 1;
  return H264SwDecInit(&context->decoder, no_output_reordering) == H264SWDEC_OK
             ? 0
             : -1;
}

/* Feeds one buffer through H264SwDecDecode until it is consumed, drains the
 * pictures each call produces, and checks the stream-position contract.
 *
 * The decoder removes emulation prevention bytes in place, so it is given a
 * private copy, as a real caller must do when it keeps the original bytes. */
static int H264SwDecFuzzDecodeBuffer(H264SwDecFuzzContext* context,
                                     const uint8_t* data, uint32_t size) {
  H264SwDecInput input;
  H264SwDecOutput output;
  uint32_t steps;
  uint32_t stalls = 0;
  u8* copy;
  int status = H264SWDEC_FUZZ_STALLED;

  if (size == 0) {
    return H264SWDEC_FUZZ_OK;
  }
  copy = (u8*)malloc(size);
  if (copy == NULL) {
    return H264SWDEC_FUZZ_DECODE_ERROR;
  }
  memcpy(copy, data, size);

  input.pStream = copy;
  input.dataLen = size;
  input.intraConcealmentMethod = 0;

  for (steps = 0; steps < H264SWDEC_FUZZ_MAX_STEPS && input.dataLen != 0;
       ++steps) {
    H264SwDecRet result;
    uint32_t consumed;

    input.picId = context->pic_id;
    output.pStrmCurrPos = NULL;
    result = H264SwDecDecode(context->decoder, &input, &output);
    if (output.pStrmCurrPos == NULL || output.pStrmCurrPos < input.pStream ||
        output.pStrmCurrPos > input.pStream + input.dataLen) {
      if (result < 0) {
        status = H264SWDEC_FUZZ_DECODE_ERROR;
        goto done;
      }
      __builtin_trap();
    }

    consumed = (uint32_t)(output.pStrmCurrPos - input.pStream);
    input.pStream = output.pStrmCurrPos;
    input.dataLen -= consumed;

    switch (result) {
      case H264SWDEC_PIC_RDY:
      case H264SWDEC_PIC_RDY_BUFF_NOT_EMPTY:
        ++context->pic_id;
        H264SwDecFuzzDrain(context, 0);
        break;
      case H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY:
        H264SwDecFuzzUpdateInfo(context);
        break;
      case H264SWDEC_STRM_PROCESSED:
        break;
      default:
        status = H264SWDEC_FUZZ_DECODE_ERROR;
        goto done;
    }

    if (consumed != 0) {
      stalls = 0;
    } else if (input.dataLen != 0 && ++stalls > H264SWDEC_FUZZ_MAX_STALLS) {
      goto done;
    }
  }
  if (input.dataLen == 0) {
    status = H264SWDEC_FUZZ_OK;
  }

done:
  free(copy);
  return status;
}

#endif /* H264SWDEC_FUZZ_COMMON_H_ */
