/* Feeds the fuzz input one NAL unit at a time with output reordering
 * disabled, as an RTC receiver does. A decoder error resets the decoder,
 * drops NAL units until the next IDR, and replays the most recent SPS and
 * PPS before that IDR. */

#include <stddef.h>
#include <stdint.h>

#include "h264swdec_fuzz_common.h"

#define NAL_TYPE_IDR 5U
#define NAL_TYPE_SPS 7U
#define NAL_TYPE_PPS 8U
#define MAX_RESETS 64U

typedef struct {
  const uint8_t* data;
  uint32_t size;
} NalUnit;

static uint32_t StartCodeSize(const uint8_t* data, uint32_t size,
                              uint32_t offset) {
  if (offset + 3U <= size && data[offset] == 0 && data[offset + 1U] == 0) {
    if (data[offset + 2U] == 1) {
      return 3U;
    }
    if (offset + 4U <= size && data[offset + 2U] == 0 &&
        data[offset + 3U] == 1) {
      return 4U;
    }
  }
  return 0;
}

/* Returns the next NAL unit, including its start code, or a zero-size unit
 * at the end of the input. Bytes before the first start code are skipped. */
static NalUnit NextNalUnit(const uint8_t* data, uint32_t size,
                           uint32_t* offset) {
  NalUnit unit = {NULL, 0};
  uint32_t begin = *offset;
  uint32_t end;

  while (begin < size && StartCodeSize(data, size, begin) == 0) {
    ++begin;
  }
  if (begin >= size) {
    *offset = size;
    return unit;
  }
  end = begin + StartCodeSize(data, size, begin);
  while (end < size && StartCodeSize(data, size, end) == 0) {
    ++end;
  }
  unit.data = data + begin;
  unit.size = end - begin;
  *offset = end;
  return unit;
}

static uint32_t NalType(NalUnit unit) {
  uint32_t header = StartCodeSize(unit.data, unit.size, 0);
  return header < unit.size ? (unit.data[header] & 0x1fU) : 0;
}

static int Reset(H264SwDecFuzzContext* context) {
  H264SwDecRelease(context->decoder);
  return H264SwDecFuzzInit(context, 1);
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  H264SwDecFuzzContext context;
  NalUnit sps = {NULL, 0};
  NalUnit pps = {NULL, 0};
  uint32_t offset = 0;
  uint32_t resets = 0;
  int waiting_for_idr = 0;

  if (size == 0 || size > UINT32_MAX) {
    return 0;
  }

  H264SwDecSetErrorCallback(H264SwDecFuzzIgnoreError, NULL);
  if (H264SwDecFuzzInit(&context, 1) != 0) {
    return 0;
  }

  for (;;) {
    NalUnit unit = NextNalUnit(data, (uint32_t)size, &offset);
    uint32_t type;
    int status = H264SWDEC_FUZZ_OK;

    if (unit.size == 0) {
      break;
    }
    type = NalType(unit);
    if (type == NAL_TYPE_SPS) {
      sps = unit;
    } else if (type == NAL_TYPE_PPS) {
      pps = unit;
    }

    if (waiting_for_idr) {
      if (type != NAL_TYPE_IDR) {
        continue;
      }
      if (sps.size != 0) {
        status = H264SwDecFuzzDecodeBuffer(&context, sps.data, sps.size);
      }
      if (status == H264SWDEC_FUZZ_OK && pps.size != 0) {
        status = H264SwDecFuzzDecodeBuffer(&context, pps.data, pps.size);
      }
      waiting_for_idr = 0;
    }
    if (status == H264SWDEC_FUZZ_OK) {
      status = H264SwDecFuzzDecodeBuffer(&context, unit.data, unit.size);
    }

    if (status != H264SWDEC_FUZZ_OK) {
      /* A failed reset leaves context.decoder NULL. */
      if (++resets > MAX_RESETS || Reset(&context) != 0) {
        break;
      }
      waiting_for_idr = 1;
    }
  }

  if (context.decoder != NULL) {
    H264SwDecFuzzDrain(&context, 1);
    H264SwDecRelease(context.decoder);
  }
  return 0;
}
