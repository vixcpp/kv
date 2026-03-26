/**
 *
 *  @file recovery_test.cpp
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

static void test_recovery()
{
  const std::string wal_path = "testdata/recovery.wal";

  fs::create_directories("testdata");
  fs::remove(wal_path);

  vix::kv::api::KvOptions options;
  options.path = wal_path;

  {
    auto db = vix::kv::open(options);

    assert(db.set({"session", "token"}, vix::kv::values::KvValue("abc123")));
    assert(db.set({"users", "42"}, vix::kv::values::KvValue("Gaspard")));
  }

  {
    auto db = vix::kv::open(options);

    auto token = db.get({"session", "token"});
    assert(token.has_value());
    assert(token->to_string() == "abc123");

    auto user = db.get({"users", "42"});
    assert(user.has_value());
    assert(user->to_string() == "Gaspard");
  }

  fs::remove(wal_path);
}

int main()
{
  test_recovery();
  return 0;
}
