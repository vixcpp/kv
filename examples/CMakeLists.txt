# Examples for vix_kv

set(VIX_KV_EXAMPLE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/examples")

add_executable(kv_basic basic.cpp)
target_link_libraries(kv_basic PRIVATE vix::kv)
set_target_properties(kv_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)

add_executable(kv_compact compact.cpp)
target_link_libraries(kv_compact PRIVATE vix::kv)
set_target_properties(kv_compact PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)

add_executable(kv_list list.cpp)
target_link_libraries(kv_list PRIVATE vix::kv)
set_target_properties(kv_list PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)

add_executable(kv_persistent persistent.cpp)
target_link_libraries(kv_persistent PRIVATE vix::kv)
set_target_properties(kv_persistent PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)
