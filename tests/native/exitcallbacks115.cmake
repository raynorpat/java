cmake_minimum_required(VERSION 3.0...3.10)
execute_process(COMMAND "${PROBE}" RESULT_VARIABLE status
  OUTPUT_VARIABLE actual ERROR_VARIABLE errors TIMEOUT 30)
file(READ "${CMAKE_CURRENT_LIST_DIR}/exitcallbacks115.expected" expected)
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
if(NOT "${status}" STREQUAL "0" OR NOT "${actual}" STREQUAL "${expected}" OR
   NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "Exit callbacks differ from Sun: ${status} ${actual}${errors}")
endif()
