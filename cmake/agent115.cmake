# Recover bindings from the unchanged classes, not the old 1.0.2 class sources.
set(agent115_gen "${CMAKE_BINARY_DIR}/agent115-gen")
file(MAKE_DIRECTORY "${agent115_gen}")
set(agent115_classes Agent AgentConstants BreakpointHandler BreakpointQueue Field StackFrame LocalVariable LineNumber)
set(agent115_headers)
set(agent115_names)
foreach(name ${agent115_classes})
  list(APPEND agent115_headers "${agent115_gen}/sun_tools_debug_${name}.h")
  list(APPEND agent115_names sun.tools.debug.${name})
endforeach()
list(APPEND agent115_headers "${agent115_gen}/sun_tools_java_Constants.h")
list(APPEND agent115_names sun.tools.java.Constants)
set(agent115_stubs "${agent115_gen}/sun_tools_debug_Agent.c"
  "${agent115_gen}/sun_tools_debug_BreakpointHandler.c")
add_custom_command(OUTPUT ${agent115_headers} ${agent115_stubs}
  COMMAND $<TARGET_FILE:javah> -classpath "${JDK115_DIR}/lib/classes.zip"
    -d "${agent115_gen}" ${agent115_names}
  COMMAND $<TARGET_FILE:javah> -stubs -classpath "${JDK115_DIR}/lib/classes.zip"
    -d "${agent115_gen}" sun.tools.debug.Agent sun.tools.debug.BreakpointHandler
  DEPENDS javah "${JDK115_DIR}/lib/classes.zip")
add_custom_target(agent115-bindings DEPENDS ${agent115_headers} ${agent115_stubs})
# Native implementation units are shared by the DLL and focused probes.
add_library(agent115-core STATIC src/share/java/debug/agent115.c src/share/java/debug/agent115_stack.c src/share/java/debug/agent115_metadata.c
  ${agent115_headers})
target_include_directories(agent115-core BEFORE PRIVATE "${agent115_gen}")
add_executable(agent115-registry tests/native/agent115-registry.c tests/native/agent115-registry-stubs.c ${agent115_headers})
target_link_libraries(agent115-registry agent115-core)
target_include_directories(agent115-registry BEFORE PRIVATE "${agent115_gen}")
add_test(NAME agent115-registry COMMAND agent115-registry)
if(WIN32)
  add_test(NAME agent115-registry-sun COMMAND agent115-registry
    "${JDK115_DIR}/bin/agent$<$<CONFIG:Debug>:_g>.dll")
endif()
add_test(NAME agent115-bindings COMMAND ${CMAKE_COMMAND}
  "-DGEN=${agent115_gen}" "-DROOT=${CMAKE_SOURCE_DIR}"
  -P "${CMAKE_SOURCE_DIR}/tests/tools/agent115-bindings.cmake")

add_executable(agent115-stack tests/native/agent115-stack.c ${agent115_headers})
target_include_directories(agent115-stack BEFORE PRIVATE "${agent115_gen}")
target_link_libraries(agent115-stack agent115-core)
add_test(NAME agent115-stack COMMAND agent115-stack)


add_executable(agent115-metadata tests/native/agent115-metadata.c ${agent115_headers} $<TARGET_OBJECTS:javai_core>)
target_include_directories(agent115-metadata BEFORE PRIVATE "${agent115_gen}")
target_link_libraries(agent115-metadata agent115-core zlib115 ${sys_libs})
add_custom_command(OUTPUT "${agent115_gen}/Agent115Values.class" "${agent115_gen}/sun/tools/debug/Agent115Access.class"
  COMMAND $<TARGET_FILE:javac> -g -classpath "${JDK115_DIR}/lib/classes.zip" -d "${agent115_gen}"
    Agent115Values.java Agent115Access.java
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/native"
  DEPENDS javac "${CMAKE_SOURCE_DIR}/tests/native/Agent115Values.java" "${CMAKE_SOURCE_DIR}/tests/native/Agent115Access.java")
