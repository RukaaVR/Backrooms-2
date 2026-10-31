file(GLOB_RECURSE BR_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(br_tests ${BR_TEST_SOURCES})
br_configure_target(br_tests)
target_link_libraries(br_tests PRIVATE br_game Catch2::Catch2WithMain)
target_compile_definitions(br_tests PRIVATE BR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
set_target_properties(br_tests PROPERTIES FOLDER "Tests")

include(Catch)
# Tests tagged [gl] need a display (Xvfb in CI). They SKIP when no GL context can
# be created, so plain `ctest` stays green on headless machines.
catch_discover_tests(br_tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE PRE_TEST)
