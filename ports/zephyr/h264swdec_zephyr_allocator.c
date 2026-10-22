/* SPDX-FileCopyrightText: Copyright (c) 2026 Chema Gonzalez */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "H264SwDecApi.h"

LOG_MODULE_REGISTER(h264swdec_allocator, LOG_LEVEL_ERR);
K_HEAP_DEFINE(h264swdec_heap, H264SWDEC_ZEPHYR_HEAP_BYTES);

void* H264SwDecMalloc(u32 size, u32 num) {
  size_t bytes;
  void* memory;

  if (num != 0 && size > SIZE_MAX / num) {
    LOG_ERR("allocation size overflow: %u * %u", size, num);
    return NULL;
  }

  bytes = (size_t)size * num;
  memory = k_heap_alloc(&h264swdec_heap, bytes, K_NO_WAIT);
  if (memory == NULL) {
    LOG_ERR("allocation failed: %zu bytes", bytes);
  }
  return memory;
}

void H264SwDecFree(void* memory) {
  if (memory != NULL) {
    k_heap_free(&h264swdec_heap, memory);
  }
}
