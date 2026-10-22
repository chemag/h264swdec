# SPDX-FileCopyrightText: Copyright (c) 2026 Chema Gonzalez
# SPDX-License-Identifier: Apache-2.0

set(H264SWDEC_ZEPHYR_HEAP_BYTES 3145728 CACHE STRING
    "Bytes reserved for the H264SwDec Zephyr heap")

set(H264SWDEC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

add_library(
  h264swdec
  OBJECT
  ${H264SWDEC_ROOT}/source/h264bsd_transform.c
  ${H264SWDEC_ROOT}/source/h264bsd_util.c
  ${H264SWDEC_ROOT}/source/h264bsd_byte_stream.c
  ${H264SWDEC_ROOT}/source/h264bsd_seq_param_set.c
  ${H264SWDEC_ROOT}/source/h264bsd_pic_param_set.c
  ${H264SWDEC_ROOT}/source/h264bsd_slice_header.c
  ${H264SWDEC_ROOT}/source/h264bsd_slice_data.c
  ${H264SWDEC_ROOT}/source/h264bsd_macroblock_layer.c
  ${H264SWDEC_ROOT}/source/h264bsd_stream.c
  ${H264SWDEC_ROOT}/source/h264bsd_vlc.c
  ${H264SWDEC_ROOT}/source/h264bsd_cavlc.c
  ${H264SWDEC_ROOT}/source/h264bsd_nal_unit.c
  ${H264SWDEC_ROOT}/source/h264bsd_neighbour.c
  ${H264SWDEC_ROOT}/source/h264bsd_storage.c
  ${H264SWDEC_ROOT}/source/h264bsd_slice_group_map.c
  ${H264SWDEC_ROOT}/source/h264bsd_intra_prediction.c
  ${H264SWDEC_ROOT}/source/h264bsd_inter_prediction.c
  ${H264SWDEC_ROOT}/source/h264bsd_reconstruct.c
  ${H264SWDEC_ROOT}/source/h264bsd_dpb.c
  ${H264SWDEC_ROOT}/source/h264bsd_image.c
  ${H264SWDEC_ROOT}/source/h264bsd_deblocking.c
  ${H264SWDEC_ROOT}/source/h264bsd_conceal.c
  ${H264SWDEC_ROOT}/source/h264bsd_vui.c
  ${H264SWDEC_ROOT}/source/h264bsd_pic_order_cnt.c
  ${H264SWDEC_ROOT}/source/h264bsd_decoder.c
  ${H264SWDEC_ROOT}/source/H264SwDecApi.c
  ${H264SWDEC_ROOT}/source/H264SwDecAllocator.c
  h264swdec_zephyr_allocator.c
)

set_target_properties(
  h264swdec
  PROPERTIES
    C_EXTENSIONS NO
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
)

target_include_directories(
  h264swdec
  PUBLIC ${H264SWDEC_ROOT}/inc
  PRIVATE ${H264SWDEC_ROOT}/source
)

target_compile_definitions(
  h264swdec
  PRIVATE
    H264SWDEC_CUSTOM_ALLOCATOR=1
    H264SWDEC_ZEPHYR_HEAP_BYTES=${H264SWDEC_ZEPHYR_HEAP_BYTES}
)

target_compile_options(
  h264swdec
  PRIVATE
    -Wno-maybe-uninitialized
    -Wno-unused-but-set-variable
    -Wno-unused-parameter
    -Wno-unused-variable
)

target_link_libraries(h264swdec PRIVATE zephyr_interface)
