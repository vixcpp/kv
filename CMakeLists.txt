cmake_minimum_required(VERSION 3.20)

project(vix_kv
    VERSION 0.1.0
    DESCRIPTION "Vix KV - Durable local-first key-value engine"
    LANGUAGES CXX
)

include(GNUInstallDirs)

# ========================
# Options
# ========================
option(VIX_KV_BUILD_TESTS "Build Vix KV tests" ON)
option(VIX_KV_BUILD_EXAMPLES "Build Vix KV examples" ON)

# ========================
# Global settings
# ========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ========================
# Sources
# ========================
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

# ========================
# Library
# ========================
add_library(vix_kv ${VIX_KV_SOURCES})
add_library(vix::kv ALIAS vix_kv)

target_compile_features(vix_kv PUBLIC cxx_std_20)

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
    target_compile_options(vix_kv PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
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
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
        add_subdirectory(examples)
    endif()
endif()

# ========================
# Tests
# ========================
if(VIX_KV_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

# ========================
# Install
# ========================
install(
    TARGETS vix_kv
    EXPORT vix_kvTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT vix_kvTargets
    NAMESPACE vix::
    FILE vix_kvTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vix_kv
)

# ========================
# Summary
# ========================
message(STATUS "------------------------------------------------------")
message(STATUS "vix::kv configured (${PROJECT_VERSION})")
message(STATUS "Include dir:     ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Build tests:     ${VIX_KV_BUILD_TESTS}")
message(STATUS "Build examples:  ${VIX_KV_BUILD_EXAMPLES}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
