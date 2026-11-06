cmake_minimum_required(VERSION 3.0...3.10)
project(SunJava C)
set(JDK115_DIR "${CMAKE_SOURCE_DIR}/vendor/jdk1.1.5" CACHE PATH "Sun JDK 1.1.5 reference tree")
if(NOT EXISTS "${JDK115_DIR}/lib/classes.zip")
  message(FATAL_ERROR "JDK115_DIR does not contain lib/classes.zip - run tools/vendor.cmake")
endif()

# Builds a JDK-style tree (bin/, lib/, include/):
#   Windows: cmake -S . -B out -G "Visual Studio 18 2026" -A Win32
#   Unix:    CC="cc -m32" cmake -S . -B out -DCMAKE_BUILD_TYPE=Release
#   then:    cmake --build out --config Release --target install
# The VM stores pointers in 32-bit Java slots, so the build must be 32-bit.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "The VM is 32-bit only (Windows: -A Win32; Unix: CC=\"cc -m32\")")
endif()
if(NOT CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/jdk" CACHE PATH "JDK image directory" FORCE)
endif()

# The sources are C89 (and rely on its implicit declarations).
set(CMAKE_C_STANDARD 90)
add_definitions(-DIN_JRT -DCOMPILER)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DTRIMMED")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DTRACING")

