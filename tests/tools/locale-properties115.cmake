cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${OUT}")
set(ENV{PATH} "${LIBDIR};$ENV{PATH}")
execute_process(COMMAND "${JAVAC}" -d "${OUT}" LocaleProperties115.java
  WORKING_DIRECTORY "${ROOT}/tests/vm" RESULT_VARIABLE compiled OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "Locale fixture compilation failed: ${output}${errors}")
endif()
foreach(mode defaults overrides)
  set(options)
  set(arguments)
  if(mode STREQUAL overrides)
    set(options -Duser.language=ja -Duser.region=JP -Dfile.encoding=SJIS -Duser.timezone=JST)
    set(arguments overrides)
  endif()
  execute_process(COMMAND "${JDK}/bin/java.exe" "-Djava.home=${JDK}" ${options}
    -classpath "${OUT};${JDK}/lib/classes.zip" LocaleProperties115 ${arguments}
    RESULT_VARIABLE oracle_status OUTPUT_VARIABLE oracle_output ERROR_VARIABLE oracle_errors TIMEOUT 30)
  if(NOT oracle_status EQUAL 0 OR oracle_errors MATCHES "Exception|Error")
    message(FATAL_ERROR "Sun locale fixture failed: ${oracle_output}${oracle_errors}")
  endif()
  execute_process(COMMAND "${JAVA}" ${options}
    -classpath "${OUT};${JDK}/lib/classes.zip" LocaleProperties115 ${arguments}
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
  if(NOT status EQUAL 0 OR errors MATCHES "Exception|Error" OR NOT output STREQUAL oracle_output)
    message(FATAL_ERROR "Windows locale mismatch (${mode}): ours=${output}${errors} Sun=${oracle_output}${oracle_errors}")
  endif()
  message(STATUS "${output}")
endforeach()
