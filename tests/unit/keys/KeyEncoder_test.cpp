/**
 *
 *  @file KeyEncoder_test.cpp
 *
 *  Vix KV unit tests
 *
 */

#include <cassert>
#include <string>

#include <softadastra/store/types/Key.hpp>

#include <vix/kv/keys/KeyEncoder.hpp>
#include <vix/kv/keys/KeyPath.hpp>

int main()
{
  using vix::kv::keys::KeyEncoder;
  using vix::kv::keys::KeyPath;

  {
    KeyPath key;
    const std::string encoded = KeyEncoder::encode_to_string(key);
    assert(encoded.empty());
  }

  {
    KeyPath key{"users"};
    const std::string encoded = KeyEncoder::encode_to_string(key);
    assert(encoded == "users");
  }

  {
    KeyPath key{"users", "42", "profile"};
    const std::string encoded = KeyEncoder::encode_to_string(key);
    assert(encoded == "users/42/profile");
  }

  {
    KeyPath key{"products", "abc", "images", "0"};
    const auto store_key = KeyEncoder::encode(key);
    assert(store_key.value == "products/abc/images/0");
  }

  {
    assert(KeyEncoder::separator() == '/');
  }

  return 0;
}
