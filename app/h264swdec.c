#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H264SwDecApi.h"
#include "config.h"

typedef struct {
  FILE* file;
  H264SwDecInfo info;
  uint32_t pictures;
  uint32_t concealed_macroblocks;
  int crop;
  int have_info;
} OutputContext;

static void PrintUsage(const char* program) {
  fprintf(stderr,
          "Usage: %s [--crop] INPUT.h264 OUTPUT.yuv\n"
          "       %s --version\n"
          "\n"
          "Decode an Annex B H.264 stream to planar I420. Use --crop to write\n"
          "the visible crop rectangle instead of the coded frame. Use - as\n"
          "OUTPUT.yuv to write I420 to standard output.\n",
          program, program);
}

static int ReadInput(const char* path, uint8_t** data, uint32_t* size) {
  FILE* file;
  long length;
  uint8_t* buffer;

  file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Cannot open input %s: %s\n", path, strerror(errno));
    return -1;
  }

  if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
      (uint64_t)length > UINT32_MAX || fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Cannot determine input size for %s\n", path);
    fclose(file);
    return -1;
  }

  buffer = malloc((size_t)length);
  if (buffer == NULL) {
    fprintf(stderr, "Cannot allocate %ld bytes for input\n", length);
    fclose(file);
    return -1;
  }

  if (fread(buffer, 1, (size_t)length, file) != (size_t)length) {
    fprintf(stderr, "Cannot read input %s\n", path);
    free(buffer);
    fclose(file);
    return -1;
  }

  fclose(file);
  *data = buffer;
  *size = (uint32_t)length;
  return 0;
}

