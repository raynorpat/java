cmake_minimum_required(VERSION 3.0...3.10)
execute_process(COMMAND "${JAVA_G}" -version RESULT_VARIABLE status
  OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
if(NOT "${status}" STREQUAL "0" OR NOT "${output}${errors}" MATCHES "version \"1\\.1\\.5\"")
  message(FATAL_ERROR "jdb's java_g launcher is unavailable: ${status} ${output}${errors}")
endif()
