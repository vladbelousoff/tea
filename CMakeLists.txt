cmake_minimum_required(VERSION 3.20)

project(tea_lang)

set(CMAKE_C_STANDARD 99)

# Include FetchContent for downloading external dependencies
include(FetchContent)

FetchContent_Declare(
        rtlib
        GIT_REPOSITORY https://github.com/vladbelousoff/rtlib.git
        GIT_TAG 4c70ddee4ab26a5005cf6d76f19599c538a65b26
)

FetchContent_MakeAvailable(rtlib)

# Download and build Lemon parser generator
FetchContent_Declare(
        lemon
        GIT_REPOSITORY https://github.com/vladbelousoff/lemon
        GIT_TAG c9aae223bb9ad4ffc991dd90717fa6a9393c732b
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

set(TEA_HEADER_FILES
        include/tea_lexer.h
        include/tea_token.h
        include/tea_ast.h
        include/tea_parser.h
        include/tea_interpret.h
        include/tea_value.h
        include/tea_scope.h
        include/tea_expression.h
        include/tea_statement.h
        include/tea_function.h
        include/tea_struct.h
)

set(TEA_SOURCE_FILES
        src/tea_lexer.c
        src/tea_token.c
        src/tea_ast.c
        src/tea_parser.c
        src/tea_interpret.c
        src/tea_value.c
        src/tea_scope.c
        src/tea_expression.c
        src/tea_statement.c
        src/tea_function.c
        src/tea_struct.c
)

add_library(tea_lang
        ${TEA_HEADER_FILES}
        ${TEA_SOURCE_FILES}
)

target_include_directories(tea_lang PUBLIC
        include/
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(tea_lang PUBLIC
        $<$<CONFIG:Debug>:TEA_DEBUG_BUILD>
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)

target_link_libraries(tea_lang PUBLIC
        tea_parser
        rtlib::rtlib
)

# Main executable for testing the simplified parser
add_executable(tea main.c)

target_link_libraries(tea PRIVATE
        tea_lang
        tea_parser
)

# Make sure parser is generated before building tea_parser
add_dependencies(tea_parser lemon_exe)

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

        # Set the working directory to the build directory and ensure tea target is built
        set_tests_properties("test_${TEST_NAME}" PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                REQUIRED_FILES $<TARGET_FILE:tea>
        )
    endforeach ()
endif ()
