cmake_minimum_required(VERSION 3.0...3.10)
function(require_success label)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "${label}: ${result}\n${output}${errors}")
  endif()
endfunction()
file(MAKE_DIRECTORY "${OUT}/overrides" "${OUT}/generated" "${OUT}/tests")
execute_process(COMMAND "${PYTHON}" "${ROOT}/tools/motif118-peers.py" prepare
  "${JDK}/lib/classes.zip" "${SOLARIS}" "${OUT}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
require_success("Prepare Motif closure")
if(WIN32)
  set(sep ";")
else()
  set(sep ":")
endif()
set(cp "${JDK}/lib/classes.zip${sep}${OUT}/motif-raw.zip")
execute_process(COMMAND "${JAVAC}" -classpath "${cp}" -d "${OUT}/overrides" InputContext.java
  WORKING_DIRECTORY "${ROOT}/src/unix/sun/awt/im"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 60)
require_success("Compile input-context adaptation")
execute_process(COMMAND "${PYTHON}" "${ROOT}/tools/motif118-peers.py" finish
  "${JDK}/lib/classes.zip" "${SOLARIS}" "${OUT}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
require_success("Validate Motif closure")
set(cp "${JDK}/lib/classes.zip${sep}${OUT}/motif-peers.zip")
file(STRINGS "${OUT}/peer-classes.txt" classes)
file(STRINGS "${OUT}/native-classes.txt" native_classes)
execute_process(COMMAND "${JAVAH}" -classpath "${cp}" -d "${OUT}/generated" ${classes}
  java.awt.Component java.awt.Button java.awt.Canvas java.awt.Window java.awt.Frame java.awt.Checkbox java.awt.Scrollbar java.awt.TextArea java.awt.ScrollPane
  java.awt.Font sun.awt.FontDescriptor java.awt.Insets java.awt.Color java.awt.Cursor java.awt.AWTEvent
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
require_success("Generate baseline-layout peer headers")
execute_process(COMMAND "${JAVAH}" -stubs -classpath "${cp}" -d "${OUT}/generated" ${native_classes}
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
require_success("Generate native peer bindings")
set(actual)
foreach(class ${native_classes})
  string(REPLACE "." "_" filename "${class}")
  file(READ "${OUT}/generated/${filename}.c" stub)
  string(REGEX MATCHALL "Java_sun_awt_motif_[A-Za-z0-9_]+_stub" names "${stub}")
  list(APPEND actual ${names})
endforeach()
list(REMOVE_DUPLICATES actual)
list(SORT actual)
file(STRINGS "${ROOT}/tools/solaris118-awt-native-entries.txt" expected REGEX "^Java_sun_awt_motif_")
list(SORT expected)
if(NOT "${actual}" STREQUAL "${expected}")
  message(FATAL_ERROR "Generated Motif bindings do not match Solaris entries")
endif()
execute_process(COMMAND "${PYTHON}" "${ROOT}/tools/motif118-stubs.py"
  "${ROOT}/tools/motif118-implemented.txt" "${OUT}/generated" "${OUT}/awt-stubs.c"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
require_success("Select reconstructed native bindings")
execute_process(COMMAND "${JAVAC}" -classpath "${cp}" -d "${OUT}/tests"
  PeerLink115.java InputContextTest115.java
  WORKING_DIRECTORY "${ROOT}/tests/awt118"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 60)
require_success("Compile peer boundary fixtures")
file(WRITE "${OUT}/bindings.ok" "76 classes, 199 Motif native bindings, baseline headers\n")
