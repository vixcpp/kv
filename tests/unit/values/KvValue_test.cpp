/**
 *
 *  @file KvValue_test.cpp
 *
 *  Vix KV unit tests
 *
 */

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <vix/kv/values/KvValue.hpp>

int main()
{
  using vix::kv::values::KvValue;

  {
    KvValue value;
    assert(value.empty());
    assert(value.size() == 0);
  }

  {
    KvValue value("Alice");
    assert(!value.empty());
    assert(value.size() == 5);
    assert(value.to_string() == "Alice");
  }

  {
    KvValue::Bytes bytes{1, 2, 3, 4};
    KvValue value(bytes);

    assert(value.size() == 4);
    assert(value.bytes().size() == 4);
    assert(value.bytes()[0] == 1);
    assert(value.bytes()[1] == 2);
    assert(value.bytes()[2] == 3);
    assert(value.bytes()[3] == 4);
  }

  {
    auto value = KvValue::from_string("Bob");
    assert(value.to_string() == "Bob");
    assert(value.size() == 3);
  }

  {
    KvValue::Bytes bytes{9, 8, 7};
    auto value = KvValue::from_bytes(bytes);

    assert(value.size() == 3);
    assert(value.bytes()[0] == 9);
    assert(value.bytes()[1] == 8);
    assert(value.bytes()[2] == 7);
  }

  {
    KvValue a("hello");
    KvValue b("hello");
    KvValue c("world");

    assert(a == b);
    assert(a != c);
  }

  {
    KvValue value("test");
    value.clear();
    assert(value.empty());
    assert(value.size() == 0);
  }

  return 0;
}
