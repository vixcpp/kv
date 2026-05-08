# Tests for vix_kv

set(VIX_KV_TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tests")

add_executable(kv_test_basic test_basic.cpp)

target_link_libraries(kv_test_basic
    PRIVATE
        vix::kv
)

target_include_directories(kv_test_basic
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(kv_test_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_TEST_OUTPUT_DIR}"
)

add_test(
    NAME kv_test_basic
    COMMAND kv_test_basic
)
