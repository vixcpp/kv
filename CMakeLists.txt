#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - KV Module
# ====================================================================
# Purpose:
#   Durable local-first key-value engine for Vix.
#   Provides a compact storage engine with WAL, snapshots,
#   segments, indexing, checksums, compaction, and public API helpers.
#
# Public Targets:
#   - vix_kv  : The actual library target
#   - vix::kv : Namespaced alias for consumers
#
# Installation/Export:
#   - Umbrella build exports into: VixTargets
#   - Standalone build exports into: vix_kvTargets
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_kv VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# --------------------------------------------------------------------
# Global settings
# --------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------
# Export set selection
# --------------------------------------------------------------------
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_KV_EXPORT_SET VixTargets)
else()
  set(VIX_KV_EXPORT_SET vix_kvTargets)
endif()

# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------
option(VIX_KV_BUILD_EXAMPLES "Build KV examples" OFF)
option(VIX_KV_BUILD_TESTS "Build KV tests" OFF)
option(VIX_KV_BUILD_BENCHMARKS "Build KV benchmarks" OFF)

# --------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------
set(VIX_KV_SOURCES
  src/api/Kv.cpp
  src/api/Open.cpp

  src/checksum/Crc32.cpp

  src/compaction/Compactor.cpp

  src/index/KvIndex.cpp

  src/internal/KvEngine.cpp

  src/keys/KeyEncoder.cpp

  src/memtable/MemTable.cpp

  src/records/KvRecordEncoder.cpp
  src/records/KvRecordDecoder.cpp

  src/snapshot/SnapshotWriter.cpp
  src/snapshot/SnapshotReader.cpp

  src/storage/DataFileWriter.cpp
  src/storage/DataFileReader.cpp
  src/storage/SegmentWriter.cpp
  src/storage/SegmentReader.cpp

  src/utils/AtomicFile.cpp

  src/values/ValueCodec.cpp

  src/wal/KvWalWriter.cpp
  src/wal/KvWalReader.cpp
  src/wal/KvWalRecovery.cpp
)

# --------------------------------------------------------------------
# Library target
# --------------------------------------------------------------------
add_library(vix_kv STATIC ${VIX_KV_SOURCES})
add_library(vix::kv ALIAS vix_kv)

target_compile_features(vix_kv PUBLIC cxx_std_20)

target_include_directories(vix_kv
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (MSVC)
  target_compile_options(vix_kv PRIVATE /W4 /permissive-)
else()
  target_compile_options(vix_kv PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(vix_kv PRIVATE
    -fno-omit-frame-pointer
    -fsanitize=address,undefined
  )

  target_link_options(vix_kv PRIVATE
    -fsanitize=address,undefined
  )
endif()

set_target_properties(vix_kv PROPERTIES
  OUTPUT_NAME vix_kv
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
  EXPORT_NAME kv
)

# --------------------------------------------------------------------
# Install target
# --------------------------------------------------------------------
install(TARGETS vix_kv
  EXPORT ${VIX_KV_EXPORT_SET}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# --------------------------------------------------------------------
# Install headers
# --------------------------------------------------------------------
if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.h"
  )
endif()

# --------------------------------------------------------------------
# Standalone package config/export
# Only for non-umbrella builds
# --------------------------------------------------------------------
if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix-kv-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_kvConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_kv"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vix_kvConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vix_kvConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_kvConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_kv"
  )

  install(EXPORT vix_kvTargets
    FILE vix_kvTargets.cmake
    NAMESPACE vix::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_kv"
  )

endif()

# --------------------------------------------------------------------
# Examples
# --------------------------------------------------------------------
if (VIX_KV_BUILD_EXAMPLES)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
  endif()
endif()

# --------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------
if (VIX_KV_BUILD_TESTS)
  include(CTest)
  enable_testing()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()

# --------------------------------------------------------------------
# Benchmarks
# --------------------------------------------------------------------
if (VIX_KV_BUILD_BENCHMARKS)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
  endif()
endif()

# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "vix::kv configured (${PROJECT_VERSION})")
message(STATUS "Mode: STATIC")
message(STATUS "Export set: ${VIX_KV_EXPORT_SET}")
message(STATUS "Include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include (for <vix/kv/...>)")
message(STATUS "Build examples: ${VIX_KV_BUILD_EXAMPLES}")
message(STATUS "Build tests: ${VIX_KV_BUILD_TESTS}")
message(STATUS "Build benchmarks: ${VIX_KV_BUILD_BENCHMARKS}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
