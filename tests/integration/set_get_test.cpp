/**
 *
 *  @file set_get_test.cpp
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

static void test_set_get()
{
  const std::string wal_path = "testdata/set_get.wal";

  fs::create_directories("testdata");
  fs::remove(wal_path);

  vix::kv::api::KvOptions options;
  options.path = wal_path;

  auto db = vix::kv::open(options);

  const bool ok = db.set({"users", "1"}, vix::kv::values::KvValue("Alice"));
  assert(ok);

  auto value = db.get({"users", "1"});
  assert(value.has_value());
  assert(value->to_string() == "Alice");

  fs::remove(wal_path);
}

int main()
{
  test_set_get();
  return 0;
}
