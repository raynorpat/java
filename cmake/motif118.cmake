# Optional source material for the Linux Motif reconstruction. Peer preflight
# runs on either host; it does not install a toolkit before libawt is rebuilt.
set(SOLARIS118_DIR "" CACHE PATH "Extracted Solaris JDK 1.1.8/16 package directory")
if(NOT SOLARIS118_DIR)
  return()
endif()
set(motif118_archive "${SOLARIS118_DIR}/SUNWjvrt/reloc/usr/java1.1/lib/classes.zip")
if(NOT EXISTS "${motif118_archive}")
  message(FATAL_ERROR "SOLARIS118_DIR does not contain the runtime classes.zip")
endif()
set(motif118_out "${CMAKE_BINARY_DIR}/motif118")
add_custom_command(OUTPUT "${motif118_out}/bindings.ok"
  COMMAND ${CMAKE_COMMAND} "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
    "-DSOLARIS=${SOLARIS118_DIR}" "-DOUT=${motif118_out}"
    "-DPYTHON=${Python3_EXECUTABLE}" "-DJAVAC=$<TARGET_FILE:javac>" "-DJAVAH=$<TARGET_FILE:javah>"
    -P "${CMAKE_SOURCE_DIR}/tests/awt118/build-peers.cmake"
  DEPENDS javac javah "${motif118_archive}" "${JDK115_DIR}/lib/classes.zip"
    tools/awt118-audit.py tools/motif118-peers.py tools/solaris118-awt-native-entries.txt
    src/unix/sun/awt/im/InputContext.java tests/awt118/build-peers.cmake
    tests/awt118/PeerLink115.java tests/awt118/InputContextTest115.java)
add_custom_target(motif118-peers DEPENDS "${motif118_out}/bindings.ok")
add_library(motif118probe SHARED tests/native/motif118probe.c)
if(WIN32)
  # Publish explicit JNI and legacy names for both oracle VMs.
  file(WRITE "${CMAKE_BINARY_DIR}/motif118probe.def" "EXPORTS\nJava_sun_awt_im_InputContextTest115_bind=_Java_sun_awt_im_InputContextTest115_bind@20\nJava_sun_awt_im_InputContextTest115_verifyLayout=_Java_sun_awt_im_InputContextTest115_verifyLayout@8\nJava_sun_awt_im_InputContextTest115_swapTarget_stub\n")
  target_sources(motif118probe PRIVATE "${CMAKE_BINARY_DIR}/motif118probe.def")
  # Sun's debug loader appends _g even for System.load with an absolute path.
  add_custom_command(TARGET motif118probe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:motif118probe>" "$<TARGET_FILE_DIR:motif118probe>/motif118probe_g.dll")
endif()
target_include_directories(motif118probe BEFORE PRIVATE "${motif118_out}/generated")
add_dependencies(motif118probe motif118-peers)
add_test(NAME motif118-audit COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_SOURCE_DIR}/tests/awt118/audit-test.py" "${JDK115_DIR}/lib/classes.zip" "${motif118_archive}")
add_test(NAME motif118-package COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_SOURCE_DIR}/tests/awt118/package-test.py" "${JDK115_DIR}/lib/classes.zip"
  "${motif118_archive}" "${motif118_out}")
foreach(mode link lifecycle reference)
  add_test(NAME motif118-${mode} COMMAND ${CMAKE_COMMAND}
    "-DJAVA=$<TARGET_FILE:java>" "-DJDK=${JDK115_DIR}" "-DOUT=${motif118_out}"
    "-DLIBDIR=$<TARGET_FILE_DIR:motif118probe>" "-DMODE=${mode}"
    -P "${CMAKE_SOURCE_DIR}/tests/awt118/run-peers.cmake")
  set_tests_properties(motif118-${mode} PROPERTIES TIMEOUT 30)
  if(WIN32)
    add_test(NAME motif118-${mode}-sun COMMAND ${CMAKE_COMMAND}
      "-DJAVA=${JDK115_DIR}/bin/java$<$<CONFIG:Debug>:_g>.exe"
      "-DJDK=${JDK115_DIR}" "-DOUT=${motif118_out}"
      "-DLIBDIR=$<TARGET_FILE_DIR:motif118probe>" "-DMODE=${mode}"
      -P "${CMAKE_SOURCE_DIR}/tests/awt118/run-peers.cmake")
    set_tests_properties(motif118-${mode}-sun PROPERTIES TIMEOUT 30)
  endif()
endforeach()
