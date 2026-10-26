set(H264SWDEC_FUZZ_SEEDS
    ${PROJECT_SOURCE_DIR}/test/data/delayed_output.h264
    ${PROJECT_SOURCE_DIR}/test/data/invalid.h264
    ${PROJECT_SOURCE_DIR}/test/data/unsupported_b_slice.h264)
file(GLOB H264SWDEC_FUZZ_REGRESSIONS CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/regressions/*.h264)

function(h264swdec_add_fuzzer name)
  add_executable(${name} ${name}.c)
  target_compile_features(${name} PRIVATE c_std_99)
  target_link_libraries(${name} PRIVATE H264SwDec::h264swdec)
  target_link_options(${name} PRIVATE -fsanitize=fuzzer)
  if(BUILD_TESTING)
    # Replays the seeds and every saved crash input once, without mutation.
    add_test(NAME h264swdec.fuzz.${name}
             COMMAND ${name} -runs=0 ${H264SWDEC_FUZZ_SEEDS}
                     ${H264SWDEC_FUZZ_REGRESSIONS})
    set_tests_properties(h264swdec.fuzz.${name} PROPERTIES TIMEOUT 120)
  endif()
endfunction()

h264swdec_add_fuzzer(h264swdec_annexb_fuzzer)
h264swdec_add_fuzzer(h264swdec_rtc_fuzzer)
