cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fdiagnostics-color=always)
endif()

# MIT License Copyright (c) 2024-2025 Tomáš Mark

# +-+-+-+-+-+-+-+-+-+
# |g|t|e|s|t|s|   |
# +-+-+-+-+-+-+-+-+-+

set(TEST_NAME LibTester)
project(${TEST_NAME} LANGUAGES CXX)
include(CTest)
include(../../cmake/CPM.cmake)
include(GoogleTest)

# ==============================================================================
# Set target properties
# ==============================================================================

# A. Way given by CPM.cmake
CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.16.0
    OPTIONS "INSTALL_GTEST OFF")
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# B. Way given via (Conan) find_package(GTest REQUIRED)

# configure the test executable
add_executable(${TEST_NAME} ${TEST_SOURCES})
target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main
                                           dotname::standalone_common)
set_target_properties(${TEST_NAME} PROPERTIES OUTPUT_NAME "${TEST_NAME}")

# Skip test discovery when cross-compiling (includes Emscripten)
if(DOTNAME_CROSSCOMPILING)
    message(STATUS "Skipping gtest_discover_tests for ${TEST_NAME} (cross-compiling)")
else()
    gtest_discover_tests(${TEST_NAME})
endif()

# ==============================================================================
# Set target properties
# ==============================================================================
include(../../cmake/tmplt-debug.cmake)
apply_debug_info_control(${TEST_NAME})

# emscripten handler
include(../../cmake/tmplt-emscripten.cmake)
if(NOT DEFINED target)
    # Ensure the emscripten() helper has a default 'target' variable to reference
    set(target ${TEST_NAME})
endif()
emscripten(${TEST_NAME} 1 0 "--preload-file ../../../../../assets@share/${target}/assets")
