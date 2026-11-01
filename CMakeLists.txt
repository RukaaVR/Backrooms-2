cmake_minimum_required(VERSION 3.24)

project(BackroomsBeyondReality
    VERSION 0.1.0
    DESCRIPTION "THE BACKROOMS: BEYOND REALITY - native C++ survival horror"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BR_BUILD_TESTS "Build automated tests" ON)
option(BR_BUILD_TOOLS "Build developer tools (level validator, asset generator)" ON)
option(BR_ENABLE_ASAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(BR_WARNINGS_AS_ERRORS "Treat warnings in project code as errors" OFF)
option(BR_USE_CCACHE "Use ccache when available" ON)
option(BR_ENABLE_TRACY "Instrument with the Tracy profiler (fetched at configure time)" OFF)
set(BR_DEPS_ROOT "" CACHE PATH "Optional prebuilt dependency root from tools/scripts/bootstrap_deps.sh")

# All executables land in one directory next to a copy of the runtime assets so the
# game runs from the build tree exactly as it does from a packaged install.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CompilerOptions)
include(Dependencies)

add_subdirectory(third_party)
add_subdirectory(engine)
add_subdirectory(game)

if(BR_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(Packaging)
