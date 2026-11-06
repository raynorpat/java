set(mmedia115_exports)
if(WIN32)
  set(mmedia115_exports src/share/sun/audio/mmedia.def)
endif()
add_library(mmedia SHARED src/share/sun/audio/AudioDevice.c src/share/sun/audio/ulaw.c
  src/${plat}/sun/audio/audio_md.c ${mmedia115_exports})
target_include_directories(mmedia PRIVATE src/share/sun/audio)
target_link_libraries(mmedia javai ${sys_libs})
install(TARGETS mmedia RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
add_executable(mmedia115-decode tests/native/mmedia115-decode.c src/share/sun/audio/ulaw.c)
target_include_directories(mmedia115-decode PRIVATE src/share/sun/audio)
add_test(NAME mmedia115-decode COMMAND mmedia115-decode)
add_executable(mmedia115-device tests/native/mmedia115-device.c)
target_link_libraries(mmedia115-device ${CMAKE_DL_LIBS})
add_test(NAME mmedia115-device COMMAND mmedia115-device "$<TARGET_FILE:mmedia>")
set_tests_properties(mmedia115-device PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
add_test(NAME vm-AudioLibrary115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:mmedia>" "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
  -P "${CMAKE_SOURCE_DIR}/tools/mmedia115.cmake")
if(WIN32)
  get_filename_component(mmedia115_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME mmedia-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${mmedia115_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:mmedia>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-mmedia115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
endif()