if(WIN32)
  if(NOT MSVC)
    message(FATAL_ERROR "On Windows only MSVC is supported")
  endif()
  set(plat win32)
  set(sep ";")
  add_definitions(-DXP_PC -DXP_WIN -DXP_WIN32 -DWIN32 -D_WINDOWS -D_X86_
    -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
  # The 1996 sources predate these MSVC defaults.
  add_compile_options(/W1 /wd4996 /GS-)
  set(sys_libs wsock32 winmm user32 advapi32)
else()
  set(plat unix)
  set(sep ":")
  # C89 mode hides POSIX; ask for it explicitly.
  # 64-bit file offsets and inodes, or 32-bit stat() fails on modern filesystems.
  add_definitions(-DXP_UNIX -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -D_FILE_OFFSET_BITS=64)
  if(APPLE)
    add_definitions(-D_DARWIN_C_SOURCE)
  endif()
  find_package(Threads REQUIRED)
  set(sys_libs ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)
  # Lay the build tree out like the JDK: java.home is the build directory.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
  endif()
endif()
include_directories(include include/_gen md-include/${plat} md-include/share runtime)

function(prefixed out dir)
  set(r)
  foreach(f ${ARGN})
    list(APPEND r "${dir}/${f}.c")
  endforeach()
  set(${out} ${r} PARENT_SCOPE)
endfunction()

# --- VM runtime DLL (javai.dll), holding the built-in native stubs ---
prefixed(runtime_sources runtime agent arena class date debug globals io math
  object opnames profiler runtime security string stubs system thread threads
  utf util zip classinitialize classresolver common_exceptions threadruntime
  compiler check_class check_code classloader classruntime gifdecoder exception
  exceptions filesystem interpreter gc finalize monitor monitor_cache)
prefixed(md_share md/share agentthread compiler_md io_md ionet_md math_md
  process_md properties_md socket_md stubs_md)
prefixed(md_plat md/${plat} dir_md gc_md link_md misc_md mon_md path_md
  system_md thread_md)

if(WIN32)
  list(APPEND md_plat java/javai.def)
else()
  list(APPEND md_plat md/unix/net_md.c)
endif()
add_library(javai SHARED java/javai.c java/javai_dll.c ${runtime_sources}
  ${md_share} ${md_plat})
target_link_libraries(javai ${sys_libs})

add_executable(java java/java_stub.c)
target_link_libraries(java javai)

# --- Standalone tools, each with its reduced runtime subset ---
prefixed(tool_runtime runtime arena classloader classresolver classresolver_stubs
  globals runtime_stubs utf util zip)
add_executable(javah javah/javah.c ${tool_runtime} md/${plat}/path_md.c
  md/share/stubs_md.c md/${plat}/system_md.c)
add_executable(javap javap/javap.c javap/console.c ${tool_runtime} runtime/check_class.c
  runtime/check_code.c runtime/decode.c md/${plat}/path_md.c md/${plat}/system_md.c)
foreach(t javah javap)
  target_link_libraries(${t} ${sys_libs})
endforeach()

# --- Smoke tests (ctest -C Release). Classes are prebuilt with JDK 1.1.5 javac (class file 45.3). ---
enable_testing()
include(CMakeParseArguments)
function(java_test)
  cmake_parse_arguments(JT "" "NAME;CLASS" "ARGS;VM_ARGS" ${ARGN})
  add_test(NAME ${JT_NAME} COMMAND ${CMAKE_COMMAND}
    "-DJAVA=$<TARGET_FILE:java>"
    "-DCP=${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip"
    "-DCLASS=${JT_CLASS}" "-DARGS=${JT_ARGS}" "-DVM_ARGS=${JT_VM_ARGS}"
    "-DEXPECTED=${CMAKE_SOURCE_DIR}/tests/vm/${JT_CLASS}.expected"
    -P ${CMAKE_SOURCE_DIR}/tools/run-java-test.cmake)
  set_tests_properties(${JT_NAME} PROPERTIES TIMEOUT 40 LABELS "jdk115")
endfunction()
# Known output mismatch until Task 4. Keep failure visible; baseline uses -LE jdk115.
java_test(NAME vm-hello115 CLASS Hello115)
configure_file(classsrc/classes.zip "${CMAKE_BINARY_DIR}/classes.zip" COPYONLY)
set(test_cp "${CMAKE_SOURCE_DIR}/tests/smoke${sep}${CMAKE_BINARY_DIR}/classes.zip")
add_test(NAME hello COMMAND java -classpath "${test_cp}" Hello)
set_tests_properties(hello PROPERTIES PASS_REGULAR_EXPRESSION "VM: 2 4 0.*sum=9999900000")
add_test(NAME stress COMMAND java -classpath "${test_cp}" Stress)
set_tests_properties(stress PROPERTIES TIMEOUT 120 PASS_REGULAR_EXPRESSION "stress ok finalizers=true")
add_test(NAME monitors COMMAND java -classpath "${test_cp}" Monitors)
set_tests_properties(monitors PROPERTIES TIMEOUT 60 PASS_REGULAR_EXPRESSION "monitors ok")
add_test(NAME threads COMMAND java -classpath "${test_cp}" Threads)
set_tests_properties(threads PROPERTIES TIMEOUT 60
  PASS_REGULAR_EXPRESSION "count=80000 .*caught NPE.*gc ok, free=true")
add_test(NAME longs COMMAND java -classpath "${test_cp}" Longs)
set_tests_properties(longs PROPERTIES PASS_REGULAR_EXPRESSION "longs ok")
add_test(NAME maths COMMAND java -classpath "${test_cp}" Maths)
set_tests_properties(maths PROPERTIES PASS_REGULAR_EXPRESSION "maths ok")
add_test(NAME dates COMMAND java -classpath "${test_cp}" Dates)
set_tests_properties(dates PROPERTIES PASS_REGULAR_EXPRESSION "dates ok")
add_test(NAME props COMMAND java -classpath "${test_cp}" Props)
if(WIN32)
  set(props_sep "\\\\")
  set(props_os "Windows (NT|95)/[0-9]+\\.[0-9]+")
else()
  set(props_sep "/")
  set(props_os "[A-Za-z]+/[0-9]+\\.[0-9]+[^/]*")
endif()
set_tests_properties(props PROPERTIES PASS_REGULAR_EXPRESSION
  "version=1\\.0\\.2.*vendor=Sun Microsystems Inc\\..*separator=${props_sep}.*home is dir=true.*child is dir=true.*classpath set=true.*os=${props_os}/x86")
add_test(NAME files COMMAND java -classpath "${test_cp}" Files)
set_tests_properties(files PROPERTIES PASS_REGULAR_EXPRESSION "files ok")
add_test(NAME javap COMMAND javap -c -classpath "${test_cp}" Hello)
set_tests_properties(javap PROPERTIES PASS_REGULAR_EXPRESSION "invokevirtual")
add_test(NAME javah COMMAND ${CMAKE_COMMAND} -DJAVAH=$<TARGET_FILE:javah>
  "-DCP=${test_cp}" -DOUT=${CMAKE_BINARY_DIR}/java_lang_Thread.h
  -P ${CMAKE_SOURCE_DIR}/tests/smoke/javah.cmake)

# A user native library built the JDK 1.0 way: javah header + stubs, linked to javai.lib.
# It lands in <java.home>\bin (Windows) or <java.home>/lib (Unix) of the build tree.
set(native_cp "${CMAKE_SOURCE_DIR}/tests/native${sep}${CMAKE_BINARY_DIR}/classes.zip")
set(native_gen "${CMAKE_BINARY_DIR}/native-gen")
add_custom_command(OUTPUT "${native_gen}/NativeHello.h" "${native_gen}/NativeHello.c"
  COMMAND javah -classpath "${native_cp}" -d "${native_gen}" NativeHello
  COMMAND javah -stubs -classpath "${native_cp}" -d "${native_gen}" NativeHello
  DEPENDS javah tests/native/NativeHello.class)
add_library(nativehello SHARED tests/native/nativehello.c "${native_gen}/NativeHello.c")
target_include_directories(nativehello PRIVATE "${native_gen}")
target_link_libraries(nativehello javai)
foreach(config RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
  set_target_properties(nativehello PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
add_test(NAME native COMMAND java -classpath "${native_cp}" NativeHello)
set_tests_properties(native PROPERTIES PASS_REGULAR_EXPRESSION "native add=42 greet=hello java")

# --- JDK image ---
install(TARGETS java javah javap javai RUNTIME DESTINATION bin LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
install(FILES classsrc/classes.zip DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(DIRECTORY md-include/${plat}/ md-include/share/ DESTINATION include/${plat}
  FILES_MATCHING PATTERN "*.h")
