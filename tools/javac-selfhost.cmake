cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${OUT}")
file(GLOB sources RELATIVE "${ROOT}/tests/vm" "${ROOT}/tests/vm/*.java")
execute_process(COMMAND "${JAVAC}" -nowarn -d "${OUT}" ${sources}
  WORKING_DIRECTORY "${ROOT}/tests/vm"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE output TIMEOUT 120)
if(NOT "${status}" STREQUAL "0" OR NOT "${output}" STREQUAL "")
  message(FATAL_ERROR "Self-hosted javac failed (${status}): ${output}")
endif()
find_program(patch_python NAMES python3 python)
if(NOT patch_python)
  message(FATAL_ERROR "Python is required to patch the circularity class fixtures")
endif()
execute_process(COMMAND "${patch_python}" "${ROOT}/tests/vm/patch-circularity115.py" "${OUT}"
  RESULT_VARIABLE patch_status OUTPUT_VARIABLE patch_output ERROR_VARIABLE patch_output)
if(NOT "${patch_status}" STREQUAL "0")
  message(FATAL_ERROR "Circularity fixture patch failed (${patch_status}): ${patch_output}")
endif()
include("${CASES}")
