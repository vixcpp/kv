/**
 *
 *  @file erase_test.cpp
 *
 *  Vix KV integration tests
 *
 */

#include <cassert>
#include <filesystem>
#include <string>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace fs = std::filesystem;

static void test_erase()
{
  const std::string wal_path = "testdata/erase.wal";

  fs::create_directories("testdata");
  fs::remove(wal_path);

  vix::kv::api::KvOptions options;
  options.path = wal_path;

  auto db = vix::kv::open(options);

  assert(db.set({"users", "2"}, vix::kv::values::KvValue("Bob")));

  auto before = db.get({"users", "2"});
  assert(before.has_value());
  assert(before->to_string() == "Bob");

  const bool erased = db.erase({"users", "2"});
  assert(erased);

  auto after = db.get({"users", "2"});
  assert(!after.has_value());

  fs::remove(wal_path);
}

int main()
{
  test_erase();
  return 0;
}
