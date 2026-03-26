# Integration tests for vix_kv

add_executable(vix_kv_open_close_test
    open_close_test.cpp
)
target_link_libraries(vix_kv_open_close_test PRIVATE vix::kv)
add_test(NAME vix_kv_open_close_test COMMAND vix_kv_open_close_test)

add_executable(vix_kv_set_get_test
    set_get_test.cpp
)
target_link_libraries(vix_kv_set_get_test PRIVATE vix::kv)
add_test(NAME vix_kv_set_get_test COMMAND vix_kv_set_get_test)

add_executable(vix_kv_erase_test
    erase_test.cpp
)
target_link_libraries(vix_kv_erase_test PRIVATE vix::kv)
add_test(NAME vix_kv_erase_test COMMAND vix_kv_erase_test)

add_executable(vix_kv_list_prefix_test
    list_prefix_test.cpp
)
target_link_libraries(vix_kv_list_prefix_test PRIVATE vix::kv)
add_test(NAME vix_kv_list_prefix_test COMMAND vix_kv_list_prefix_test)

add_executable(vix_kv_recovery_test
    recovery_test.cpp
)
target_link_libraries(vix_kv_recovery_test PRIVATE vix::kv)
add_test(NAME vix_kv_recovery_test COMMAND vix_kv_recovery_test)
