cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${OUT}")
execute_process(COMMAND "${JAVAC}" -d "${OUT}" Suspend115.java
  WORKING_DIRECTORY "${ROOT}/tests/debugger"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT "${status}" STREQUAL "0")
  message(FATAL_ERROR "Suspend test compile failed: ${output}${errors}")
endif()
if(WIN32)
  set(cp "${OUT};${JDK}/lib/classes.zip")
else()
  set(cp "${OUT}:${JDK}/lib/classes.zip")
endif()
execute_process(COMMAND "${JAVA}" -classpath "${cp}" Suspend115
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
if(NOT "${status}" STREQUAL "0" OR
    NOT output MATCHES "repeated suspension cleared by one resume" OR NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "Suspend test failed: ${status} ${output}${errors}")
endif()
