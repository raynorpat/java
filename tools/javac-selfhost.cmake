cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${OUT}")
file(GLOB sources RELATIVE "${ROOT}/tests/vm" "${ROOT}/tests/vm/*.java")
execute_process(COMMAND "${JAVAC}" -nowarn -d "${OUT}" ${sources}
  WORKING_DIRECTORY "${ROOT}/tests/vm"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE output TIMEOUT 120)
if(NOT "${status}" STREQUAL "0" OR NOT "${output}" STREQUAL "")
  message(FATAL_ERROR "Self-hosted javac failed (${status}): ${output}")
endif()
include("${CASES}")
