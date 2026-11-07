find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(debugger115_wrapper "")
if(WIN32)
  add_executable(debugger115-process tests/native/debugger115-process.c)
  set(debugger115_wrapper "$<TARGET_FILE:debugger115-process>")
endif()
add_test(NAME debugger115-suspend COMMAND ${CMAKE_COMMAND}
  "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
  "-DJAVAC=$<TARGET_FILE:javac>" "-DJAVA=$<TARGET_FILE:java>"
  "-DOUT=${CMAKE_BINARY_DIR}/debugger115-suspend-$<CONFIG>"
  -P "${CMAKE_SOURCE_DIR}/tests/debugger/suspend115.cmake")
add_test(NAME debugger115-launcher COMMAND ${CMAKE_COMMAND}
  "-DJAVA_G=$<TARGET_FILE:java_g>"
  -P "${CMAKE_SOURCE_DIR}/tests/tools/debug-launcher115.cmake"
  CONFIGURATIONS Debug)
foreach(mode session stopped release launch caught uncaught)
  set(release_rejection OFF)
  set(configuration Debug)
  set(sun_java java_g)
  set(launch OFF)
  set(exception_mode "")
  if(mode STREQUAL "caught" OR mode STREQUAL "uncaught")
    set(exception_mode "${mode}")
  endif()
  if(mode STREQUAL "launch")
    set(launch ON)
  endif()
  if(mode STREQUAL "stopped")
    set(stopped ON)
  else()
    set(stopped OFF)
  endif()
  if(mode STREQUAL "release")
    set(configuration Release)
    set(release_rejection ON)
    set(sun_java java)
  endif()
  add_test(NAME debugger115-${mode} COMMAND ${CMAKE_COMMAND}
    "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
    "-DPYTHON=${Python3_EXECUTABLE}" "-DJAVAC=$<TARGET_FILE:javac>"
    "-DJAVA=$<TARGET_FILE:java>" "-DJDB=$<TARGET_FILE:jdb>"
    "-DLIBDIR=$<TARGET_FILE_DIR:javai>" "-DSTOPPED=${stopped}"
    "-DRELEASE_REJECTION=${release_rejection}"
    "-DLAUNCH=${launch}" "-DWRAPPER=${debugger115_wrapper}"
    "-DEXCEPTION_MODE=${exception_mode}"
    "-DBUILD_DIR=${CMAKE_BINARY_DIR}" "-DCONFIG=$<CONFIG>"
    "-DOUT=${CMAKE_BINARY_DIR}/debugger115-$<CONFIG>-${mode}"
    -P "${CMAKE_SOURCE_DIR}/tests/debugger/run115.cmake"
    CONFIGURATIONS ${configuration})
  set_tests_properties(debugger115-${mode} PROPERTIES TIMEOUT 180)
  if(WIN32)
    add_test(NAME debugger115-${mode}-sun COMMAND ${CMAKE_COMMAND}
      "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
      "-DPYTHON=${Python3_EXECUTABLE}" "-DJAVAC=${JDK115_DIR}/bin/javac.exe"
      "-DJAVA=${JDK115_DIR}/bin/${sun_java}.exe" "-DJDB=${JDK115_DIR}/bin/jdb.exe"
      "-DLIBDIR=${JDK115_DIR}/bin" "-DSTOPPED=${stopped}"
      "-DRELEASE_REJECTION=${release_rejection}"
      "-DLAUNCH=${launch}" "-DWRAPPER=${debugger115_wrapper}"
      "-DEXCEPTION_MODE=${exception_mode}"
      "-DOUT=${CMAKE_BINARY_DIR}/debugger115-sun-$<CONFIG>-${mode}"
      -P "${CMAKE_SOURCE_DIR}/tests/debugger/run115.cmake")
    set_tests_properties(debugger115-${mode}-sun PROPERTIES TIMEOUT 180)
  endif()
endforeach()
