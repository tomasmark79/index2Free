cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# MIT License Copyright (c) 2024-2025 Tomáš Mark

# +-+-+-+-+-+-+-+-+-+-+
# LIBRARY
# +-+-+-+-+-+-+-+-+-+-+

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0091 NEW)

option(ENABLE_CCACHE "Use ccache compiler cache" ON)
option(BUILD_SHARED_LIBS "Build shared (.so) libraries" OFF)
option(USE_STATIC_RUNTIME "Link C++ runtime statically" ON)
option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)
option(SANITIZE_UNDEFINED "Enable undefined behavior sanitizer" OFF)
option(SANITIZE_THREAD "Enable thread sanitizer" OFF)
option(SANITIZE_MEMORY "Enable memory sanitizer" OFF)
option(ENABLE_HARDENING "Enable security hardening" OFF)
option(ENABLE_IPO "Enable link-time optimization" OFF)

option(ENABLE_GTESTS "Build and run unit tests" ON)

if(ENABLE_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()
include(cmake/tmplt-runtime.cmake)
include(cmake/tmplt-sanitizer.cmake)
include(cmake/tmplt-hardening.cmake)
include(cmake/tmplt-ipo.cmake)
include(cmake/tmplt-debug.cmake)

# Linting C/C++ code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Library attributes - case size will reflect resource files case size
# ==============================================================================
set(LIBRARY_NAME CoreLib)
set(LIBRARY_NAMESPACE dotname)
project(
    ${LIBRARY_NAME}
    VERSION 0.0.1
    LANGUAGES C CXX
    DESCRIPTION "template Copyright (c) 2024 TomasMark [at] digitalspace.name"
    HOMEPAGE_URL "https://github.com/tomasmark79")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        WARNING
            "In-source builds. Please make a new directory (called a Build directory) and run CMake from there."
    )
endif()

# ==============================================================================
# Library installation attributes
# ==============================================================================
set(INSTALL_INCLUDE_DIR include/${LIBRARY_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}/
        DESTINATION ${INSTALL_INCLUDE_DIR})

# ==============================================================================
# System / Conan dependencies
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(glm CONFIG REQUIRED)

# find_package(glew REQUIRED)
if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")

    # set(OPENGL_opengl_LIBRARY "/usr/lib64/libOpenGL.so")
    # set(OPENGL_glx_LIBRARY "/usr/lib64/libGLX.so")

    find_package(OpenGL REQUIRED MODULE)
    find_package(GLEW REQUIRED)

    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)
    find_package(SDL2_net CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)
endif()
find_package(imgui REQUIRED)

# ==============================================================================
# CPM.cmake dependencies - take care conflicts
# ==============================================================================
# export CPM_SOURCE_CACHE=$HOME/.cache/CPM # for localy caching CPM packages
# ==============================================================================
include(cmake/CPM.cmake)
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.12.0")
CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.7")
cpm_licenses_create_disclaimer_target(
    write-licenses-${LIBRARY_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}_third_party.txt"
    "${CPM_PACKAGES}")

# ==============================================================================
# public header files
# ==============================================================================
file(
    GLOB_RECURSE
    headers
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx)

# ==============================================================================
# source files and `internal` header files that are not intended for public use
# ==============================================================================

