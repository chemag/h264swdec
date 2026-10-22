#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H264SwDecApi.h"

static const char* first_feature;
static const char* first_source_file;
static u32 first_source_line;
static u32 reports;

static void record_error(const char* feature, const char* source_file,
                         u32 source_line, void* user_data) {
  (void)user_data;
  reports++;
  if (first_feature == NULL) {
    first_feature = feature;
    first_source_file = source_file;
    first_source_line = source_line;
  }
}

int main(int argc, char** argv) {
  H264SwDecInst decoder = NULL;
  H264SwDecInput input = {0};
  H264SwDecOutput output = {0};
  H264SwDecRet result = H264SWDEC_OK;
  unsigned char* data;
  long length;
  FILE* file;

  if (argc != 2) {
    fprintf(stderr, "usage: %s input.h264\n", argv[0]);
    return 2;
  }
  file = fopen(argv[1], "rb");
  if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
    return 2;
  }
  length = ftell(file);
  if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return 2;
  }
  data = malloc((size_t)length);
  if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
    free(data);
    fclose(file);
    return 2;
  }
  fclose(file);

  H264SwDecSetErrorCallback(record_error, NULL);
  if (H264SwDecInit(&decoder, 1) != H264SWDEC_OK) {
    free(data);
    return 2;
  }
  input.pStream = data;
  input.dataLen = (u32)length;
  input.intraConcealmentMethod = 0;
  while (input.dataLen != 0U) {
    u32 consumed;

    result = H264SwDecDecode(decoder, &input, &output);
    if (output.pStrmCurrPos < input.pStream ||
        output.pStrmCurrPos > input.pStream + input.dataLen) {
      result = H264SWDEC_PARAM_ERR;
      break;
    }
    consumed = (u32)(output.pStrmCurrPos - input.pStream);
    input.pStream = output.pStrmCurrPos;
    input.dataLen -= consumed;
    if (result == H264SWDEC_STRM_ERR || result < 0) {
      break;
    }
    if (consumed == 0U) {
      result = H264SWDEC_PARAM_ERR;
      break;
    }
  }
  H264SwDecRelease(decoder);
  H264SwDecSetErrorCallback(NULL, NULL);
  free(data);

  if (result != H264SWDEC_STRM_ERR || reports == 0U || first_feature == NULL ||
      strcmp(first_feature, "slice_type") != 0 || first_source_file == NULL ||
      first_source_line == 0U) {
    fprintf(stderr, "result=%d reports=%u feature=%s file=%s line=%u\n", result,
            reports, first_feature != NULL ? first_feature : "none",
            first_source_file != NULL ? first_source_file : "none",
            first_source_line);
    return 1;
  }
  printf("feature=%s file=%s line=%u reports=%u\n", first_feature,
         first_source_file, first_source_line, reports);
  return 0;
}
