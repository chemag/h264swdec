if(NOT DEFINED DECODER OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
  message(FATAL_ERROR "DECODER, INPUT, and OUTPUT are required")
endif()

file(REMOVE "${OUTPUT}")
execute_process(
  COMMAND "${DECODER}" "${INPUT}" "${OUTPUT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE standard_output
  ERROR_VARIABLE standard_error
)

if(NOT result EQUAL 1)
  message(FATAL_ERROR
          "decoder returned ${result}, expected 1\nstdout:\n${standard_output}\nstderr:\n${standard_error}")
endif()

string(FIND "${standard_error}" "Decoder rejected the input stream" diagnostic)
if(diagnostic EQUAL -1)
  message(FATAL_ERROR "missing clean rejection diagnostic:\n${standard_error}")
endif()

file(REMOVE "${OUTPUT}")
