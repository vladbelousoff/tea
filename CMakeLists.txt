cmake_minimum_required(VERSION 3.20)
project(tea_lang C)

set(CMAKE_C_STANDARD 99)

# Fetch Lemon parser generator
include(FetchContent)
FetchContent_Declare(lemon
    GIT_REPOSITORY https://github.com/vladbelousoff/lemon
    GIT_TAG c9aae223bb9ad4ffc991dd90717fa6a9393c732b
)
FetchContent_MakeAvailable(lemon)

# Build Lemon executable
add_executable(lemon_exe ${lemon_SOURCE_DIR}/lemon.c)
set_target_properties(lemon_exe PROPERTIES 
    OUTPUT_NAME "lemon"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Generate parser
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tea_grammar.c" "${CMAKE_CURRENT_BINARY_DIR}/tea_grammar.h"
    COMMAND "$<TARGET_FILE:lemon_exe>" 
            "-T${lemon_SOURCE_DIR}/lempar.c" 
            "-d${CMAKE_CURRENT_BINARY_DIR}" 
            "${CMAKE_CURRENT_SOURCE_DIR}/tea_grammar.y"
    DEPENDS lemon_exe tea_grammar.y "${lemon_SOURCE_DIR}/lempar.c"
    COMMENT "Generating TEA parser"
    VERBATIM
)

# Parser library
add_library(tea_parser "${CMAKE_CURRENT_BINARY_DIR}/tea_grammar.c")
target_include_directories(tea_parser PUBLIC ${CMAKE_CURRENT_BINARY_DIR} include/)

# Main tea library
file(GLOB_RECURSE TEA_SOURCES "src/*.c" "include/*.h")
add_library(tea_lang ${TEA_SOURCES})
target_include_directories(tea_lang PUBLIC include/ ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tea_lang PUBLIC tea_parser)

# Compile definitions
target_compile_definitions(tea_lang PUBLIC
    $<$<CONFIG:Debug>:TEA_DEBUG_BUILD>
    $<$<CONFIG:Debug>:TEA_DEBUG_LEVEL=4>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)

# Main executable
add_executable(tea main.c)
target_link_libraries(tea PRIVATE tea_lang)

# Tests (only if this is the main project)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    enable_testing()
    file(GLOB TEA_EXAMPLES "examples/*.tea")
    foreach(TEA_FILE ${TEA_EXAMPLES})
        get_filename_component(TEST_NAME ${TEA_FILE} NAME_WE)
        add_test(NAME ${TEST_NAME} COMMAND tea ${TEA_FILE})
        set_tests_properties(${TEST_NAME} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endforeach()
endif()