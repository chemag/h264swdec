/*
 * Copyright (C) 2026 Chema Gonzalez
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef H264SWDEC_DEBLOCKING_INTERNAL_H
#define H264SWDEC_DEBLOCKING_INTERNAL_H

#include "basetype.h"

static inline i32 h264bsdFilterDelta(i32 p0, i32 q0, i32 p1, i32 q1) {
  return (4 * (q0 - p0) + (p1 - q1) + 4) >> 3;
}

#endif /* H264SWDEC_DEBLOCKING_INTERNAL_H */
