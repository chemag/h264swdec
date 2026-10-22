#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "H264SwDecApi.h"

static size_t allocation_count;
static size_t free_count;
static size_t outstanding_allocations;
static int fail_next_allocation;

void* H264SwDecMalloc(u32 size, u32 num) {
  void* memory;

  if (num != 0 && size > UINT32_MAX / num) {
    return NULL;
  }
  if (fail_next_allocation) {
    fail_next_allocation = 0;
    return NULL;
  }

  memory = malloc((size_t)size * num);
  if (memory != NULL) {
    ++allocation_count;
    ++outstanding_allocations;
  }
  return memory;
}

void H264SwDecFree(void* memory) {
  if (memory != NULL) {
    ++free_count;
    --outstanding_allocations;
  }
  free(memory);
}

int main(void) {
  H264SwDecInst decoder = NULL;
  H264SwDecRet result;

  if (H264SwDecMalloc(UINT32_MAX, 2) != NULL) {
    fprintf(stderr, "overflowing allocation was not rejected\n");
    return 1;
  }

  fail_next_allocation = 1;
  result = H264SwDecInit(&decoder, 0);
  if (result != H264SWDEC_MEMFAIL || decoder != NULL) {
    fprintf(stderr, "allocation failure returned %d\n", result);
    return 2;
  }

  result = H264SwDecInit(&decoder, 0);
  if (result != H264SWDEC_OK || decoder == NULL) {
    fprintf(stderr, "custom-allocator initialization returned %d\n", result);
    return 3;
  }
  H264SwDecRelease(decoder);

  if (allocation_count == 0 || allocation_count != free_count ||
      outstanding_allocations != 0) {
    fprintf(stderr, "unbalanced allocations: %zu allocations, %zu frees\n",
            allocation_count, free_count);
    return 4;
  }

  return 0;
}
