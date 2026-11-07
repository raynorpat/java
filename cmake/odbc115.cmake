if(WIN32)
  add_library(JdbcOdbc SHARED src/win32/sun/jdbc/odbc115.c
    src/win32/sun/jdbc/odbcstubs.c src/win32/sun/jdbc/JdbcOdbc.def)
  target_link_libraries(JdbcOdbc javai odbc32)
  install(TARGETS JdbcOdbc RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
  get_filename_component(odbc_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME odbc-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${odbc_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:JdbcOdbc>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-JdbcOdbc115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
  add_test(NAME odbc-bridge115 COMMAND ${CMAKE_COMMAND}
    "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:JdbcOdbc>"
    "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}" "-DCONFIG=$<CONFIG>"
    "-DOUT=${CMAKE_BINARY_DIR}/odbc115/$<CONFIG>"
    -P "${CMAKE_SOURCE_DIR}/tests/odbc/odbc115.cmake")
  set_tests_properties(odbc-bridge115 PROPERTIES TIMEOUT 120)
endif()
