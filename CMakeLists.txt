cmake_minimum_required(VERSION 3.16)

project(
  H264SwDec
  VERSION 2.3.0
  LANGUAGES C
)

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(H264SWDEC_CUSTOM_ALLOCATOR
       "Use an allocator implementation supplied by the final application" OFF)

set(
  H264SWDEC_SOURCES
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
  source/H264SwDecAllocator.c
)
add_library(h264swdec ${H264SWDEC_SOURCES})
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

if(H264SWDEC_CUSTOM_ALLOCATOR)
  target_compile_definitions(h264swdec PRIVATE H264SWDEC_CUSTOM_ALLOCATOR=1)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(H264SWDEC_BUILD_TOOLS_DEFAULT ON)
else()
  set(H264SWDEC_BUILD_TOOLS_DEFAULT OFF)
endif()
option(H264SWDEC_BUILD_TOOLS "Build the POSIX Annex B decoder"
       ${H264SWDEC_BUILD_TOOLS_DEFAULT})

if(H264SWDEC_BUILD_TOOLS)
  add_executable(h264swdec_decoder app/h264swdec.c)
  target_compile_features(h264swdec_decoder PRIVATE c_std_99)
  target_link_libraries(h264swdec_decoder PRIVATE H264SwDec::h264swdec)
  set_target_properties(h264swdec_decoder PROPERTIES OUTPUT_NAME h264swdec)
endif()

if(BUILD_TESTING)
  add_library(h264swdec_custom_allocator_test_library STATIC
              ${H264SWDEC_SOURCES})
  target_compile_definitions(h264swdec_custom_allocator_test_library
                             PRIVATE H264SWDEC_CUSTOM_ALLOCATOR=1)
  target_compile_features(h264swdec_custom_allocator_test_library
                          PRIVATE c_std_99)
  target_include_directories(
    h264swdec_custom_allocator_test_library
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source
  )
  add_executable(h264swdec_custom_allocator_test
                 test/custom_allocator_test.c)
  target_link_libraries(h264swdec_custom_allocator_test
                        PRIVATE h264swdec_custom_allocator_test_library)
  add_test(NAME h264swdec.custom_allocator
           COMMAND h264swdec_custom_allocator_test)

  add_executable(h264swdec_api_smoke test/api_smoke.c)
  target_link_libraries(h264swdec_api_smoke PRIVATE H264SwDec::h264swdec)
  add_test(NAME h264swdec.api_smoke COMMAND h264swdec_api_smoke)

  if(H264SWDEC_BUILD_TOOLS)
    add_test(
      NAME h264swdec.posix_delayed_output
      COMMAND
        ${CMAKE_COMMAND}
        -DDECODER=$<TARGET_FILE:h264swdec_decoder>
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/test/data/delayed_output.h264
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/delayed_output.yuv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/check_decode.cmake
    )
    set_tests_properties(h264swdec.posix_delayed_output PROPERTIES TIMEOUT 30)

    add_test(
      NAME h264swdec.posix_invalid_stream
      COMMAND
        ${CMAKE_COMMAND}
        -DDECODER=$<TARGET_FILE:h264swdec_decoder>
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/test/data/invalid.h264
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/invalid.yuv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/check_invalid.cmake
    )
    set_tests_properties(h264swdec.posix_invalid_stream PROPERTIES TIMEOUT 30)
  endif()
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

if(H264SWDEC_BUILD_TOOLS)
  install(TARGETS h264swdec_decoder
          RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

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
