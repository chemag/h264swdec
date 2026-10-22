cmake_minimum_required(VERSION 3.16)

project(
  H264SwDec
  VERSION 2.3.0
  LANGUAGES C
)

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

add_library(
  h264swdec
  source/h264bsd_transform.c
  source/h264bsd_util.c
  source/h264bsd_byte_stream.c
  source/h264bsd_seq_param_set.c
  source/h264bsd_pic_param_set.c
  source/h264bsd_slice_header.c
  source/h264bsd_slice_data.c
  source/h264bsd_macroblock_layer.c
  source/h264bsd_stream.c
  source/h264bsd_vlc.c
  source/h264bsd_cavlc.c
  source/h264bsd_nal_unit.c
  source/h264bsd_neighbour.c
  source/h264bsd_storage.c
  source/h264bsd_slice_group_map.c
  source/h264bsd_intra_prediction.c
  source/h264bsd_inter_prediction.c
  source/h264bsd_reconstruct.c
  source/h264bsd_dpb.c
  source/h264bsd_image.c
  source/h264bsd_deblocking.c
  source/h264bsd_conceal.c
  source/h264bsd_vui.c
  source/h264bsd_pic_order_cnt.c
  source/h264bsd_decoder.c
  source/H264SwDecApi.c
)
add_library(H264SwDec::h264swdec ALIAS h264swdec)

set_target_properties(
  h264swdec
  PROPERTIES
    C_EXTENSIONS NO
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    PUBLIC_HEADER "inc/H264SwDecApi.h;inc/basetype.h"
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

target_include_directories(
  h264swdec
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

if(BUILD_TESTING)
  add_executable(h264swdec_api_smoke test/api_smoke.c)
  target_link_libraries(h264swdec_api_smoke PRIVATE H264SwDec::h264swdec)
  add_test(NAME h264swdec.api_smoke COMMAND h264swdec_api_smoke)
endif()

set(H264SWDEC_CMAKE_INSTALL_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/H264SwDec")

configure_package_config_file(
  cmake/H264SwDecConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/H264SwDecConfig.cmake"
  INSTALL_DESTINATION "${H264SWDEC_CMAKE_INSTALL_DIR}"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/H264SwDecConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

install(
  TARGETS h264swdec
  EXPORT H264SwDecTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
  EXPORT H264SwDecTargets
  FILE H264SwDecTargets.cmake
  NAMESPACE H264SwDec::
  DESTINATION "${H264SWDEC_CMAKE_INSTALL_DIR}"
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/H264SwDecConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/H264SwDecConfigVersion.cmake"
  DESTINATION "${H264SWDEC_CMAKE_INSTALL_DIR}"
)
