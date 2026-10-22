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

file(SIZE "${OUTPUT}" output_size)
if(NOT output_size EQUAL 0)
  message(FATAL_ERROR "invalid stream produced ${output_size} output bytes")
endif()

string(FIND "${standard_error}" "No pictures decoded" diagnostic)
if(diagnostic EQUAL -1)
  message(FATAL_ERROR "missing clean failure diagnostic:\n${standard_error}")
endif()
