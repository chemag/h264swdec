/*
 * Copyright (C) 2009 The Android Open Source Project
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

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264SWDEC_CFG_H
#define H264SWDEC_CFG_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

#define MAX_NUM_REF_PICS 16
#define MAX_NUM_SLICE_GROUPS 8
#define MAX_NUM_SEQ_PARAM_SETS 32
#define MAX_NUM_PIC_PARAM_SETS 256

/* Largest coded picture accepted from a sequence parameter set, in pixels.
 * An SPS that exceeds any of these limits is rejected while it is parsed,
 * before the decoder allocates memory for the picture. The defaults match
 * libavc (decoder/ih264d_defs.h). Override them at build time to match the
 * memory available. */
#ifndef H264SWDEC_MAX_FRAME_WIDTH
#define H264SWDEC_MAX_FRAME_WIDTH 4080
#endif
#ifndef H264SWDEC_MAX_FRAME_HEIGHT
#define H264SWDEC_MAX_FRAME_HEIGHT 4080
#endif
#ifndef H264SWDEC_MAX_FRAME_SIZE
#define H264SWDEC_MAX_FRAME_SIZE (4096 * 2048)
#endif

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif /* #ifdef H264SWDEC_CFG_H */

