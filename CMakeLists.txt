cmake_minimum_required(VERSION 3.20)

project(vix_kv
    VERSION 0.1.0
    DESCRIPTION "Vix KV - Durable local-first key-value engine powered by Softadastra"
    LANGUAGES CXX
)

include(GNUInstallDirs)

# ========================
# Options
# ========================
option(VIX_KV_BUILD_TESTS "Build kv tests" ON)
option(VIX_KV_BUILD_EXAMPLES "Build kv examples" ON)

# ========================
# Global settings
# ========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ========================
# Dependencies
# ========================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake")
endif()

# Try installed/exported package only if target is still missing
if(NOT TARGET softadastra::store AND NOT TARGET softadastra_store)
    find_package(softadastra_store CONFIG QUIET)
    find_package(store CONFIG QUIET)
endif()

set(VIX_KV_STORE_TARGET "")
if(TARGET softadastra::store)
    set(VIX_KV_STORE_TARGET softadastra::store)
elseif(TARGET softadastra_store)
    set(VIX_KV_STORE_TARGET softadastra_store)
endif()

if(NOT VIX_KV_STORE_TARGET)
    message(FATAL_ERROR
        "softadastra::store not found.\n"
        "Install dependencies with Vix:\n"
        "  vix add @softadastra/store\n"
        "  vix install\n"
        "or provide an installed/exported softadastra_store package."
    )
endif()

# ========================
# Library
# ========================
file(GLOB_RECURSE KV_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(vix_kv ${KV_SOURCES})
add_library(vix::kv ALIAS vix_kv)

target_compile_features(vix_kv PUBLIC cxx_std_20)

target_link_libraries(vix_kv
    PUBLIC
        ${VIX_KV_STORE_TARGET}
)

# ========================
# Include directories
# ========================
target_include_directories(vix_kv
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
# Target properties
# ========================
set_target_properties(vix_kv PROPERTIES
    OUTPUT_NAME vix_kv
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME kv
)

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
    enable_testing()
    add_subdirectory(tests)
endif()

# ========================
# Summary
# ========================
message(STATUS "------------------------------------------------------")
message(STATUS "vix::kv configured (${PROJECT_VERSION})")
message(STATUS "Store target:    ${VIX_KV_STORE_TARGET}")
message(STATUS "Include dir:     ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Build tests:     ${VIX_KV_BUILD_TESTS}")
message(STATUS "Build examples:  ${VIX_KV_BUILD_EXAMPLES}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
