cmake_minimum_required(VERSION 3.0...3.10)
project(SunJava C)
if(WIN32)
  enable_language(CXX)
endif()
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
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DTRACING -DJCOV")

if(WIN32)
  if(NOT MSVC)
    message(FATAL_ERROR "On Windows only MSVC is supported")
  endif()
  set(plat win32)
  set(file_hpi_sources src/win32/java/runtime/file_md.c)
  set(sep ";")
  add_definitions(-DXP_PC -DXP_WIN -DXP_WIN32 -DWIN32 -D_WINDOWS -D_X86_
    -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
  # The 1996 sources predate these MSVC defaults.
  add_compile_options(/W1 /wd4996 /GS-)
  set(sys_libs wsock32 winmm user32 advapi32)
else()
  set(plat unix)
  set(JDK115_UNIX_THREADS "pthreads" CACHE STRING "Unix thread backend: pthreads or cthreads")
  set_property(CACHE JDK115_UNIX_THREADS PROPERTY STRINGS pthreads cthreads)
  if(JDK115_UNIX_THREADS STREQUAL "cthreads")
    add_definitions(-DJDK115_CTHREADS)
  elseif(NOT JDK115_UNIX_THREADS STREQUAL "pthreads")
    message(FATAL_ERROR "Unknown JDK115_UNIX_THREADS backend")
  endif()
  set(sep ":")
  # C89 mode hides POSIX; ask for it explicitly.
  # 64-bit file offsets and inodes, or 32-bit stat() fails on modern filesystems.
  add_definitions(-DXP_UNIX -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -D_FILE_OFFSET_BITS=64)
  if(APPLE)
    add_definitions(-D_DARWIN_C_SOURCE)
  endif()
  if(JDK115_UNIX_THREADS STREQUAL "pthreads")
    find_package(Threads REQUIRED)
    set(sys_libs ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)
  else()
    set(sys_libs m)
  endif()
  # Lay the build tree out like the JDK: java.home is the build directory.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
  endif()
endif()
include_directories(src/share/java/include src/share/java/include/_gen src/${plat}/java/include src/share/java/runtime)

function(prefixed out dir)
  set(r)
  foreach(f ${ARGN})
    list(APPEND r "${dir}/${f}.c")
  endforeach()
  set(${out} ${r} PARENT_SCOPE)
endfunction()

# --- VM runtime DLL (javai.dll), holding the built-in native stubs ---
prefixed(runtime_sources src/share/java/runtime agent arena class date debug globals io math
  primitive system115 resource115 reflect numeric115 conversions115 serialization core115 corepath115 string115 stringhelpers115 exports115 hpi115
  jni jni-call jni-invoke fileextras115 breakpoints115 jcov115
  object opnames profiler runtime security string stubs system thread threads
  utf util zip classinitialize classresolver classhash115 common_exceptions threadruntime
  compiler check_class check_code classloader classruntime gifdecoder exception
  exceptions filesystem interpreter gc finalize monitor monitor_cache)
prefixed(md_share src/share/java/runtime agentthread compiler_md io_md ionet_md math_md
  process_md properties_md socket_md stubs_md)
prefixed(md_plat src/${plat}/java/runtime dir_md gc_md link_md misc_md mon_md path_md
  system_md thread_md)
if(NOT WIN32 AND JDK115_UNIX_THREADS STREQUAL "cthreads")
  list(REMOVE_ITEM md_plat src/unix/java/runtime/thread_md.c src/unix/java/runtime/mon_md.c src/unix/java/runtime/link_md.c)
  list(APPEND md_plat src/unix/java/runtime/thread_cthreads.c src/unix/java/runtime/mon_cthreads.c
    src/unix/java/runtime/condition_md.c src/unix/java/runtime/condvar_md.c src/unix/java/runtime/link_cthreads.c
    src/unix/java/runtime/process_cthreads.c)
endif()

if(WIN32)
  # Sun's Debug DLL adds exactly this decorated debugger-hook entry.
  file(READ "${CMAKE_SOURCE_DIR}/src/share/java/javai/javai.def" javai_export_text)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/share/java/javai/javai.def")
  file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/generated/javai-$<CONFIG>.def"
    CONTENT "${javai_export_text}$<$<CONFIG:Debug>:    _set_debugger_hooks@12\n>")
  set(javai_exports "${CMAKE_BINARY_DIR}/generated/javai-$<CONFIG>.def")
  list(APPEND md_plat src/win32/java/runtime/condvar_md.c ${file_hpi_sources})
else()
  list(APPEND md_plat src/unix/java/runtime/net_md.c)
endif()
include(tools/builtin-natives.cmake)
add_library(javai_core OBJECT src/share/java/javai/javai.c src/share/java/javai/javai_dll.c ${runtime_sources}
  ${md_share} ${md_plat})
