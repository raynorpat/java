set(net115_sources
  src/share/java/net/net_util.c src/share/java/net/InetAddressImpl.c
  src/share/java/net/PlainSocketImpl.c src/share/java/net/PlainDatagramSocketImpl.c
  src/share/java/net/SocketStream.c
  src/share/java/net/java_net_InetAddressImpl.c src/share/java/net/java_net_PlainSocketImpl.c
  src/share/java/net/java_net_PlainDatagramSocketImpl.c
  src/share/java/net/java_net_SocketInputStream.c src/share/java/net/java_net_SocketOutputStream.c)
if(WIN32)
  list(APPEND net115_sources src/share/java/net/net.def)
endif()
add_library(net SHARED ${net115_sources})
target_include_directories(net BEFORE PRIVATE src/share/java/net)
target_link_libraries(net javai ${sys_libs})

if(UNIX AND NOT APPLE)
  # Legacy VM symbols must not interpose the reconstructed net implementations.
  set_target_properties(net PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
endif()
install(TARGETS net RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
add_test(NAME vm-net115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DLIBDIR=$<TARGET_FILE_DIR:net>" "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}" -P ${CMAKE_SOURCE_DIR}/tools/net115.cmake)
if(WIN32)
  add_test(NAME net-exports COMMAND ${CMAKE_COMMAND}
    "-DDUMPBIN=${msvc_bin}/dumpbin.exe" "-DDLL=$<TARGET_FILE:net>"
    "-DREFERENCE=${CMAKE_SOURCE_DIR}/tools/net115-exports.txt"
    -P ${CMAKE_SOURCE_DIR}/tools/check-exports.cmake)
endif()

add_test(NAME tools-rmi-functional115 COMMAND ${CMAKE_COMMAND}
  "-DJAVA=$<TARGET_FILE:java>" "-DRMIC=$<TARGET_FILE:rmic>"
  "-DLIBDIR=$<TARGET_FILE_DIR:net>" "-DROOT=${CMAKE_SOURCE_DIR}" "-DJDK=${JDK115_DIR}"
  "-DOUT=${CMAKE_BINARY_DIR}/net115-rmi/$<CONFIG>" -P ${CMAKE_SOURCE_DIR}/tools/net115-rmi.cmake)
add_executable(net115-errors tests/native/net115-errors.c src/share/java/net/net_util.c)
target_include_directories(net115-errors BEFORE PRIVATE src/share/java/net)
target_link_libraries(net115-errors ${sys_libs})
add_test(NAME net-errors115 COMMAND net115-errors)
