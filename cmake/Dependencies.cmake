# Third-party dependency resolution.
#
# Strategy:
#   * Pure C / header-only libraries (SDL3, GLM, nlohmann_json, Catch2) are first
#     looked up with find_package (system or BR_DEPS_ROOT prefix produced by
#     tools/scripts/bootstrap_deps.sh) and otherwise fetched from source.
#   * Jolt Physics is ALWAYS built in-tree. Jolt exports configuration-dependent
#     compile definitions (asserts, debug renderer, profiler) that must match the
#     consuming translation units exactly, so mixing a Release Jolt binary with a
#     Debug game would be an ODR violation. Sources are reused from BR_DEPS_ROOT
#     when present to avoid re-cloning; ccache makes rebuilds cheap.
#   * OpenAL Soft is taken from the system when available (Linux packages) and
#     otherwise built from source (Windows/macOS).
#
# Versions here must stay in sync with tools/scripts/bootstrap_deps.sh.

include(FetchContent)
set(FETCHCONTENT_QUIET ON)

if(BR_DEPS_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${BR_DEPS_ROOT}/prefix")
    foreach(dep sdl3 jolt glm catch2 json)
        string(TOUPPER ${dep} DEP_UPPER)
        if(EXISTS "${BR_DEPS_ROOT}/src/${dep}" AND NOT FETCHCONTENT_SOURCE_DIR_${DEP_UPPER})
            set(FETCHCONTENT_SOURCE_DIR_${DEP_UPPER} "${BR_DEPS_ROOT}/src/${dep}" CACHE PATH "" FORCE)
        endif()
    endforeach()
endif()

# --- SDL3 ---------------------------------------------------------------------
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found, fetching from source")
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(sdl3)
endif()

# --- GLM ----------------------------------------------------------------------
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE)
    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glm)
endif()

# --- nlohmann_json ------------------------------------------------------------
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE)
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(json)
endif()

# --- Jolt Physics (always in-tree, see above) ---------------------------------
set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
set(CPP_EXCEPTIONS_ENABLED ON CACHE BOOL "" FORCE)
FetchContent_Declare(jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG v5.3.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR Build)
FetchContent_MakeAvailable(jolt)
if(TARGET Jolt AND NOT TARGET Jolt::Jolt)
    add_library(Jolt::Jolt ALIAS Jolt)
endif()
if(TARGET Jolt)
    set_target_properties(Jolt PROPERTIES FOLDER "ThirdParty")
endif()

# --- OpenAL Soft --------------------------------------------------------------
find_package(OpenAL QUIET)
if(OpenAL_FOUND OR OPENAL_FOUND)
    if(NOT TARGET OpenAL::OpenAL)
        add_library(OpenAL::OpenAL UNKNOWN IMPORTED)
        set_target_properties(OpenAL::OpenAL PROPERTIES
            IMPORTED_LOCATION "${OPENAL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}")
    endif()
    set(BR_OPENAL_TARGET OpenAL::OpenAL)
else()
    message(STATUS "OpenAL not found, fetching OpenAL Soft from source")
    set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(openal
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG 1.24.2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(openal)
    set(BR_OPENAL_TARGET OpenAL::OpenAL)
endif()

# --- Catch2 (tests only) --------------------------------------------------------
if(BR_BUILD_TESTS)
    find_package(Catch2 3 CONFIG QUIET)
    if(NOT Catch2_FOUND)
        FetchContent_Declare(catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(catch2)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()
endif()

# --- Tracy (optional profiler) -------------------------------------------------
# Off by default: the built-in profiler (engine/core/Profiler.h) covers the
# in-game overlay; Tracy adds live remote capture for deep profiling sessions.
if(BR_ENABLE_TRACY)
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    FetchContent_Declare(tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(tracy)
endif()
