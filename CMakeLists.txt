cmake_minimum_required(VERSION 3.0...3.10)
project(SunJava C)

# Builds a JDK-style tree (bin/, lib/, include/) for 32-bit Windows with MSVC:
#   cmake -S . -B out -G "Visual Studio 18 2026" -A Win32
#   cmake --build out --config Release --target INSTALL
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Only 32-bit MSVC is supported (use -A Win32); the VM and NSPR assume x86")
endif()

# shortcut: NSPR still comes from the sibling netscape checkout; it goes away once the Win32 md layer is rewritten.
set(NSPR_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../netscape/nspr" CACHE PATH "NSPR source tree")
if(NOT EXISTS "${NSPR_SOURCE_DIR}/src/md_WIN32.c")
  message(FATAL_ERROR "NSPR_SOURCE_DIR (${NSPR_SOURCE_DIR}) does not contain NSPR")
endif()
if(NOT CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/jdk" CACHE PATH "JDK image directory" FORCE)
endif()

add_definitions(-DXP_PC -DXP_WIN -DXP_WIN32 -DWIN32 -D_WINDOWS -D_X86_ -DHW_THREADS
  -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -DIN_JRT -DCOMPILER)
# The 1996 sources predate these MSVC defaults.
add_compile_options(/W1 /wd4996 /GS-)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DTRIMMED")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DTRACING")

set(gen "${CMAKE_BINARY_DIR}/generated")
include_directories(include include/_gen md-include runtime
  "${gen}" "${NSPR_SOURCE_DIR}/include" "${NSPR_SOURCE_DIR}/include/os")

# --- NSPR (Win32 native threads; list from nspr/src/makefile.win) ---
add_executable(gencfg "${NSPR_SOURCE_DIR}/include/gencfg.c")
add_custom_command(OUTPUT "${gen}/prcpucfg.h"
  COMMAND gencfg > "${gen}/prcpucfg.h" DEPENDS gencfg)
add_custom_target(nspr_headers DEPENDS "${gen}/prcpucfg.h")
file(MAKE_DIRECTORY "${gen}")

set(nspr_sources)
foreach(f file getopt linker log2 longlong page prabort prarena prdump prevent
    prhash prmcache prtime prmjtime prfile prfileio prnetdb prlog prprf prsystem
    prdtoa md_WIN md_WIN32 os_WIN prwindows prgcapi prmsgc hwthread hwsched hwmon)
  list(APPEND nspr_sources "${NSPR_SOURCE_DIR}/src/${f}.c")
endforeach()
add_library(nspr SHARED ${nspr_sources})
target_link_libraries(nspr wsock32 winmm user32 advapi32)
set_property(TARGET nspr APPEND PROPERTY COMPILE_DEFINITIONS _NSPR)
add_dependencies(nspr nspr_headers)

function(prefixed out dir)
  set(r)
  foreach(f ${ARGN})
    list(APPEND r "${dir}/${f}.c")
  endforeach()
  set(${out} ${r} PARENT_SCOPE)
endfunction()

# --- VM runtime DLL (javai.dll). javai.c loads it by name to resolve native stubs. ---
prefixed(runtime_sources runtime agent arena class date debug globals io math
  object opnames profiler runtime security string stubs system thread threads
  utf util zip classinitialize classresolver common_exceptions threadruntime
  compiler check_class check_code classloader classruntime gifdecoder exception
  exceptions filesystem interpreter)
prefixed(md_sources md dir_md gc_md io_md link_md math_md misc_md mon_md path_md
  ionet_md stubs_md trace_md agentthread compiler_md process_md properties_md
  socket_md system_md thread_md)

add_library(javai SHARED java/javai.c java/javai_dll.c ${runtime_sources}
  ${md_sources} java/javai.def)
add_dependencies(javai nspr_headers)
target_link_libraries(javai nspr wsock32 winmm user32 advapi32)

add_executable(java java/java_stub.c)
target_link_libraries(java javai)

# --- Standalone tools, each with its reduced runtime subset ---
prefixed(tool_runtime runtime arena classloader classresolver classresolver_stubs
  globals runtime_stubs utf util zip)
add_executable(javah javah/javah.c ${tool_runtime} md/path_md.c
  md/stubs_md.c md/system_md.c)
add_executable(javap javap/javap.c javap/console.c ${tool_runtime} runtime/check_class.c
  runtime/check_code.c runtime/decode.c md/path_md.c md/system_md.c
 )
foreach(t javah javap)
  add_dependencies(${t} nspr_headers)
  target_link_libraries(${t} nspr wsock32 winmm user32 advapi32)
endforeach()

# --- Smoke tests (ctest -C Release). Classes are prebuilt with JDK 1.1.5 javac (class file 45.3). ---
enable_testing()
configure_file(classsrc/classes.zip "${CMAKE_BINARY_DIR}/classes.zip" COPYONLY)
set(test_cp "${CMAKE_SOURCE_DIR}/tests/smoke;${CMAKE_BINARY_DIR}/classes.zip")
add_test(NAME hello COMMAND java -classpath "${test_cp}" Hello)
set_tests_properties(hello PROPERTIES PASS_REGULAR_EXPRESSION "VM: 2 4 0.*sum=9999900000")
add_test(NAME threads COMMAND java -classpath "${test_cp}" Threads)
set_tests_properties(threads PROPERTIES TIMEOUT 60
  PASS_REGULAR_EXPRESSION "count=80000 .*caught NPE.*gc ok, free=true")
add_test(NAME longs COMMAND java -classpath "${test_cp}" Longs)
set_tests_properties(longs PROPERTIES PASS_REGULAR_EXPRESSION "longs ok")
add_test(NAME props COMMAND java -classpath "${test_cp}" Props)
set_tests_properties(props PROPERTIES PASS_REGULAR_EXPRESSION
  "version=1\\.0\\.2.*vendor=Sun Microsystems Inc\\..*separator=\\\\.*home is dir=true.*child is dir=true.*classpath set=true.*os=Windows (NT|95)/[0-9]+\\.[0-9]+/x86")
add_test(NAME files COMMAND java -classpath "${test_cp}" Files)
set_tests_properties(files PROPERTIES PASS_REGULAR_EXPRESSION "files ok")
add_test(NAME javap COMMAND javap -c -classpath "${test_cp}" Hello)
set_tests_properties(javap PROPERTIES PASS_REGULAR_EXPRESSION "invokevirtual")
add_test(NAME javah COMMAND ${CMAKE_COMMAND} -DJAVAH=$<TARGET_FILE:javah>
  "-DCP=${test_cp}" -DOUT=${CMAKE_BINARY_DIR}/java_lang_Thread.h
  -P ${CMAKE_SOURCE_DIR}/tests/smoke/javah.cmake)

# --- JDK image ---
install(TARGETS java javah javap javai nspr RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES classsrc/classes.zip DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(DIRECTORY md-include/ DESTINATION include/win32 FILES_MATCHING PATTERN "*.h")
