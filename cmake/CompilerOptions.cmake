# Project-wide compiler configuration. Third-party targets are deliberately left
# alone; project targets opt in through br_configure_target().

if(BR_USE_CCACHE)
    find_program(BR_CCACHE_PROGRAM ccache)
    if(BR_CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER ${BR_CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${BR_CCACHE_PROGRAM})
        message(STATUS "Using ccache: ${BR_CCACHE_PROGRAM}")
    endif()
endif()

if(BR_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

function(br_configure_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus /MP
            /wd4100 /wd4201 /wd4324)
        target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
        if(BR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow=local
            -Wno-unused-parameter -Wno-missing-field-initializers)
        if(BR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:BR_DEBUG=1>
        GLM_FORCE_DEPTH_ZERO_TO_ONE=0
        GLM_ENABLE_EXPERIMENTAL)
endfunction()
