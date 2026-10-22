#include <stddef.h>

#include "H264SwDecApi.h"

int main(void) {
  H264SwDecApiVersion version = H264SwDecGetAPIVersion();
  H264SwDecInst decoder = NULL;
  H264SwDecRet result;

  if (version.major != 2 || version.minor != 3) {
    return 1;
  }

  result = H264SwDecInit(&decoder, 0);
  if (result != H264SWDEC_OK || decoder == NULL) {
    return 2;
  }

  H264SwDecRelease(decoder);
  return 0;
}
