# Unit tests for vix_kv

add_executable(vix_kv_keypath_test
    keys/KeyPath_test.cpp
)
target_link_libraries(vix_kv_keypath_test PRIVATE vix::kv)
add_test(NAME vix_kv_keypath_test COMMAND vix_kv_keypath_test)

add_executable(vix_kv_keyencoder_test
    keys/KeyEncoder_test.cpp
)
target_link_libraries(vix_kv_keyencoder_test PRIVATE vix::kv)
add_test(NAME vix_kv_keyencoder_test COMMAND vix_kv_keyencoder_test)

add_executable(vix_kv_kvvalue_test
    values/KvValue_test.cpp
)
target_link_libraries(vix_kv_kvvalue_test PRIVATE vix::kv)
add_test(NAME vix_kv_kvvalue_test COMMAND vix_kv_kvvalue_test)

add_executable(vix_kv_valuecodec_test
    values/ValueCodec_test.cpp
)
target_link_libraries(vix_kv_valuecodec_test PRIVATE vix::kv)
add_test(NAME vix_kv_valuecodec_test COMMAND vix_kv_valuecodec_test)
