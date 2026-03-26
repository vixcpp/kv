/**
 *
 *  @file list_prefix_test.cpp
 *
 *  Vix KV integration tests
 *
 */

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace fs = std::filesystem;

static void test_list_prefix()
{
  const std::string wal_path = "testdata/list_prefix.wal";

  fs::create_directories("testdata");
  fs::remove(wal_path);

  vix::kv::api::KvOptions options;
  options.path = wal_path;

  auto db = vix::kv::open(options);

  assert(db.set({"users", "1"}, vix::kv::values::KvValue("Alice")));
  assert(db.set({"users", "2"}, vix::kv::values::KvValue("Bob")));
  assert(db.set({"orders", "1"}, vix::kv::values::KvValue("Order-A")));

  auto users = db.list({"users"});
  assert(users.size() == 2);

  bool found_alice = false;
  bool found_bob = false;

  for (const auto &[key, value] : users)
  {
    if (key.size() == 2 && key.at(0) == "users" && key.at(1) == "1" &&
        value.to_string() == "Alice")
    {
      found_alice = true;
    }

    if (key.size() == 2 && key.at(0) == "users" && key.at(1) == "2" &&
        value.to_string() == "Bob")
    {
      found_bob = true;
    }
  }

  assert(found_alice);
  assert(found_bob);

  fs::remove(wal_path);
}

int main()
{
  test_list_prefix();
  return 0;
}
