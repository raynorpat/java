add_executable(debugger115-jni tests/native/debugger115-jni.c)
if(NOT WIN32)
  target_link_libraries(debugger115-jni ${CMAKE_DL_LIBS})
endif()
set(debugger_jni115_classes "${CMAKE_BINARY_DIR}/debugger-jni115-classes")
file(MAKE_DIRECTORY "${debugger_jni115_classes}")
add_custom_command(OUTPUT "${debugger_jni115_classes}/NativeException115.class"
  COMMAND $<TARGET_FILE:javac> -classpath "${JDK115_DIR}/lib/classes.zip"
    -d "${debugger_jni115_classes}" NativeException115.java
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/debugger"
  DEPENDS javac "${CMAKE_SOURCE_DIR}/tests/debugger/NativeException115.java")
add_custom_target(debugger115-jni-classes DEPENDS "${debugger_jni115_classes}/NativeException115.class")
add_dependencies(debugger115-jni debugger115-jni-classes)
add_test(NAME debugger115-jni COMMAND debugger115-jni "$<TARGET_FILE:javai>"
  "${debugger_jni115_classes}${sep}${JDK115_DIR}/lib/classes.zip" "$<CONFIG>")
set_tests_properties(debugger115-jni PROPERTIES TIMEOUT 30)
if(WIN32)
  add_test(NAME debugger115-jni-sun COMMAND debugger115-jni
    "${JDK115_DIR}/bin/javai$<$<CONFIG:Debug>:_g>.dll"
    "${debugger_jni115_classes}${sep}${JDK115_DIR}/lib/classes.zip" "$<CONFIG>")
  set_tests_properties(debugger115-jni-sun PROPERTIES TIMEOUT 30)
endif()
