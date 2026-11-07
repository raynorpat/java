cmake_minimum_required(VERSION 3.0...3.10)
set(image "${BUILD}/motif118/installed")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD}" --prefix "${image}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT "${result}" STREQUAL "0")
  message(FATAL_ERROR "Install: ${result}\n${output}${errors}")
endif()
file(MAKE_DIRECTORY "${image}/test")
unset(ENV{CLASSPATH})
unset(ENV{JAVA_HOME})
unset(ENV{LD_LIBRARY_PATH})
execute_process(COMMAND "${image}/bin/javac" -d . "${ROOT}/tests/awt118/Installed115.java"
  WORKING_DIRECTORY "${image}/test" RESULT_VARIABLE result
  OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT "${result}" STREQUAL "0")
  message(FATAL_ERROR "Installed compiler: ${result}\n${output}${errors}")
endif()
execute_process(COMMAND xvfb-run -a -s "-screen 0 800x600x24 -nolisten tcp"
  "${image}/bin/java" Installed115 WORKING_DIRECTORY "${image}/test"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT "${result}" STREQUAL "0" OR NOT output MATCHES "installed Motif passed")
  message(FATAL_ERROR "Installed toolkit: ${result}\n${output}${errors}")
endif()
message(STATUS "${output}")
