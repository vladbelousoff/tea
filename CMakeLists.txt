cmake_minimum_required(VERSION 3.20)

project(tea-lang)

set(CMAKE_C_STANDARD 99)

# Include FetchContent for downloading external dependencies
include(FetchContent)

FetchContent_Declare(
        rtlib
        GIT_REPOSITORY https://github.com/vladbelousoff/rtlib.git
        GIT_TAG 4ca25dcbda602f8adce4b38c34fd4b5f23ff97e7
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

# Path to the lemon executable
set(LEMON_EXECUTABLE "${CMAKE_BINARY_DIR}/bin/lemon")

# Path to the lemon template file (lempar.c)
set(LEMON_TEMPLATE "${lemon_SOURCE_DIR}/lempar.c")

# Custom command to generate parser from simplified grammar
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
        COMMENT "Generating simplified TEA parser with Lemon"
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

# Simplified TEA language implementation - only what we need
add_library(tea_lang
        src/tokenizer.c
        src/ast.c
        src/parser.c
)

target_include_directories(tea_lang PUBLIC
        include/
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(tea_lang PUBLIC tea_parser)

# Main executable for testing the simplified parser
add_executable(tea src/main.c)

target_link_libraries(tea PRIVATE
        tea_lang
        tea_parser
        rtlib::rtlib
)

target_compile_definitions(rtlib PUBLIC
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
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
        COMMENT "Regenerating simplified TEA parser"
        VERBATIM
)

# Make sure parser is generated before building tea_parser
add_dependencies(tea_parser lemon_exe)

# Print status
message(STATUS "Simplified TEA Language Parser")
message(STATUS "Features: Functions with empty bodies and attributes only")
message(STATUS "Lemon parser generator will be built and used to generate TEA parser")
message(STATUS "Grammar file: ${CMAKE_CURRENT_SOURCE_DIR}/tea.y")
message(STATUS "Generated parser: ${CMAKE_CURRENT_BINARY_DIR}/tea.c")
message(STATUS "Example file: ${CMAKE_CURRENT_SOURCE_DIR}/example.tea")