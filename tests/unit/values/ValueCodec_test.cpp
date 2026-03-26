/**
 *
 *  @file ValueCodec_test.cpp
 *
 *  Vix KV unit tests
 *
 */

#include <cassert>
#include <cstdint>
#include <vector>

#include <softadastra/store/types/Value.hpp>

#include <vix/kv/values/KvValue.hpp>
#include <vix/kv/values/ValueCodec.hpp>

int main()
{
  using vix::kv::values::KvValue;
  using vix::kv::values::ValueCodec;

  {
    KvValue value("Alice");
    const auto encoded = ValueCodec::encode(value);

    assert(encoded.data.size() == 5);
    assert(encoded.data[0] == static_cast<std::uint8_t>('A'));
    assert(encoded.data[1] == static_cast<std::uint8_t>('l'));
    assert(encoded.data[2] == static_cast<std::uint8_t>('i'));
    assert(encoded.data[3] == static_cast<std::uint8_t>('c'));
    assert(encoded.data[4] == static_cast<std::uint8_t>('e'));
  }

  {
    softadastra::store::types::Value store_value;
    store_value.data = {
        static_cast<std::uint8_t>('B'),
        static_cast<std::uint8_t>('o'),
        static_cast<std::uint8_t>('b')};

    const auto decoded = ValueCodec::decode(store_value);

    assert(decoded.size() == 3);
    assert(decoded.to_string() == "Bob");
  }

  {
    KvValue original("Charlie");
    const auto encoded = ValueCodec::encode(original);
    const auto decoded = ValueCodec::decode(encoded);

    assert(decoded == original);
    assert(decoded.to_string() == "Charlie");
  }

  {
    KvValue::Bytes bytes{1, 2, 3, 255};
    KvValue original(bytes);

    const auto encoded = ValueCodec::encode(original);
    const auto decoded = ValueCodec::decode(encoded);

    assert(decoded.size() == 4);
    assert(decoded.bytes()[0] == 1);
    assert(decoded.bytes()[1] == 2);
    assert(decoded.bytes()[2] == 3);
    assert(decoded.bytes()[3] == 255);
  }

  return 0;
}
