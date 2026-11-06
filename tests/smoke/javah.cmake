# Runs javah on java.lang.Thread and checks the generated header (javah only writes files).
file(REMOVE "${OUT}")
execute_process(COMMAND "${JAVAH}" -classpath "${CP}" -o "${OUT}" java.lang.Thread RESULT_VARIABLE rc)
file(READ "${OUT}" header)
if(NOT rc EQUAL 0 OR NOT header MATCHES "struct Classjava_lang_Thread")
  message(FATAL_ERROR "javah failed (rc=${rc})")
endif()
