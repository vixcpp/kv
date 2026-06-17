# Tests for vix_kv

set(VIX_KV_TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tests")

function(vix_kv_add_test test_name test_source)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")
    add_executable(${test_name}
      "${test_source}"
    )

    target_compile_features(${test_name}
      PRIVATE
        cxx_std_20
    )

    target_link_libraries(${test_name}
      PRIVATE
        vix::kv
    )

    target_include_directories(${test_name}
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
      target_compile_options(${test_name}
        PRIVATE
          -fno-omit-frame-pointer
          -fsanitize=address,undefined
      )

      target_link_options(${test_name}
        PRIVATE
          -fsanitize=address,undefined
      )
    endif()

    set_target_properties(${test_name}
      PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_TEST_OUTPUT_DIR}"
    )

    add_test(
      NAME ${test_name}
      COMMAND ${test_name}
    )
  endif()
endfunction()

# ========================
# Unit tests
# ========================
vix_kv_add_test(kv_test_crc32 unit/test_crc32.cpp)
vix_kv_add_test(kv_test_endian unit/test_endian.cpp)
vix_kv_add_test(kv_test_value_codec unit/test_value_codec.cpp)

vix_kv_add_test(kv_test_key_path unit/test_key_path.cpp)
vix_kv_add_test(kv_test_key_validator unit/test_key_validator.cpp)
vix_kv_add_test(kv_test_key_encoder unit/test_key_encoder.cpp)

vix_kv_add_test(kv_test_kv_record unit/test_kv_record.cpp)
vix_kv_add_test(kv_test_kv_record_codec unit/test_kv_record_codec.cpp)

vix_kv_add_test(kv_test_memtable unit/test_memtable.cpp)
vix_kv_add_test(kv_test_kv_index unit/test_kv_index.cpp)

# ========================
# WAL tests
# ========================
vix_kv_add_test(kv_test_wal_writer wal/test_wal_writer.cpp)
vix_kv_add_test(kv_test_wal_reader wal/test_wal_reader.cpp)
vix_kv_add_test(kv_test_wal_recovery wal/test_wal_recovery.cpp)

# ========================
# Storage tests
# ========================
vix_kv_add_test(kv_test_data_file storage/test_data_file.cpp)
vix_kv_add_test(kv_test_segment storage/test_segment.cpp)

# ========================
# Snapshot tests
# ========================
vix_kv_add_test(kv_test_snapshot snapshot/test_snapshot.cpp)

# ========================
# Compaction tests
# ========================
vix_kv_add_test(kv_test_compactor compaction/test_compactor.cpp)

# ========================
# Engine tests
# ========================
vix_kv_add_test(kv_test_kv_engine engine/test_kv_engine.cpp)

# ========================
# Public API tests
# ========================
vix_kv_add_test(kv_test_open api/test_open.cpp)
vix_kv_add_test(kv_test_basic api/test_basic.cpp)
vix_kv_add_test(kv_test_public_api api/test_public_api.cpp)
vix_kv_add_test(kv_test_persistence api/test_persistence.cpp)

# Temporary compatibility test during migration.
vix_kv_add_test(kv_test_basic_legacy test_basic.cpp)
