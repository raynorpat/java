# IJG release 6, unmodified sources; Sun's sole public entry is the stub.
set(jpeg6_dir third_party/jpeg-6)
configure_file(${jpeg6_dir}/jconfig.doc "${CMAKE_BINARY_DIR}/jpeg6-config/jconfig.h" COPYONLY)
set(jpeg6_names jcomapi jutils jerror jmemmgr jmemnobs
  jcapimin jcapistd jctrans jcparam jdatadst jcinit jcmaster jcmarker jcmainct
  jcprepct jccoefct jccolor jcsample jchuff jcphuff jcdctmgr jfdctfst jfdctflt jfdctint
  jdapimin jdapistd jdtrans jdatasrc jdmaster jdinput jdmarker jdhuff jdphuff
  jdmainct jdcoefct jdpostct jddctmgr jidctfst jidctflt jidctint jidctred
  jdsample jdcolor jquant1 jquant2 jdmerge)
set(jpeg6_sources)
foreach(source ${jpeg6_names})
  list(APPEND jpeg6_sources ${jpeg6_dir}/${source}.c)
endforeach()
add_library(ijg6 STATIC ${jpeg6_sources})
target_include_directories(ijg6 PUBLIC ${jpeg6_dir} "${CMAKE_BINARY_DIR}/jpeg6-config")
set_target_properties(ijg6 PROPERTIES POSITION_INDEPENDENT_CODE ON)
set(jpeg_exports)
if(WIN32)
  set(jpeg_exports src/share/sun/awt/image/jpeg.def)
else()
  target_compile_options(ijg6 PRIVATE -fvisibility=hidden)
endif()
add_library(jpeg SHARED src/share/sun/awt/image/jpegdecoder.c
  src/share/sun/awt/image/jpegstubs.c ${jpeg_exports})
target_link_libraries(jpeg javai ijg6)
if(NOT WIN32)
  target_compile_options(jpeg PRIVATE -fvisibility=hidden)
endif()
install(TARGETS jpeg RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES ${jpeg6_dir}/README ${jpeg6_dir}/PROVENANCE.md DESTINATION legal/jpeg-6)
add_library(jpeg115probe SHARED tests/native/jpeg115-bridge.c)
target_link_libraries(jpeg115probe jpeg)
add_executable(jpeg115-fixtures tests/native/jpeg115-fixtures.c)
target_link_libraries(jpeg115-fixtures ijg6)
add_test(NAME vm-JpegLibrary115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DGENERATOR=$<TARGET_FILE:jpeg115-fixtures>"
  "-DLIBDIR=$<TARGET_FILE_DIR:jpeg115probe>" "-DROOT=${CMAKE_SOURCE_DIR}"
  "-DJDK=${JDK115_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/jpeg115-fixtures/$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tests/vm/jpeg115.cmake")
if(WIN32)
  get_filename_component(jpeg_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME jpeg-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${jpeg_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:jpeg>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-jpeg115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
endif()
