cmake_minimum_required(VERSION 3.0...3.10)
# Inventory (no GUI): cmake -DROOT=... -DOUT=... -P this-file
# Native rejection/ownership fixtures (GUI): add -DMODE=SelfTest -DRUNNER=...
# Full unchanged-demo gate (GUI, serial desktop ownership required):
#   -DMODE=Run -DRUNNER=.../awtdemos115.exe -DBIN=.../Release
#   -DJDK_HOME=.../build-or-image-root [-DORIGIN=Both] [-DSELECT=regex]
# Run outputs manifest.json/csv, results.json/csv, comparisons.json, and per
# case process.log, command/identity metadata, client/content BMPs and hashes.
# Pixel comparison metrics are evidence for review, never a visual-parity claim.
if(NOT ROOT)
  get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
if(NOT OUT)
  set(OUT "${ROOT}/out/winawt-demos115")
endif()
if(NOT MODE)
  set(MODE Inventory)
endif()
find_program(POWERSHELL powershell REQUIRED)
file(MAKE_DIRECTORY "${OUT}")
set(command "${POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File)
if(MODE STREQUAL "Inventory" OR MODE STREQUAL "SelfTest")
  list(APPEND command "${CMAKE_CURRENT_LIST_DIR}/winawt-demos115.tests.ps1" -Root "${ROOT}" -Out "${OUT}")
  if(MODE STREQUAL "SelfTest")
    if(NOT RUNNER)
      message(FATAL_ERROR "SelfTest requires RUNNER")
    endif()
    list(APPEND command -Runner "${RUNNER}")
  endif()
  set(timeout 90)
elseif(MODE STREQUAL "Run" OR MODE STREQUAL "Review")
  if(NOT ORIGIN)
    set(ORIGIN Both)
  endif()
  if(NOT SELECT)
    set(SELECT ".*")
  endif()
  list(APPEND command "${CMAKE_CURRENT_LIST_DIR}/winawt-demos115.ps1" -Mode "${MODE}"
    -Root "${ROOT}" -Out "${OUT}" -Runner "${RUNNER}" -Bin "${BIN}"
    -JdkHome "${JDK_HOME}" -Origin "${ORIGIN}" -Select "${SELECT}")
  set(timeout 4000)
else()
  message(FATAL_ERROR "Unknown demo mode ${MODE}")
endif()
execute_process(COMMAND ${command} RESULT_VARIABLE status
  OUTPUT_FILE "${OUT}/harness.log" ERROR_FILE "${OUT}/harness-errors.log" TIMEOUT ${timeout})
file(READ "${OUT}/harness.log" output)
file(READ "${OUT}/harness-errors.log" errors)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Demo ${MODE} gate failed (${status}); artifacts: ${OUT}\n${output}\n${errors}")
endif()
message(STATUS "Demo ${MODE} gate passed; artifacts: ${OUT}\n${output}")