set_target_properties(javai_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(javai_core PRIVATE "${CMAKE_BINARY_DIR}/generated" third_party/zlib-1.0.4)
target_compile_definitions(javai_core PRIVATE JNI115_ENABLED JAVA_INTERNAL_BUILD)
add_library(javai SHARED $<TARGET_OBJECTS:javai_core> ${javai_exports})
prefixed(zlib_sources third_party/zlib-1.0.4 adler32 infblock infcodes inffast inflate inftrees infutil zutil)
add_library(zlib115 STATIC ${zlib_sources})
target_include_directories(zlib115 PUBLIC third_party/zlib-1.0.4)
if(MSVC)
  target_compile_options(zlib115 PRIVATE /UDEBUG)
else()
  target_compile_options(zlib115 PRIVATE -UDEBUG)
endif()
target_link_libraries(javai zlib115 ${sys_libs})

add_executable(java src/share/java/javai/java_stub.c)
target_link_libraries(java javai)
add_executable(java-rmi src/share/java/javai/rmi_cgi.c)
set_target_properties(java-rmi PROPERTIES SUFFIX ".cgi")
install(TARGETS java-rmi RUNTIME DESTINATION bin)
foreach(tool javac javap jar appletviewer native2ascii serialver javadoc javakey rmic rmiregistry)
  add_executable(${tool} src/share/java/javai/tool_main.c src/${plat}/java/runtime/path_md.c
    src/share/java/runtime/zip.c ${file_hpi_sources})
  target_link_libraries(${tool} javai zlib115)
endforeach()
target_compile_definitions(javac PRIVATE TOOL_CLASS="sun.tools.javac.Main")
target_compile_definitions(javap PRIVATE TOOL_CLASS="sun.tools.javap.JavaP" TOOL_SIMPLE TOOL_HEAP="")
target_compile_definitions(jar PRIVATE TOOL_CLASS="sun.tools.jar.Main" TOOL_SIMPLE)
target_compile_definitions(native2ascii PRIVATE TOOL_CLASS="sun.tools.native2ascii.Main" TOOL_SIMPLE)
target_compile_definitions(serialver PRIVATE TOOL_CLASS="sun.tools.serialver.SerialVer" TOOL_HOME TOOL_HEAP="")
target_compile_definitions(javadoc PRIVATE TOOL_CLASS="sun.tools.javadoc.Main" TOOL_SIMPLE TOOL_HEAP="")
target_compile_definitions(javakey PRIVATE TOOL_CLASS="sun.security.provider.Main" TOOL_SIMPLE)
target_compile_definitions(rmic PRIVATE TOOL_CLASS="sun.rmi.rmic.Main" TOOL_HOME TOOL_HEAP="")
target_compile_definitions(rmiregistry PRIVATE TOOL_CLASS="sun.rmi.registry.RegistryImpl" TOOL_HOME TOOL_REGISTRY TOOL_HEAP="")
target_compile_definitions(appletviewer PRIVATE TOOL_CLASS="sun.applet.AppletViewer" TOOL_APPLET TOOL_HEAP="")
set(jre_sources src/share/java/jre/jre.c src/share/java/jre/jre_main.c src/${plat}/java/jre/jre_md.c)
add_executable(jre ${jre_sources})
set(jre_targets jre)
if(WIN32)
  add_executable(javaw WIN32 src/win32/java/javai/javaw.c)
  target_link_libraries(javaw javai)
  add_executable(jrew WIN32 ${jre_sources})
  list(APPEND jre_targets jrew)
endif()
foreach(tool ${jre_targets})
  target_include_directories(${tool} PRIVATE src/share/java/jre src/${plat}/java/jre)
  target_compile_definitions(${tool} PRIVATE VERSION="1.1.5")
  target_link_libraries(${tool} ${sys_libs})
endforeach()

# --- Standalone tools, each with its reduced runtime subset ---
prefixed(tool_runtime src/share/java/runtime arena classloader classresolver classhash115 classresolver_stubs conversions115
  globals runtime_stubs utf util zip)
add_executable(javah src/share/java/javah/javah.c ${tool_runtime} src/${plat}/java/runtime/path_md.c
  src/share/java/runtime/stubs_md.c src/${plat}/java/runtime/system_md.c ${file_hpi_sources})
target_compile_definitions(javah PRIVATE JAVA_INTERNAL_BUILD)
foreach(t javah)
  target_link_libraries(${t} zlib115 ${sys_libs})
endforeach()

# --- Smoke tests (ctest -C Release). Classes are prebuilt with JDK 1.1.5 javac (class file 45.3). ---
enable_testing()
include(cmake/zip115.cmake)
include(cmake/sysresource115.cmake)
include(cmake/mmedia115.cmake)
include(cmake/math115.cmake)
include(cmake/jpeg115.cmake)
add_test(NAME tools-documentation115 COMMAND ${CMAKE_COMMAND}
  "-DBIN=$<TARGET_FILE_DIR:javadoc>" "-DEXE=${CMAKE_EXECUTABLE_SUFFIX}"
  "-DHOME115=${CMAKE_BINARY_DIR}"
  "-DLIBDIR=$<TARGET_FILE_DIR:math>"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/documentation115/$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tests/tools/documentation115.cmake")
add_test(NAME tools-cgi115 COMMAND ${CMAKE_COMMAND}
  "-DBIN=$<TARGET_FILE_DIR:java>" "-DTOOL=$<TARGET_FILE:java-rmi>"
  -P "${CMAKE_SOURCE_DIR}/tests/tools/cgi115.cmake")
add_test(NAME tools-console115 COMMAND ${CMAKE_COMMAND}
  "-DBIN=$<TARGET_FILE_DIR:serialver>" "-DHOME115=${CMAKE_BINARY_DIR}"
  "-DEXE=${CMAKE_EXECUTABLE_SUFFIX}" -P "${CMAKE_SOURCE_DIR}/tests/tools/console115.cmake")
add_test(NAME tools-native2ascii115 COMMAND ${CMAKE_COMMAND}
  "-DTOOL=$<TARGET_FILE:native2ascii>" "-DROOT=${CMAKE_SOURCE_DIR}"
  "-DOUT=${CMAKE_BINARY_DIR}/native2ascii115/$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tests/tools/native2ascii115.cmake")
if(WIN32)
  get_filename_component(msvc_bin "${CMAKE_LINKER}" DIRECTORY)
  add_test(NAME vm-exports COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:javai>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-sun115$<$<CONFIG:Debug>:-debug>.txt"
    -P ${CMAKE_SOURCE_DIR}/tools/check-exports.cmake)
endif()
add_executable(jni-layout tests/native/jni-layout.c)
add_test(NAME jni-layout COMMAND jni-layout)
add_executable(class-layout115 tests/native/class-layout115.c)
add_test(NAME class-layout115 COMMAND class-layout115)
add_library(hpi-signatures OBJECT tests/native/hpi-signatures.c)
add_executable(interrupt-state tests/native/interrupt-state.c $<TARGET_OBJECTS:javai_core>)
target_compile_definitions(interrupt-state PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(interrupt-state zlib115 ${sys_libs})
add_test(NAME interrupt-state COMMAND interrupt-state)

add_executable(alloc-cache115 tests/native/alloc-cache115.c $<TARGET_OBJECTS:javai_core>)
target_compile_definitions(alloc-cache115 PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(alloc-cache115 zlib115 ${sys_libs})
add_test(NAME alloc-cache115 COMMAND alloc-cache115 "${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip")
set_tests_properties(alloc-cache115 PROPERTIES TIMEOUT 30)
add_test(NAME class-name115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DROOT=${PROJECT_SOURCE_DIR}"
  "-DJDK115_DIR=${JDK115_DIR}"
  "-DWORK=${PROJECT_BINARY_DIR}/misnamed-gate-$<CONFIG>"
  -P "${PROJECT_SOURCE_DIR}/tools/check-class-name115.cmake")
add_executable(managed-class115 tests/native/managed-class115.c $<TARGET_OBJECTS:javai_core>)
target_compile_definitions(managed-class115 PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(managed-class115 zlib115 ${sys_libs})
foreach(classgc_enabled 0 1)
  add_test(NAME managed-class115-${classgc_enabled} COMMAND managed-class115
    "${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip"
    "${CMAKE_SOURCE_DIR}/tests/vm/ManagedPayload115.class" ${classgc_enabled})
  set_tests_properties(managed-class115-${classgc_enabled} PROPERTIES TIMEOUT 30)
endforeach()

add_executable(hpi115-test tests/native/hpi115.c src/share/java/runtime/hpi115.c
  src/${plat}/java/runtime/dir_md.c ${file_hpi_sources})
target_compile_definitions(hpi115-test PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(hpi115-test ${sys_libs})
if(MSVC)
  target_compile_options(hpi115-test PRIVATE /UNDEBUG)
else()
  target_compile_options(hpi115-test PRIVATE -UNDEBUG)
endif()
add_test(NAME hpi115-test COMMAND hpi115-test)
if(WIN32)
  add_executable(file-hpi115 tests/native/file-hpi115.c ${file_hpi_sources})
  target_compile_definitions(file-hpi115 PRIVATE JAVA_INTERNAL_BUILD)
  add_test(NAME file-hpi115 COMMAND ${CMAKE_COMMAND}
    "-DEXE=$<TARGET_FILE:file-hpi115>"
    "-DEXPECTED=${CMAKE_SOURCE_DIR}/tests/native/file-hpi115.expected"
    -P ${CMAKE_SOURCE_DIR}/tests/native/file-hpi115.cmake)
endif()


add_executable(string-helpers115 tests/native/string-helpers115.c $<TARGET_OBJECTS:javai_core>)
target_compile_definitions(string-helpers115 PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(string-helpers115 zlib115 ${sys_libs})
foreach(encoding 8859_1 Cp1252 no-such-encoding)
  add_test(NAME string-helpers115-${encoding} COMMAND ${CMAKE_COMMAND}
    "-DPROGRAM=$<TARGET_FILE:string-helpers115>" "-DCP=${JDK115_DIR}/lib/classes.zip"
    "-DENCODING=${encoding}"
    "-DPROFILE=${CMAKE_BINARY_DIR}/string-profile-$<CONFIG>-${encoding}.txt"
    "-DEXPECTED=${CMAKE_SOURCE_DIR}/tests/native/string-helpers115-${encoding}.expected"
    -P ${CMAKE_SOURCE_DIR}/tests/native/string-helpers115.cmake)
endforeach()
if(WIN32)
  add_executable(thread-layout tests/native/thread-layout.c)
  add_test(NAME thread-layout COMMAND thread-layout)
  add_executable(condvar-test tests/native/condvar-test.c src/win32/java/runtime/condvar_md.c)
  target_link_libraries(condvar-test winmm)
  add_test(NAME condvar COMMAND condvar-test)
  set_tests_properties(condvar PROPERTIES TIMEOUT 30)
  add_executable(monitor-test tests/native/monitor-test.c
    src/win32/java/runtime/mon_md.c src/win32/java/runtime/condvar_md.c)
  target_link_libraries(monitor-test winmm)
  add_test(NAME hpi-monitor COMMAND monitor-test)
  set_tests_properties(hpi-monitor PROPERTIES TIMEOUT 30)
endif()
include(CMakeParseArguments)
function(java_test)
  cmake_parse_arguments(JT "" "NAME;CLASS;EXPECTED;EXIT" "ARGS;VM_ARGS;EXTRA_CP" ${ARGN})
  if(NOT JT_EXIT)
    set(JT_EXIT 0)
  endif()
  if(NOT JT_EXPECTED)
    set(JT_EXPECTED "${JT_CLASS}")
  endif()
  set(cp "${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip")
  foreach(entry ${JT_EXTRA_CP})
    set(cp "${cp}${sep}${entry}")
  endforeach()
  add_test(NAME ${JT_NAME} COMMAND ${CMAKE_COMMAND}
    "-DJAVA=$<TARGET_FILE:java>"
    "-DCP=${cp}"
    "-DCLASS=${JT_CLASS}" "-DARGS=${JT_ARGS}" "-DVM_ARGS=${JT_VM_ARGS}"
    "-DEXPECTED_EXIT=${JT_EXIT}"
    "-DEXPECTED=${CMAKE_SOURCE_DIR}/tests/vm/${JT_EXPECTED}.expected"
    -P ${CMAKE_SOURCE_DIR}/tools/run-java-test.cmake)
  set_tests_properties(${JT_NAME} PROPERTIES TIMEOUT 40 LABELS "jdk115"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/vm")
  # Replay the same oracle cases against classes compiled by our javac.
  set(selfhost_cp "\${OUT}${sep}${JDK115_DIR}/lib/classes.zip")
  foreach(entry ${JT_EXTRA_CP})
    set(selfhost_cp "${selfhost_cp}${sep}${entry}")
  endforeach()
  set_property(GLOBAL APPEND_STRING PROPERTY selfhost_cases
    "set(CLASS [==[${JT_CLASS}]==])\nset(ARGS [==[${JT_ARGS}]==])\nset(VM_ARGS [==[${JT_VM_ARGS}]==])\nset(EXPECTED_EXIT ${JT_EXIT})\nset(EXPECTED [==[${CMAKE_SOURCE_DIR}/tests/vm/${JT_EXPECTED}.expected]==])\nset(CP \"${selfhost_cp}\")\ninclude([==[${CMAKE_SOURCE_DIR}/tools/run-java-test.cmake]==])\n")
endfunction()
# Reconstruction progress tests; failures remain visible, baseline uses -LE jdk115.
java_test(NAME vm-hello115 CLASS Hello115)
java_test(NAME vm-i18n115 CLASS I18n115)
java_test(NAME vm-small-heap115 CLASS Hello115 VM_ARGS -ms16k)
java_test(NAME vm-FinalizeQueue115 CLASS FinalizeQueue115)
java_test(NAME vm-primitives CLASS PrimitiveClasses)
java_test(NAME vm-Circularity115 CLASS Circularity115)
add_test(NAME vm-JCov115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DCP=${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip"
  "-DJAVAC=$<TARGET_FILE:javac>" "-DSEP=${sep}"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/jcov115/$<CONFIG>"
  "-DENABLED=$<CONFIG:Debug>" -P "${CMAKE_SOURCE_DIR}/tests/vm/jcov115.cmake")
java_test(NAME vm-NumericConversions115 CLASS NumericConversions115)
java_test(NAME vm-CommandLineProps115 CLASS CommandLineProps115 VM_ARGS
  -Dp0=zero -Dp1=one -Dp2=two -Dp3=three -Dp4=four -Dp5=five -Dp6=six -Dp7=seven -Dp8=)
foreach(fixture Props115 Unicode ClassPathForms Reflect ReflectExceptions Numeric115 Serial ClinitFail Interrupt Exec)
  java_test(NAME vm-${fixture} CLASS ${fixture})
endforeach()
java_test(NAME vm-ReflectHierarchy CLASS ReflectHierarchy)
java_test(NAME vm-ReflectValues CLASS ReflectValues)
java_test(NAME vm-SerialValues CLASS SerialValues)
java_test(NAME vm-SerialHooks CLASS SerialHooks)
java_test(NAME vm-Core115 CLASS Core115)
java_test(NAME vm-Launcher115 CLASS Launcher115 ARGS one -two "three four")
java_test(NAME vm-MonitorCapacityOption115 CLASS Launcher115 EXPECTED Launcher115
  VM_ARGS -nm256 ARGS one -two "three four")
java_test(NAME vm-MonitorCache115 CLASS MonitorCache115)
java_test(NAME vm-VMReserve115 CLASS VMReserve115 VM_ARGS -ms1m -mx4m -mr256k -my256k)
java_test(NAME vm-Intern115 CLASS Intern115)
java_test(NAME vm-FileExtras115 CLASS FileExtras115)
java_test(NAME vm-Profiler115 CLASS Profiler115)
add_library(jnihello SHARED tests/native/jnihello.c)
add_library(jniabi SHARED tests/native/jniabi.c)
add_library(debug115 SHARED tests/native/debug115.c)
target_link_libraries(debug115 ${sys_libs})
java_test(NAME vm-Debug115 CLASS Debug115 ARGS "$<TARGET_FILE:debug115>"
  EXPECTED "Debug115$<$<CONFIG:Debug>:-debug>")
java_test(NAME vm-JniHello CLASS JniHello ARGS "$<TARGET_FILE:jnihello>")
java_test(NAME vm-JniAbi CLASS JniAbi ARGS "$<TARGET_FILE:jniabi>")
add_executable(jniinvoke tests/native/jniinvoke.c)
target_link_libraries(jniinvoke ${sys_libs})
add_test(NAME jni-invocation COMMAND ${CMAKE_COMMAND}
  "-DINVOKER=$<TARGET_FILE:jniinvoke>" "-DVM=$<TARGET_FILE:javai>"
  "-DCP=${CMAKE_SOURCE_DIR}/tests/vm${sep}${JDK115_DIR}/lib/classes.zip"
  -P ${CMAKE_SOURCE_DIR}/tests/native/jniinvoke.cmake)
add_test(NAME jre-invocation COMMAND ${CMAKE_COMMAND}
  "-DJRE=$<TARGET_FILE:jre>" "-DVM=$<TARGET_FILE:javai>"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK115_DIR=${JDK115_DIR}"
  "-DOUT=${CMAKE_BINARY_DIR}/jre-image/$<CONFIG>"
  -P ${CMAKE_SOURCE_DIR}/tools/jre-test.cmake)
java_test(NAME vm-Canonical115 CLASS Canonical115)
foreach(mode 0 1 2 3)
  java_test(NAME vm-ExecLifecycle-${mode} CLASS ExecLifecycle ARGS ${mode}
    EXPECTED ExecLifecycle-${mode})
endforeach()
foreach(mode zero normal disabled reenter)
  java_test(NAME vm-FinalizeExit-${mode} CLASS FinalizeExit ARGS ${mode}
    EXPECTED FinalizeExit-${mode})
endforeach()
java_test(NAME vm-FinalizeExit-nonzero CLASS FinalizeExit ARGS nonzero
  EXPECTED FinalizeExit-nonzero EXIT 7)
add_test(NAME javac-selfhost COMMAND ${CMAKE_COMMAND}
  "-DJAVAC=$<TARGET_FILE:javac>" "-DJAVA=$<TARGET_FILE:java>"
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK115_DIR=${JDK115_DIR}"
  "-DOUT=${CMAKE_BINARY_DIR}/selfhost/$<CONFIG>"
  "-DCASES=${CMAKE_BINARY_DIR}/selfhost-cases-$<CONFIG>.cmake"
  -P ${CMAKE_SOURCE_DIR}/tools/javac-selfhost.cmake)
set_tests_properties(javac-selfhost PROPERTIES TIMEOUT 180
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/vm")
add_test(NAME vm-classpath-forms COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DROOT=${CMAKE_SOURCE_DIR}"
  "-DJDK115_DIR=${JDK115_DIR}" -P ${CMAKE_SOURCE_DIR}/tools/classpath-forms.cmake)
set_tests_properties(vm-classpath-forms PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/vm")
java_test(NAME vm-InnerAndOld CLASS InnerAndOld ARGS java.util.ObserverList
  EXTRA_CP "${CMAKE_SOURCE_DIR}/tests/vm/old")
java_test(NAME vm-BadVerify CLASS BadVerify VM_ARGS -verify
  EXTRA_CP "${CMAKE_SOURCE_DIR}/tests/vm/bad")
java_test(NAME vm-Resources CLASS Resources EXTRA_CP "${CMAKE_SOURCE_DIR}/tests/vm/resources115.zip")
get_property(selfhost_cases GLOBAL PROPERTY selfhost_cases)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/selfhost-cases-$<CONFIG>.cmake"
  CONTENT "${selfhost_cases}")
add_executable(zip115-test tests/native/zip115-test.c src/share/java/runtime/zip.c ${file_hpi_sources})
target_link_libraries(zip115-test zlib115 ${sys_libs})
add_test(NAME zip115 COMMAND zip115-test "${CMAKE_SOURCE_DIR}/tests/vm/resources115.zip")
configure_file("${JDK115_DIR}/lib/classes.zip" "${CMAKE_BINARY_DIR}/classes.zip" COPYONLY)
configure_file("${JDK115_DIR}/lib/classes.zip" "${CMAKE_BINARY_DIR}/lib/classes.zip" COPYONLY)
configure_file("${JDK115_DIR}/lib/security/java.security" "${CMAKE_BINARY_DIR}/lib/security/java.security" COPYONLY)
foreach(resource serialver.properties rmic.properties)
  configure_file("${JDK115_DIR}/lib/${resource}" "${CMAKE_BINARY_DIR}/lib/${resource}" COPYONLY)
  install(FILES "${JDK115_DIR}/lib/${resource}" DESTINATION lib)
endforeach()
set(test_cp "${CMAKE_SOURCE_DIR}/tests/smoke${sep}${CMAKE_BINARY_DIR}/classes.zip")
add_test(NAME hello COMMAND java -classpath "${test_cp}" Hello)
set_tests_properties(hello PROPERTIES PASS_REGULAR_EXPRESSION "VM: 2.0 4.0 0.*sum=9999900000")
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
  "version=1\\.1\\.5.*vendor=Sun Microsystems Inc\\..*separator=${props_sep}.*home is dir=true.*child is dir=true.*classpath set=true.*os=${props_os}/x86")
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
add_test(NAME javah-jni COMMAND ${CMAKE_COMMAND}
  "-DJAVAH=$<TARGET_FILE:javah>" "-DCP=${native_cp}"
  "-DOUT=${CMAKE_BINARY_DIR}/jni-headers/$<CONFIG>"
  -P ${CMAKE_SOURCE_DIR}/tests/native/javah-jni.cmake)
set(native_gen "${CMAKE_BINARY_DIR}/native-gen")
file(MAKE_DIRECTORY "${native_gen}/tmp")
add_custom_command(OUTPUT "${native_gen}/NativeHello.h" "${native_gen}/NativeHello.c"
  COMMAND ${CMAKE_COMMAND} -E env "TMP=${native_gen}/tmp" "TMPDIR=${native_gen}/tmp" $<TARGET_FILE:javah> -classpath "${native_cp}" -d "${native_gen}" NativeHello
  COMMAND ${CMAKE_COMMAND} -E env "TMP=${native_gen}/tmp" "TMPDIR=${native_gen}/tmp" $<TARGET_FILE:javah> -stubs -classpath "${native_cp}" -d "${native_gen}" NativeHello
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
install(TARGETS java javac javah javap jar appletviewer native2ascii serialver javadoc javakey rmic rmiregistry jre javai RUNTIME DESTINATION bin LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
if(WIN32)
  install(TARGETS javaw jrew RUNTIME DESTINATION bin)
endif()
install(FILES "${JDK115_DIR}/lib/classes.zip" DESTINATION lib)
install(FILES "${JDK115_DIR}/lib/security/java.security" DESTINATION lib/security)
install(DIRECTORY src/share/java/include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/${plat}/java/include/ DESTINATION include/${plat}
  FILES_MATCHING PATTERN "*.h")

# Link internal monitor-cache probes without widening the production DLL ABI.
add_executable(monitor-cache115 tests/native/monitor-cache115.c $<TARGET_OBJECTS:javai_core>)
target_compile_definitions(monitor-cache115 PRIVATE JAVA_INTERNAL_BUILD)
target_link_libraries(monitor-cache115 zlib115 ${sys_libs})
add_test(NAME monitor-cache115 COMMAND monitor-cache115 "${JDK115_DIR}/lib/classes.zip")
set_tests_properties(monitor-cache115 PROPERTIES TIMEOUT 60)

# Exercise the coverage writer's three input formats without exporting private APIs.
add_executable(jcov115-probe tests/native/jcov115-probe.c src/share/java/runtime/jcov115.c)
target_compile_definitions(jcov115-probe PRIVATE JAVA_INTERNAL_BUILD JCOV)
add_test(NAME jcov115-writer COMMAND ${CMAKE_COMMAND}
  "-DPROBE=$<TARGET_FILE:jcov115-probe>" "-DROOT=${CMAKE_SOURCE_DIR}"
  "-DOUT=${CMAKE_BINARY_DIR}/jcov115-native/$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tests/native/jcov115-probe.cmake")

add_executable(exitcallbacks115 tests/native/exitcallbacks115.c src/${plat}/java/runtime/system_md.c)
target_compile_definitions(exitcallbacks115 PRIVATE JAVA_INTERNAL_BUILD JNI115_ENABLED)
add_test(NAME exitcallbacks115 COMMAND ${CMAKE_COMMAND}
  "-DPROBE=$<TARGET_FILE:exitcallbacks115>" -P "${CMAKE_SOURCE_DIR}/tests/native/exitcallbacks115.cmake")
include(cmake/net115.cmake)

# Exercise the reconstructed Mach protocol with deterministic test-only IPC.
# This does not select Mach for the running VM or claim DR2 execution.
add_executable(mach115-condition tests/native/mach115-condition.c
  src/unix/java/runtime/condition_md.c)
target_include_directories(mach115-condition BEFORE PRIVATE tests/native/mach115)
add_test(NAME mach115-condition COMMAND mach115-condition)
if(WIN32)
  set(awt_gen "${CMAKE_BINARY_DIR}/awt115-generated")
  file(MAKE_DIRECTORY "${awt_gen}/tmp")
  configure_file("${JDK115_DIR}/lib/font.properties" "${CMAKE_BINARY_DIR}/lib/font.properties" COPYONLY)
  install(FILES "${JDK115_DIR}/lib/font.properties" DESTINATION lib)
  set(awt_classes sun.awt.windows.WComponentPeer sun.awt.windows.WWindowPeer
    sun.awt.windows.WFramePeer sun.awt.windows.WGraphics java.awt.Component java.awt.Container java.awt.Window
    sun.awt.windows.WMenuItemPeer sun.awt.windows.WMenuPeer sun.awt.windows.WMenuBarPeer sun.awt.windows.WCheckboxMenuItemPeer sun.awt.windows.WPopupMenuPeer
    java.awt.MenuComponent java.awt.MenuItem java.awt.Menu java.awt.MenuBar java.awt.CheckboxMenuItem java.awt.PopupMenu java.awt.MenuShortcut
    java.awt.Frame java.awt.Button java.awt.Label java.awt.Checkbox java.awt.Scrollbar java.awt.TextArea java.awt.TextField java.awt.Insets java.awt.Font java.awt.Cursor java.awt.Color java.awt.AWTEvent java.awt.event.KeyEvent java.awt.event.MouseEvent
    sun.awt.windows.WFontMetrics sun.awt.windows.WDefaultFontCharset sun.awt.FontDescriptor sun.awt.PlatformFont sun.awt.CharsetString
    sun.awt.image.ImageRepresentation sun.awt.image.Image sun.awt.image.OffScreenImageSource sun.awt.image.GifImageDecoder
    java.awt.image.ColorModel java.awt.image.DirectColorModel java.awt.image.IndexColorModel java.awt.Rectangle
    sun.awt.windows.WScrollPanePeer java.awt.ScrollPane java.awt.ScrollPaneAdjustable
    sun.awt.windows.WFileDialogPeer java.awt.FileDialog)
  set(awt_headers)
  foreach(clazz ${awt_classes})
    string(REPLACE "." "_" header ${clazz})
    list(APPEND awt_headers "${awt_gen}/${header}.h")
  endforeach()
  add_custom_command(OUTPUT ${awt_headers}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${awt_gen}"
    COMMAND ${CMAKE_COMMAND} -E env "TMP=${awt_gen}/tmp" "TMPDIR=${awt_gen}/tmp" $<TARGET_FILE:javah> -classpath "${JDK115_DIR}/lib/classes.zip" -d "${awt_gen}" ${awt_classes}
    DEPENDS javah "${JDK115_DIR}/lib/classes.zip")
  set(awt_stubs)
  foreach(peer WComponentPeer WWindowPeer)
    list(APPEND awt_stubs "${awt_gen}/sun_awt_windows_${peer}.c")
  endforeach()
  add_custom_command(OUTPUT ${awt_stubs}
    COMMAND ${CMAKE_COMMAND} -E env "TMP=${awt_gen}/tmp" "TMPDIR=${awt_gen}/tmp" $<TARGET_FILE:javah> -stubs -classpath "${JDK115_DIR}/lib/classes.zip" -d "${awt_gen}"
      sun.awt.windows.WComponentPeer sun.awt.windows.WWindowPeer
    DEPENDS javah ${awt_headers})
  add_library(winawt SHARED src/win32/sun/windows/awt_Toolkit.cpp
    src/win32/sun/windows/awt_Component.cpp src/win32/sun/windows/awt_Input.cpp src/win32/sun/windows/awt_Window.cpp
    src/win32/sun/windows/awt_Frame.cpp src/win32/sun/windows/awt_Graphics.cpp
    src/win32/sun/windows/awt_Canvas.cpp src/win32/sun/windows/awt_Button.cpp
    src/win32/sun/windows/awt_Font.cpp src/win32/sun/windows/awt_Label.cpp
    src/win32/sun/windows/awt_Checkbox.cpp src/win32/sun/windows/awt_Choice.cpp src/win32/sun/windows/awt_List.cpp src/win32/sun/windows/awt_Scrollbar.cpp src/win32/sun/windows/awt_Image.cpp src/win32/sun/windows/image.cpp ${awt_headers} ${awt_stubs}
    src/win32/sun/windows/awt_TextComponent.cpp src/win32/sun/windows/awt_ScrollPane.cpp
    src/win32/sun/windows/awt_PixelFormat.cpp src/win32/sun/windows/awt_Dialog.cpp
    src/win32/sun/windows/awt_FileDialog.cpp src/win32/sun/windows/winawt.def)
  target_include_directories(winawt BEFORE PRIVATE "${awt_gen}")
  target_link_libraries(winawt javai user32 gdi32 advapi32 comdlg32)
  add_library(awtgraphics115probe SHARED tests/native/awtgraphics115probe.c)
  target_link_libraries(awtgraphics115probe javai user32 gdi32)
  add_executable(awtformats115 tests/native/awtformats115.cpp src/win32/sun/windows/awt_PixelFormat.cpp)
  target_link_libraries(awtformats115 user32 gdi32 advapi32)
  add_test(NAME awt-formats115 COMMAND awtformats115)
  add_library(awt115probe SHARED tests/native/awt115probe.c)
  target_link_libraries(awt115probe javai user32)
  add_executable(winawt115-toolkit tests/native/winawt115-toolkit.c
    src/win32/sun/windows/awt_Toolkit.cpp)
  target_compile_definitions(winawt115-toolkit PRIVATE AWT115_QUERIES_ONLY)
  target_link_libraries(winawt115-toolkit user32 gdi32)
  add_test(NAME winawt115-toolkit COMMAND winawt115-toolkit
    "${JDK115_DIR}/bin/winawt$<$<CONFIG:Debug>:_g>.dll")
  foreach(awt_test Toolkit115 Lifecycle115 Graphics115 WindowEvents115 WindowState115 Dialog115 FileDialog115 Input115 InputRetire115 Canvas115 Button115 Fonts115 Label115 Checkbox115 Choice115 List115 Scrollbar115 Text115 ScrollPane115 ReentrantPeers115 PaneRace115 RetiredGraphics115 Images115 Raster115 TextRaster115 ImageRaster115 ColorRaster115 ImageBounds115 DecoderImages115 GifRaster115 GraphicsResources115 ProgressiveImages115 GraphicsHDC115 ScaledDelivery115)
  add_test(NAME awt-${awt_test} COMMAND ${CMAKE_COMMAND}
    -DROOT=${CMAKE_SOURCE_DIR} -DJAVA=$<TARGET_FILE:java> -DJAVAC=$<TARGET_FILE:javac>
    -DJDK=${JDK115_DIR} -DLIBDIR=$<TARGET_FILE_DIR:winawt>
    -DOUT=${CMAKE_BINARY_DIR}/awt115-$<CONFIG> -DCLASS=${awt_test}
    -DCOMPARE_ORACLE=ON
    -P ${CMAKE_SOURCE_DIR}/tests/awt/run115.cmake)
  endforeach()
endif()
add_executable(mach115-condvar tests/native/mach115-condvar.c
  src/unix/java/runtime/condvar_md.c)
add_executable(mach115-condvar-integration tests/native/mach115-condition.c
  src/unix/java/runtime/condition_md.c src/unix/java/runtime/condvar_md.c)
target_compile_definitions(mach115-condvar-integration PRIVATE TEST_VM_CONDVARS)
add_executable(mach115-thread tests/native/mach115-thread.c)
add_executable(mach115-monitor tests/native/mach115-monitor.c)
add_test(NAME mach115-monitor-entry-error COMMAND mach115-monitor --entry-error)
add_test(NAME mach115-gc-resume-error COMMAND mach115-thread --gc-resume-error)
add_test(NAME mach115-gc-rollback-error COMMAND mach115-thread --gc-rollback-error)
foreach(probe mach115-condvar mach115-condvar-integration mach115-thread mach115-monitor)
  target_include_directories(${probe} BEFORE PRIVATE tests/native/mach115-hpi
    tests/native/mach115 src/unix/java/include)
  add_test(NAME ${probe} COMMAND ${probe})
endforeach()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_executable(mach115-gc-monitor tests/native/mach115-gc-monitor.c
    src/unix/java/runtime/mon_cthreads.c src/unix/java/runtime/thread_cthreads.c)
  target_include_directories(mach115-gc-monitor BEFORE PRIVATE
    tests/native/mach115-hpi tests/native/mach115 src/unix/java/include)
  target_compile_options(mach115-gc-monitor PRIVATE -ffunction-sections -fdata-sections)
  set_property(TARGET mach115-gc-monitor APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
  add_test(NAME mach115-gc-monitor COMMAND mach115-gc-monitor)
  add_executable(thread-create115 tests/native/thread-create115.c src/share/java/runtime/threads.c)
  target_compile_options(thread-create115 PRIVATE -UDEBUG -ffunction-sections -fdata-sections)
  set_property(TARGET thread-create115 APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
  add_test(NAME thread-create115 COMMAND thread-create115)
  add_executable(mach115-memory tests/native/mach115-memory.c src/unix/java/runtime/gc_md.c)
  add_executable(mach115-link tests/native/mach115-link.c src/unix/java/runtime/link_cthreads.c)
  foreach(probe mach115-memory mach115-link)
    target_include_directories(${probe} BEFORE PRIVATE tests/native/mach115)
    target_compile_definitions(${probe} PRIVATE JDK115_CTHREADS)
    add_test(NAME ${probe} COMMAND ${probe})
  endforeach()
  add_executable(mach115-spawn tests/native/mach115-spawn.c src/unix/java/runtime/process_cthreads.c)
  add_test(NAME mach115-spawn COMMAND mach115-spawn)
  set_tests_properties(mach115-spawn PROPERTIES TIMEOUT 30)
  add_executable(mach115-monitor-concurrent tests/native/mach115-monitor-concurrent.c
    src/unix/java/runtime/mon_cthreads.c src/unix/java/runtime/condvar_md.c
    src/unix/java/runtime/thread_cthreads.c)
  target_include_directories(mach115-monitor-concurrent BEFORE PRIVATE
    tests/native/mach115-hpi tests/native/mach115 src/unix/java/include)
  target_compile_options(mach115-monitor-concurrent PRIVATE -ffunction-sections -fdata-sections)
  set_property(TARGET mach115-monitor-concurrent APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
  target_link_libraries(mach115-monitor-concurrent ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME mach115-monitor-concurrent COMMAND mach115-monitor-concurrent)
  set_tests_properties(mach115-monitor-concurrent PROPERTIES TIMEOUT 30)
endif()
