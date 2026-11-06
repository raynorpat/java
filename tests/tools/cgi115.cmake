cmake_minimum_required(VERSION 3.0...3.10)
if(WIN32)
  set(ENV{PATH} "${BIN};$ENV{PATH}")
else()
  set(ENV{PATH} "${BIN}:$ENV{PATH}")
endif()
set(ENV{QUERY_STRING} "ping")
set(ENV{REQUEST_METHOD} "GET")
set(ENV{CONTENT_LENGTH} "0")
execute_process(COMMAND "${TOOL}" ignored-argument RESULT_VARIABLE result
  OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT "${result}" STREQUAL "0" OR NOT "${errors}" STREQUAL "" OR
    NOT "${output}" STREQUAL "Status: 200 OK\nContent-type: application/octet-stream\nContent-length: 0\n\n")
  message(FATAL_ERROR "CGI ping: ${result}, [${output}], [${errors}]")
endif()
set(ENV{PATH} "")
execute_process(COMMAND "${TOOL}" RESULT_VARIABLE result
  OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT "${result}" STREQUAL "1" OR NOT "${errors}" STREQUAL "" OR
    NOT "${output}" STREQUAL "Status: 500 Server Error: interpreter execution failure\nContent-type: text/plain\n\ninterpreter execution failure")
  message(FATAL_ERROR "CGI exec failure: ${result}, [${output}], [${errors}]")
endif()
