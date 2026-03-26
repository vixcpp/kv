# Examples for vix_kv

set(VIX_KV_EXAMPLE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/examples")

add_executable(kv_basic basic_kv.cpp)
target_link_libraries(kv_basic PRIVATE vix::kv)
set_target_properties(kv_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)

add_executable(kv_durable_restart durable_restart.cpp)
target_link_libraries(kv_durable_restart PRIVATE vix::kv)
set_target_properties(kv_durable_restart PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)

add_executable(kv_prefix_scan prefix_scan.cpp)
target_link_libraries(kv_prefix_scan PRIVATE vix::kv)
set_target_properties(kv_prefix_scan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
)
