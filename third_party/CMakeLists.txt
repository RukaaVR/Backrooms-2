# Vendored single-file libraries. These are compiled once into small static
# libraries so their (large, warning-heavy) implementations stay out of project
# translation units.

add_library(br_glad STATIC glad/src/gl.c)
target_include_directories(br_glad SYSTEM PUBLIC glad/include)
set_target_properties(br_glad PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

add_library(br_stb STATIC stb_impl.c)
target_include_directories(br_stb SYSTEM PUBLIC stb)
set_target_properties(br_stb PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
