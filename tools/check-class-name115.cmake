# Verify that both directory and ZIP imports reject a mismatched internal name.
cmake_minimum_required(VERSION 3.0...3.10)
file(MAKE_DIRECTORY "${WORK}")
configure_file("${ROOT}/tests/vm/Hello115.class" "${WORK}/Wrong115.class" COPYONLY)
if(WIN32)
    set(separator ";")
else()
    set(separator ":")
endif()
foreach(entry "${WORK}" "${ROOT}/tests/vm/resources/misnamed115.zip")
    execute_process(COMMAND "${JAVA}" -classpath
        "${entry}${separator}${JDK115_DIR}/lib/classes.zip" Wrong115
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 30)
    set(actual "${output}${error}")
    string(REPLACE "\r" "" actual "${actual}")
    if(NOT result EQUAL 1 OR NOT actual STREQUAL "Can't find class Wrong115\n")
        message(FATAL_ERROR "Mismatched class accepted from ${entry}: ${result}: ${actual}")
    endif()
endforeach()
