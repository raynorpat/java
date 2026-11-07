cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${OUT}")
if(LIBDIR)
  set(ENV{PATH} "${LIBDIR};$ENV{PATH}")
endif()
execute_process(COMMAND "${JAVAC}" -d "${OUT}" "${CLASS}.java"
  WORKING_DIRECTORY "${ROOT}/tests/awt"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "AWT test compile failed: ${output}${errors}")
endif()
execute_process(COMMAND "${JAVA}" -classpath "${OUT};${JDK}/lib/classes.zip" "${CLASS}"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT status EQUAL 0 OR NOT output MATCHES "passed" OR errors MATCHES "Exception|Error")
  message(FATAL_ERROR "AWT test failed (${status}): ${output}${errors}")
endif()
message(STATUS "${output}")
