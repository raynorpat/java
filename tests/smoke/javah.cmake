# Runs javah and checks the generated headers (javah only writes files).
function(javah_check class pattern)
  file(REMOVE "${OUT}")
  execute_process(COMMAND "${JAVAH}" -classpath "${CP}" -o "${OUT}" ${class} RESULT_VARIABLE rc)
  file(READ "${OUT}" header)
  if(NOT rc EQUAL 0 OR NOT header MATCHES "${pattern}")
    message(FATAL_ERROR "javah ${class}: rc=${rc}, expected '${pattern}' in:\n${header}")
  endif()
endfunction()

javah_check(java.lang.Thread "struct Classjava_lang_Thread")
# Float constants are read from the class's float slots; non-finite values spell Inf/NaN.
javah_check(java.lang.Float "MAX_VALUE\t3\\.40282e\\+38f\n")
javah_check(java.lang.Float "POSITIVE_INFINITY\tInff\n")
javah_check(java.lang.Double "NaN\tNaND\n")
