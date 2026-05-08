# Examples for vix_kv

set(VIX_KV_EXAMPLE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/examples")

function(add_vix_kv_example target source)
    add_executable(${target} ${source})

    target_link_libraries(${target}
        PRIVATE
            vix::kv
    )

    target_compile_features(${target}
        PRIVATE
            cxx_std_20
    )

    set_target_properties(${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${VIX_KV_EXAMPLE_OUTPUT_DIR}"
    )
endfunction()

# Small examples
add_vix_kv_example(kv_hello hello.cpp)
add_vix_kv_example(kv_memory memory.cpp)
add_vix_kv_example(kv_delete delete.cpp)
add_vix_kv_example(kv_stats stats.cpp)
add_vix_kv_example(kv_persistence_small persistence_small.cpp)

# Larger examples
add_vix_kv_example(kv_basic basic.cpp)
add_vix_kv_example(kv_list list.cpp)
add_vix_kv_example(kv_persistent persistent.cpp)
add_vix_kv_example(kv_compact compact.cpp)
