# br_engine: reusable, game-agnostic engine library.
# Sources are collected per module directory with CONFIGURE_DEPENDS so that
# parallel feature work never conflicts on a central source list.
file(GLOB_RECURSE BR_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(br_engine STATIC ${BR_ENGINE_SOURCES})
br_configure_target(br_engine)
target_include_directories(br_engine PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(br_engine
    PUBLIC
        SDL3::SDL3
        glm::glm
        nlohmann_json::nlohmann_json
        Jolt::Jolt
        ${BR_OPENAL_TARGET}
        br_glad
        br_stb
    PRIVATE
        Threads::Threads)
find_package(Threads REQUIRED)
set_target_properties(br_engine PROPERTIES FOLDER "Engine")

# Runtime data layout (identical in the build tree and in packages):
#   <bin>/data/shaders  <- engine/shaders
#   <bin>/data/assets   <- assets
add_custom_target(br_runtime_data ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/engine/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/assets
    COMMENT "Copying runtime data (shaders, assets)")
set_target_properties(br_runtime_data PROPERTIES FOLDER "Engine")
add_dependencies(br_engine br_runtime_data)
