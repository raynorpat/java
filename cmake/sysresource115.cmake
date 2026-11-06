set(sysresource_dir src/share/sun/net/www/protocol/systemresource)
if(WIN32)
  set(sysresource_exports ${sysresource_dir}/sysresource.def)
endif()
add_library(sysresource SHARED ${sysresource_dir}/SystemResourceManager.c ${sysresource_exports})
target_link_libraries(sysresource javai)
install(TARGETS sysresource RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
foreach(resource_kind zip file)
  if(resource_kind STREQUAL zip)
    set(resource_path "${CMAKE_SOURCE_DIR}/tests/vm/resources115.zip")
  else()
    set(resource_path "${CMAKE_SOURCE_DIR}/tests/vm/resources")
  endif()
  add_test(NAME vm-ResourceLibrary115-${resource_kind} COMMAND ${CMAKE_COMMAND}
    "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:sysresource>"
    "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
    "-DRESOURCE=${resource_path}" -DCLASS=ResourceLibrary115
    -P "${CMAKE_SOURCE_DIR}/tests/vm/math-resource115.cmake")
endforeach()
if(WIN32)
  get_filename_component(sysresource_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME sysresource-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${sysresource_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:sysresource>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-sysresource115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
endif()
