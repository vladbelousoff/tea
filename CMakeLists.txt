cmake_minimum_required(VERSION 3.20)

project(tea_lang)

set(CMAKE_C_STANDARD 99)

# Enable warnings as errors
if (MSVC)
    # Microsoft Visual C++ compiler
    add_compile_options(/WX)
else ()
    # GCC, Clang, and other compilers
    add_compile_options(-Werror)
endif ()

# Include FetchContent for downloading external dependencies
include(FetchContent)

FetchContent_Declare(
        rtlib
        GIT_REPOSITORY https://github.com/vladbelousoff/rtlib.git
        GIT_TAG 433c78f93e14cd1def7975df16bf74c37896fefd
)

FetchContent_MakeAvailable(rtlib)

# Download and build Lemon parser generator
FetchContent_Declare(
        lemon
        GIT_REPOSITORY https://github.com/vladbelousoff/lemon
        GIT_TAG 76e06e6a449723c7a1fafa6a3bb13b4c8c3c0b6a
)

FetchContent_MakeAvailable(lemon)

# Build the lemon executable
add_executable(lemon_exe
        ${lemon_SOURCE_DIR}/lemon.c
)

# Make sure lemon_exe is built before we try to use it
set_target_properties(lemon_exe PROPERTIES
        OUTPUT_NAME "lemon"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set(LEMON_EXECUTABLE "$<TARGET_FILE:lemon_exe>")

# Path to the lemon template file (lempar.c)
set(LEMON_TEMPLATE "${lemon_SOURCE_DIR}/lempar.c")

add_custom_command(
        OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/tea.c"
        "${CMAKE_CURRENT_BINARY_DIR}/tea.h"
        COMMAND
        ${LEMON_EXECUTABLE}
        -T${LEMON_TEMPLATE}
        -d${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tea.y
        DEPENDS
        lemon_exe
        ${CMAKE_CURRENT_SOURCE_DIR}/tea.y
        ${LEMON_TEMPLATE}
        COMMENT "Generating TEA parser with Lemon"
        VERBATIM
)

# Create a library for the generated parser
add_library(tea_parser
        "${CMAKE_CURRENT_BINARY_DIR}/tea.c"
        "${CMAKE_CURRENT_BINARY_DIR}/tea.h"
)

# Include directories for the parser
target_include_directories(tea_parser PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        include/
)

target_link_libraries(tea_parser PUBLIC rtlib::rtlib)

add_library(tea_lang
        src/tea_lexer.c
        src/tea_token.c
        src/tea_ast.c
        src/tea_parser.c
)

target_include_directories(tea_lang PUBLIC
        include/
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(tea_lang PUBLIC
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)

target_link_libraries(tea_lang PUBLIC
        tea_parser
        rtlib::rtlib
)

# Main executable for testing the simplified parser
add_executable(tea src/main.c)

target_link_libraries(tea PRIVATE
        tea_lang
        tea_parser
)

# Optional: Add custom target for easier grammar regeneration
add_custom_target(regenerate_parser
        COMMAND
        ${LEMON_EXECUTABLE}
        -T${LEMON_TEMPLATE}
        -d${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tea.y
        DEPENDS
        lemon_exe
        ${CMAKE_CURRENT_SOURCE_DIR}/tea.y
        COMMENT "Regenerating TEA parser"
        VERBATIM
)

# Make sure parser is generated before building tea_parser
add_dependencies(tea_parser lemon_exe)

message(STATUS "TEA Language")
message(STATUS "Grammar file: ${CMAKE_CURRENT_SOURCE_DIR}/tea.y")
message(STATUS "Generated parser: ${CMAKE_CURRENT_BINARY_DIR}/tea.c")

# Testing executable (only built if this is the main project)
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Enable testing
    enable_testing()

    # Find all .tea files in the examples directory
    file(GLOB TEA_EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.tea")

    # Create tests for each .tea file
    foreach (TEA_FILE ${TEA_EXAMPLE_FILES})
        # Get just the filename without path
        get_filename_component(TEA_FILENAME ${TEA_FILE} NAME)
        # Create a test name by removing the .tea extension
        string(REPLACE ".tea" "" TEST_NAME ${TEA_FILENAME})

        # Add a test that runs the tea executable with this file
        add_test(
                NAME "test_${TEST_NAME}"
                COMMAND tea ${TEA_FILE}
        )

        # Set the working directory to the build directory
        set_tests_properties("test_${TEST_NAME}" PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endforeach ()
endif ()