add_custom_target(agent115-values DEPENDS "${agent115_gen}/Agent115Values.class" "${agent115_gen}/sun/tools/debug/Agent115Access.class")
add_dependencies(agent115-metadata agent115-values)
add_test(NAME agent115-metadata COMMAND agent115-metadata "${agent115_gen}${sep}${JDK115_DIR}/lib/classes.zip")
set_tests_properties(agent115-metadata PROPERTIES TIMEOUT 30)


# The installable library contains exactly the native bindings shipped by Sun.
file(STRINGS "${CMAKE_SOURCE_DIR}/tools/exports-agent115.txt" agent115_exports)
if(WIN32)
  set(agent115_def "${CMAKE_BINARY_DIR}/agent115.def")
  file(WRITE "${agent115_def}" "EXPORTS\n")
  foreach(symbol ${agent115_exports})
    file(APPEND "${agent115_def}" "    ${symbol}\n")
  endforeach()
endif()
add_library(agent SHARED src/share/java/debug/agent115_events.c ${agent115_stubs} ${agent115_headers} ${agent115_def})
target_include_directories(agent BEFORE PRIVATE "${agent115_gen}")
target_link_libraries(agent agent115-core javai)
set_target_properties(agent115-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(WIN32)
  install(TARGETS agent RUNTIME DESTINATION bin)
  get_filename_component(agent115_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
  add_test(NAME agent115-exports COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${agent115_compiler_dir}/dumpbin.exe" "-DDLL=$<TARGET_FILE:agent>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-agent115.txt"
    -P "${CMAKE_SOURCE_DIR}/tools/check-exports.cmake")
else()
  file(WRITE "${CMAKE_BINARY_DIR}/agent115.map" "{ global:\n")
  foreach(symbol ${agent115_exports})
    file(APPEND "${CMAKE_BINARY_DIR}/agent115.map" "  ${symbol};\n")
  endforeach()
  file(APPEND "${CMAKE_BINARY_DIR}/agent115.map" "local: *; };\n")
  if(NOT APPLE)
    set_target_properties(agent PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_BINARY_DIR}/agent115.map")
  endif()
  install(TARGETS agent LIBRARY DESTINATION lib)
endif()


add_executable(agent115-jni tests/native/agent115-jni.c)
add_dependencies(agent115-jni agent agent115-values)
set_target_properties(agent115-jni PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/agent115-probe")
if(WIN32)
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(agent115-jni PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/agent115-probe")
  endforeach()
else()
  target_link_libraries(agent115-jni ${CMAKE_DL_LIBS})
endif()
add_test(NAME agent115-jni COMMAND agent115-jni "$<TARGET_FILE:javai>" "${agent115_gen}${sep}${JDK115_DIR}/lib/classes.zip")
set_tests_properties(agent115-jni PROPERTIES TIMEOUT 30 ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:javai>")
if(WIN32)
  add_test(NAME agent115-jni-sun COMMAND agent115-jni "${JDK115_DIR}/bin/javai$<$<CONFIG:Debug>:_g>.dll" "${agent115_gen}${sep}${JDK115_DIR}/lib/classes.zip")
  set_tests_properties(agent115-jni-sun PROPERTIES TIMEOUT 30)
endif()
if(UNIX AND NOT APPLE)
  add_test(NAME agent115-exports COMMAND ${CMAKE_COMMAND}
    "-DNM=${CMAKE_NM}" "-DLIBRARY=$<TARGET_FILE:agent>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/exports-agent115.txt"
    -P "${CMAKE_SOURCE_DIR}/tests/tools/agent115-exports.cmake")
endif()
if(WIN32)
  add_dependencies(agent115-stack agent)
  add_test(NAME agent115-stack-shared COMMAND agent115-stack "$<TARGET_FILE:agent>")
  add_test(NAME agent115-stack-sun COMMAND agent115-stack "${JDK115_DIR}/bin/agent$<$<CONFIG:Debug>:_g>.dll")
endif()
