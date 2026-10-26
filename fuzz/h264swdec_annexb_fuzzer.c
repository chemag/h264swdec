/* Decodes the whole fuzz input as one Annex B byte stream, the way the POSIX
 * decoder does, then flushes the DPB. */

#include <stddef.h>
#include <stdint.h>

#include "h264swdec_fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  H264SwDecFuzzContext context;

  if (size == 0 || size > UINT32_MAX) {
    return 0;
  }

  H264SwDecSetErrorCallback(H264SwDecFuzzIgnoreError, NULL);
  if (H264SwDecFuzzInit(&context, 0) != 0) {
    return 0;
  }

  (void)H264SwDecFuzzDecodeBuffer(&context, data, (uint32_t)size);
  H264SwDecFuzzDrain(&context, 1);
  H264SwDecRelease(context.decoder);
  return 0;
}
