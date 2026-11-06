set(math_dir src/share/java/math)
set(bn_dir ${math_dir}/bn-1.1)
set(math_sources ${math_dir}/BigInteger.c ${bn_dir}/bn.c ${bn_dir}/bn16.c
  ${bn_dir}/bninit16.c ${bn_dir}/lbn16.c ${bn_dir}/lbnmem.c
  ${bn_dir}/prime.c ${bn_dir}/sieve.c ${bn_dir}/legal.c)
if(WIN32)
  set(math_exports ${math_dir}/math.def)
endif()
add_library(math SHARED ${math_sources} ${math_exports})
target_include_directories(math PRIVATE ${bn_dir})
target_link_libraries(math javai)
# Sun's release DLL retains bnlib assertions, as does its debug DLL.
if(MSVC)
  target_compile_options(math PRIVATE /UNDEBUG)
else()
  target_compile_options(math PRIVATE -UNDEBUG -fvisibility=hidden)
endif()
install(TARGETS math RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES ${bn_dir}/COPYING ${bn_dir}/legal.c ${bn_dir}/PROVENANCE.md
  DESTINATION legal/bnlib-1.1)
add_test(NAME vm-MathLibrary115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:math>"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}" -DCLASS=MathLibrary115
  -P "${CMAKE_SOURCE_DIR}/tests/vm/math-resource115.cmake")
if(WIN32)
  get_filename_component(math_msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME math-exports115 COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${math_msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:math>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-math115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
endif()
