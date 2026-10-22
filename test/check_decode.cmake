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

if(NOT result EQUAL 0)
  message(FATAL_ERROR
          "decoder failed (${result})\nstdout:\n${standard_output}\nstderr:\n${standard_error}")
endif()

file(SIZE "${OUTPUT}" actual_size)
if(NOT actual_size EQUAL 646272)
  message(FATAL_ERROR "output size ${actual_size}, expected 646272")
endif()

file(SHA256 "${OUTPUT}" actual_sha256)
set(expected_sha256
    "76a23816c275966d5eeeaaf279626a916a3aace808a0164cbb90e978b12fc151")
if(NOT actual_sha256 STREQUAL expected_sha256)
  message(FATAL_ERROR
          "output SHA-256 ${actual_sha256}, expected ${expected_sha256}")
endif()

string(FIND "${standard_error}" "Decoded 17 pictures; wrote 17 pictures;" summary)
if(summary EQUAL -1)
  message(FATAL_ERROR "missing picture summary in stderr:\n${standard_error}")
endif()
