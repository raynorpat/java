# Sun 1.1.5 ZIP companion library: legacy native stubs, zlib 1.0.4.
set(zip115_exports)
if(WIN32)
  set(zip115_exports src/share/java/zip/zip.def)
endif()
add_library(zip SHARED src/share/java/zip/zip.c
  third_party/zlib-1.0.4/deflate.c third_party/zlib-1.0.4/trees.c
  third_party/zlib-1.0.4/crc32.c ${zip115_exports})
target_link_libraries(zip javai zlib115)
if(MSVC)
  target_compile_options(zip PRIVATE /UDEBUG)
else()
  target_compile_options(zip PRIVATE -UDEBUG)
  set_target_properties(zlib115 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
install(TARGETS zip RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES third_party/zlib-1.0.4/README third_party/zlib-1.0.4/zlib.h
  third_party/zlib-1.0.4/ORIGIN.md DESTINATION legal/zlib-1.0.4)
add_test(NAME vm-ZipLibrary115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:zip>" "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
  -P "${CMAKE_SOURCE_DIR}/tools/zip115.cmake")
if(WIN32)
  get_filename_component(zip115_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME zip-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${zip115_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:zip>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-zip115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
endif()

add_test(NAME tools-zip115-jar COMMAND ${CMAKE_COMMAND}
  "-DJAR=$<TARGET_FILE:jar>" "-DLIBDIR=$<TARGET_FILE_DIR:zip>"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/zip115-jar/$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tools/zip115-jar.cmake")
