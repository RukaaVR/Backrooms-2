# Developer tools. Each subdirectory with a CMakeLists.txt is added automatically.
file(GLOB BR_TOOL_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(dir ${BR_TOOL_DIRS})
    if(IS_DIRECTORY ${dir} AND EXISTS ${dir}/CMakeLists.txt)
        add_subdirectory(${dir})
    endif()
endforeach()
