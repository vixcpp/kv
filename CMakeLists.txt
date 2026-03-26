cmake_minimum_required(VERSION 3.20)

project(vix_kv
    VERSION 0.1.0
    DESCRIPTION "Vix KV - Durable local-first key-value engine powered by Softadastra"
    LANGUAGES CXX
)

# ========================
# Options
# ========================
option(VIX_KV_BUILD_TESTS "Build kv tests" ON)
option(VIX_KV_BUILD_EXAMPLES "Build kv examples" ON)

# ========================
# Library
# ========================
add_library(vix_kv)

target_compile_features(vix_kv PUBLIC cxx_std_20)

# ========================
# Dependencies
# ========================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake)
endif()

if(NOT TARGET softadastra::wal)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../wal/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../wal build_wal)
    else()
        message(FATAL_ERROR
            "softadastra::wal not found. "
            "Install dependencies with Vix or provide ../wal in the monorepo."
        )
    endif()
endif()

if(NOT TARGET softadastra::store)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../store/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../store build_store)
    else()
        message(FATAL_ERROR
            "softadastra::store not found. "
            "Install dependencies with Vix or provide ../store in the monorepo."
        )
    endif()
endif()

target_link_libraries(vix_kv
    PUBLIC
        softadastra::wal
        softadastra::store
)

# ========================
# Include directories
# ========================
target_include_directories(vix_kv
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ========================
# Sources
# ========================
file(GLOB_RECURSE KV_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

target_sources(vix_kv
    PRIVATE
        ${KV_SOURCES}
)

# ========================
# Compiler options
# ========================
if(MSVC)
    target_compile_options(vix_kv PRIVATE /W4 /permissive-)
else()
    target_compile_options(vix_kv PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ========================
# Position Independent Code
# ========================
set_target_properties(vix_kv PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# ========================
# Namespacing
# ========================
add_library(vix::kv ALIAS vix_kv)

# ========================
# Examples
# ========================
if(VIX_KV_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ========================
# Tests
# ========================
if(VIX_KV_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()