static int WriteBytes(FILE* file, const uint8_t* data, size_t size) {
  if (fwrite(data, 1, size, file) != size) {
    fprintf(stderr, "Cannot write output: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

static int WritePlane(FILE* file, const uint8_t* data, uint32_t stride,
                      uint32_t left, uint32_t top, uint32_t width,
                      uint32_t height) {
  uint32_t row;

  data += (size_t)top * stride + left;
  for (row = 0; row < height; ++row) {
    if (WriteBytes(file, data, width) != 0) {
      return -1;
    }
    data += stride;
  }
  return 0;
}

static int WritePicture(OutputContext* context,
                        const H264SwDecPicture* picture) {
  const uint8_t* data = (const uint8_t*)picture->pOutputPicture;
  uint32_t coded_width = context->info.picWidth;
  uint32_t coded_height = context->info.picHeight;
  uint64_t luma_size = (uint64_t)coded_width * coded_height;

  if (data == NULL || coded_width == 0 || coded_height == 0 ||
      (coded_width & 1U) != 0 || (coded_height & 1U) != 0 ||
      luma_size > SIZE_MAX / 3U) {
    fprintf(stderr, "Decoder returned invalid picture metadata\n");
    return -1;
  }

  if (!context->crop || !context->info.croppingFlag) {
    if (WriteBytes(context->file, data, (size_t)(luma_size * 3U / 2U)) != 0) {
      return -1;
    }
  } else {
    const CropParams* crop = &context->info.cropParams;
    uint32_t left = crop->cropLeftOffset;
    uint32_t top = crop->cropTopOffset;
    uint32_t width = crop->cropOutWidth;
    uint32_t height = crop->cropOutHeight;
    const uint8_t* chroma_u = data + luma_size;
    const uint8_t* chroma_v = chroma_u + luma_size / 4U;

    if (width == 0 || height == 0 || width > coded_width ||
        height > coded_height || ((left | top | width | height) & 1U) != 0 ||
        left > coded_width - width || top > coded_height - height) {
      fprintf(stderr, "Decoder returned an invalid I420 crop rectangle\n");
      return -1;
    }

    if (WritePlane(context->file, data, coded_width, left, top, width,
                   height) != 0 ||
        WritePlane(context->file, chroma_u, coded_width / 2U, left / 2U,
                   top / 2U, width / 2U, height / 2U) != 0 ||
        WritePlane(context->file, chroma_v, coded_width / 2U, left / 2U,
                   top / 2U, width / 2U, height / 2U) != 0) {
      return -1;
    }
  }

  ++context->pictures;
  context->concealed_macroblocks += picture->nbrOfErrMBs;
  return 0;
}

static int DrainPictures(H264SwDecInst decoder, OutputContext* context,
                         uint32_t flush) {
  H264SwDecPicture picture;
  H264SwDecRet result;

  for (;;) {
    result = H264SwDecNextPicture(decoder, &picture, flush);
    if (result == H264SWDEC_OK) {
      return 0;
    }
    if (result != H264SWDEC_PIC_RDY) {
      fprintf(stderr, "H264SwDecNextPicture failed: %d\n", result);
      return -1;
    }
    if (!context->have_info || WritePicture(context, &picture) != 0) {
      return -1;
    }
  }
}

static int UpdateStreamInfo(H264SwDecInst decoder, OutputContext* context) {
  H264SwDecRet result = H264SwDecGetInfo(decoder, &context->info);
  const CropParams* crop = &context->info.cropParams;

  if (result != H264SWDEC_OK) {
    fprintf(stderr, "H264SwDecGetInfo failed: %d\n", result);
    return -1;
  }

  context->have_info = 1;
  fprintf(stderr, "Stream: profile %u, coded %ux%u", context->info.profile,
          context->info.picWidth, context->info.picHeight);
  if (context->info.croppingFlag) {
    fprintf(stderr, ", crop %u,%u %ux%u", crop->cropLeftOffset,
            crop->cropTopOffset, crop->cropOutWidth, crop->cropOutHeight);
  }
  fputc('\n', stderr);
  return 0;
}

static int Decode(const uint8_t* data, uint32_t size, OutputContext* context) {
  H264SwDecInst decoder = NULL;
  H264SwDecInput input;
  H264SwDecOutput output;
  H264SwDecRet result;
  const uint8_t* current = data;
  uint32_t remaining = size;
  uint32_t decoded_pictures = 0;
  uint32_t stalled_calls = 0;
  int status = -1;

  result = H264SwDecInit(&decoder, 0);
  if (result != H264SWDEC_OK) {
    fprintf(stderr, "H264SwDecInit failed: %d\n", result);
    return -1;
  }

  while (remaining != 0) {
    size_t consumed;

    input.pStream = (uint8_t*)current;
    input.dataLen = remaining;
    input.picId = decoded_pictures + 1U;
    input.intraConcealmentMethod = 0;

    result = H264SwDecDecode(decoder, &input, &output);
    if (output.pStrmCurrPos == NULL || output.pStrmCurrPos < current ||
        output.pStrmCurrPos > current + remaining) {
      fprintf(stderr, "Decoder returned an invalid stream position\n");
      goto cleanup;
    }

    consumed = (size_t)(output.pStrmCurrPos - current);
    current = output.pStrmCurrPos;
    remaining -= (uint32_t)consumed;

    if (consumed == 0) {
      if (++stalled_calls > 4U) {
        fprintf(stderr, "Decoder stopped making stream progress\n");
        goto cleanup;
      }
    } else {
      stalled_calls = 0;
    }

    switch (result) {
      case H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY:
        if (UpdateStreamInfo(decoder, context) != 0) {
          goto cleanup;
        }
        break;

      case H264SWDEC_PIC_RDY_BUFF_NOT_EMPTY:
      case H264SWDEC_PIC_RDY:
        ++decoded_pictures;
        if (DrainPictures(decoder, context, 0) != 0) {
          goto cleanup;
        }
        break;

      case H264SWDEC_STRM_PROCESSED:
        break;

      case H264SWDEC_STRM_ERR:
        fprintf(stderr, "Decoder rejected the input stream\n");
        goto cleanup;

      default:
        fprintf(stderr, "H264SwDecDecode failed: %d\n", result);
        goto cleanup;
    }
  }

  if (DrainPictures(decoder, context, 1) != 0) {
    goto cleanup;
  }
  if (!context->have_info || context->pictures == 0) {
    fprintf(stderr, "No pictures decoded\n");
    goto cleanup;
  }

  fprintf(stderr,
          "Decoded %u pictures; wrote %u pictures; concealed %u macroblocks.\n",
          decoded_pictures, context->pictures, context->concealed_macroblocks);
  status = 0;

cleanup:
  H264SwDecRelease(decoder);
  return status;
}

int main(int argc, char** argv) {
  const char* input_path;
  const char* output_path;
  uint8_t* data = NULL;
  uint32_t size = 0;
  OutputContext context;
  int crop = 0;
  int status = EXIT_FAILURE;

  if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    fprintf(stdout, "version: %s\n", PROJECT_VERSION);
    return EXIT_SUCCESS;
  }

  if (argc == 4 && strcmp(argv[1], "--crop") == 0) {
    crop = 1;
    input_path = argv[2];
    output_path = argv[3];
  } else if (argc == 3) {
    input_path = argv[1];
    output_path = argv[2];
  } else {
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (ReadInput(input_path, &data, &size) != 0) {
    return EXIT_FAILURE;
  }

  memset(&context, 0, sizeof(context));
  context.crop = crop;
  if (strcmp(output_path, "-") == 0) {
    context.file = stdout;
  } else {
    context.file = fopen(output_path, "wb");
    if (context.file == NULL) {
      fprintf(stderr, "Cannot open output %s: %s\n", output_path,
              strerror(errno));
      goto cleanup;
    }
  }

  if (Decode(data, size, &context) == 0 && fflush(context.file) == 0) {
    status = EXIT_SUCCESS;
  }

cleanup:
  if (context.file != NULL && context.file != stdout &&
      fclose(context.file) != 0) {
    status = EXIT_FAILURE;
  }
  free(data);
  return status;
}