# Common source files (excluding platform-specific GUI files)
file(
    GLOB_RECURSE
    common_sources
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

# Remove platform-specific files from common sources
list(REMOVE_ITEM common_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/EmscriptenPlatform.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/DesktopPlatform.cpp)

# Platform-specific source files
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(platform_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/EmscriptenPlatform.cpp)
    message(STATUS "Using Emscripten platform sources")
else()
    set(platform_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/DesktopPlatform.cpp)
    message(STATUS "Using Desktop platform sources")
endif()

# Combine all sources
set(sources ${common_sources} ${platform_sources})

# ==============================================================================
# Create target
# ==============================================================================
add_library(${LIBRARY_NAME})
target_sources(${LIBRARY_NAME} PRIVATE ${headers} ${sources})

# Target-specific statické linkování
if(USE_STATIC_RUNTIME AND NOT MSVC)
    target_link_options(${LIBRARY_NAME} PRIVATE -static-libgcc -static-libstdc++)
    target_compile_options(${LIBRARY_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

apply_ipo(${LIBRARY_NAME})
apply_hardening(${LIBRARY_NAME})
apply_sanitizers(${LIBRARY_NAME})
apply_static_runtime(${LIBRARY_NAME})
apply_debug_info_control(${LIBRARY_NAME})

# ==============================================================================
# Set target properties
# ==============================================================================
# emscripten handler
include(cmake/tmplt-emscripten.cmake)
emscripten(${LIBRARY_NAME} 0 1 "")

# ==============================================================================
# Set headers
# ==============================================================================
# header-only libraries change all PUBLIC flags to INTERFACE and create an interface target:
# <add_library(${LIBRARY_NAME} INTERFACE)>
target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    PUBLIC $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>)

# + stb_image dependencies
CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG master)
if(stb_ADDED)
    target_include_directories(${LIBRARY_NAME} PRIVATE ${stb_SOURCE_DIR})
endif()

# ==============================================================================
# Set compile options
# ==============================================================================
# note: generator expressions are evaluated during generation of the buildsystem, and not during
# processing of CMakeLists.txt files
target_compile_options(
    ${LIBRARY_NAME}
    PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive-;/W4>"
    PUBLIC
        "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CXX,MSVC>>,$<NOT:$<PLATFORM_ID:Darwin>>,$<NOT:$<CXX_COMPILER_ID:Clang>>>:-Wall;-Wextra;-Wpedantic;-MMD;-MP>"
    PUBLIC
        "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CXX,MSVC>>,$<PLATFORM_ID:Darwin>>:-Wall;-Wextra;-Wpedantic>"
)

# ==============================================================================
# Set compile features C++ version from Conan Profile has priority over this setting
# ==============================================================================
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Set linking
# ==============================================================================
target_link_libraries(
    ${LIBRARY_NAME}
    PUBLIC fmt::fmt
    PUBLIC nlohmann_json::nlohmann_json
    PUBLIC glm::glm
    PRIVATE imgui::imgui)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    target_link_libraries(${LIBRARY_NAME} PUBLIC OpenGL::GL)
    target_link_libraries(${LIBRARY_NAME} PUBLIC GLEW::GLEW)
    target_link_libraries(${LIBRARY_NAME} PUBLIC SDL2::SDL2main SDL2::SDL2)
    target_link_libraries(${LIBRARY_NAME} PUBLIC SDL2_image::SDL2_image)
    target_link_libraries(${LIBRARY_NAME} PUBLIC SDL2_ttf::SDL2_ttf-static)
    target_link_libraries(${LIBRARY_NAME} PUBLIC SDL2_net::SDL2_net)
    target_link_libraries(${LIBRARY_NAME} PUBLIC SDL2_mixer::SDL2_mixer)
endif()

# ==============================================================================
# set packageProject arttributes for library
# ==============================================================================
packageProject(
    NAME ${LIBRARY_NAME}
    VERSION ${PROJECT_VERSION}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR "/include"
    INCLUDE_DESTINATION "include"
    INCLUDE_HEADER_PATTERN "*.h;*.hpp;*.hh;*.hxx"
    DEPENDENCIES "fmt#11.1.0;CPMLicenses.cmake@0.0.7"
    VERSION_HEADER "${LIBRARY_NAME}/version.h"
    EXPORT_HEADER "${LIBRARY_NAME}/export.h"
    NAMESPACE ${LIBRARY_NAMESPACE}
    # `AnyNewerVersion|SameMajorVersion|SameMinorVersion'
    COMPATIBILITY AnyNewerVersion
    DISABLE_VERSION_SUFFIX YES
    # YES for header only (i.e. INTERFACE) libraries
    ARCH_INDEPENDENT NO
    CPACK YES
    RUNTIME_DESTINATION /)
