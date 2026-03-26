/**
 *
 *  @file open_close_test.cpp
 *
 *  Vix KV integration tests
 *
 */

#include <cassert>
#include <filesystem>
#include <string>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/api/KvOptions.hpp>

namespace fs = std::filesystem;

static void test_open_close()
{
  const std::string wal_path = "testdata/open_close.wal";

  fs::create_directories("testdata");
  fs::remove(wal_path);

  vix::kv::api::KvOptions options;
  options.path = wal_path;

  auto db = vix::kv::open(options);

  auto value = db.get({"missing"});
  assert(!value.has_value());

  fs::remove(wal_path);
}

int main()
{
  test_open_close();
  return 0;
}
