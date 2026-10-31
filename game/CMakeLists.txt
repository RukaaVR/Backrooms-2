# br_game: all Backrooms-specific gameplay, levels and UI. The executable is a
# thin main() so that tests and tools can link the full game library.
file(GLOB_RECURSE BR_GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
list(FILTER BR_GAME_SOURCES EXCLUDE REGEX ".*/game/main\\.cpp$")

add_library(br_game STATIC ${BR_GAME_SOURCES})
br_configure_target(br_game)
target_link_libraries(br_game PUBLIC br_engine)
set_target_properties(br_game PROPERTIES FOLDER "Game")

add_executable(BackroomsBeyondReality WIN32 main.cpp)
br_configure_target(BackroomsBeyondReality)
target_link_libraries(BackroomsBeyondReality PRIVATE br_game)
set_target_properties(BackroomsBeyondReality PROPERTIES FOLDER "Game")
if(WIN32)
    add_custom_command(TARGET BackroomsBeyondReality POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:BackroomsBeyondReality> $<TARGET_FILE_DIR:BackroomsBeyondReality>
        COMMAND_EXPAND_LISTS)
endif()
