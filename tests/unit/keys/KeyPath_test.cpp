/**
 *
 *  @file KeyPath_test.cpp
 *
 *  Vix KV unit tests
 *
 */

#include <cassert>
#include <string>
#include <vector>

#include <vix/kv/keys/KeyPath.hpp>

int main()
{
  using vix::kv::keys::KeyPath;

  {
    KeyPath key;
    assert(key.empty());
    assert(key.size() == 0);
  }

  {
    KeyPath key{"users", "42", "profile"};
    assert(!key.empty());
    assert(key.size() == 3);
    assert(key.at(0) == "users");
    assert(key.at(1) == "42");
    assert(key.at(2) == "profile");
    assert(key.front() == "users");
    assert(key.back() == "profile");
  }

  {
    KeyPath key;
    key.push_back("users").push_back("1").push_back("settings");

    assert(key.size() == 3);
    assert(key.at(0) == "users");
    assert(key.at(1) == "1");
    assert(key.at(2) == "settings");
  }

  {
    KeyPath a{"users", "1"};
    KeyPath b{"users", "1"};
    KeyPath c{"users", "2"};

    assert(a == b);
    assert(a != c);
  }

  {
    KeyPath a{"a"};
    KeyPath b{"b"};
    assert(a < b);
  }

  {
    KeyPath key{"users", "42"};
    const auto &segments = key.segments();

    assert(segments.size() == 2);
    assert(segments[0] == "users");
    assert(segments[1] == "42");
  }

  {
    KeyPath key{"users", "42"};
    key.clear();
    assert(key.empty());
    assert(key.size() == 0);
  }

  return 0;
}
