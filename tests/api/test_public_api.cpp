/**
 *
 *  @file test_public_api.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public API surface tests
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
  namespace fs = std::filesystem;

  bool expect_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      return false;
    }

    return true;
  }

  template <typename T>
  bool expect_eq(
      const T &actual,
      const T &expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: " << expected << '\n';
      std::cerr << "  actual  : " << actual << '\n';
      return false;
    }

    return true;
  }

  bool expect_error_code(
      vix::kv::KvErrorCode actual,
      vix::kv::KvErrorCode expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: "
                << vix::kv::core::to_string(expected)
                << '\n';
      std::cerr << "  actual  : "
                << vix::kv::core::to_string(actual)
                << '\n';
      return false;
    }

    return true;
  }

  fs::path make_test_root()
  {
    auto root =
        fs::temp_directory_path() /
        "vix_kv_test_public_api";

    fs::remove_all(root);
    fs::create_directories(root);

    return root;
  }

  bool test_public_type_aliases_are_available()
  {
    const bool kv_alias_ok =
        std::is_same_v<vix::kv::Kv, vix::kv::api::Kv>;

    const bool options_alias_ok =
        std::is_same_v<vix::kv::KvOptions, vix::kv::api::KvOptions>;

    const bool value_alias_ok =
        std::is_same_v<vix::kv::KvValue, vix::kv::values::KvValue>;

    const bool key_alias_ok =
        std::is_same_v<vix::kv::KeyPath, vix::kv::keys::KeyPath>;

    const bool error_alias_ok =
        std::is_same_v<vix::kv::KvError, vix::kv::core::KvError>;

    const bool error_code_alias_ok =
        std::is_same_v<vix::kv::KvErrorCode, vix::kv::core::KvErrorCode>;

    const bool stats_alias_ok =
        std::is_same_v<vix::kv::KvStats, vix::kv::core::KvStats>;

    return expect_true(
               kv_alias_ok,
               "public Kv alias should map to api::Kv") &&
           expect_true(
               options_alias_ok,
               "public KvOptions alias should map to api::KvOptions") &&
           expect_true(
               value_alias_ok,
               "public KvValue alias should map to values::KvValue") &&
           expect_true(
               key_alias_ok,
               "public KeyPath alias should map to keys::KeyPath") &&
           expect_true(
               error_alias_ok,
               "public KvError alias should map to core::KvError") &&
           expect_true(
               error_code_alias_ok,
               "public KvErrorCode alias should map to core::KvErrorCode") &&
           expect_true(
               stats_alias_ok,
               "public KvStats alias should map to core::KvStats");
  }

  bool test_public_key_path_alias()
  {
    vix::kv::KeyPath key{"users", "1", "name"};

    return expect_eq<std::size_t>(
               key.size(),
               3,
               "public KeyPath should accept string literal segments") &&
           expect_eq<std::string>(
               key.at(0),
               "users",
               "public KeyPath first segment should match") &&
           expect_eq<std::string>(
               key.at(1),
               "1",
               "public KeyPath second segment should match") &&
           expect_eq<std::string>(
               key.at(2),
               "name",
               "public KeyPath third segment should match");
  }

  bool test_public_value_alias()
  {
    auto value = vix::kv::KvValue::from_string("hello");

    return expect_eq<std::string>(
               value.to_string(),
               "hello",
               "public KvValue should convert from string") &&
           expect_eq<std::size_t>(
               value.size(),
               5,
               "public KvValue size should match string size") &&
           expect_true(
               !value.empty(),
               "public KvValue should not be empty");
  }

  bool test_public_options_memory_only()
  {
    auto options = vix::kv::KvOptions::memory_only();

    return expect_true(
               options.memory_only,
               "KvOptions::memory_only should enable memory_only") &&
           expect_true(
               !options.auto_flush,
               "memory-only options should not need auto_flush") &&
           expect_true(
               options.path.empty(),
               "memory-only options should not require a path") &&
           expect_true(
               options.validate().is_ok(),
               "memory-only options should validate");
  }

  bool test_public_options_durable()
  {
    const auto root = make_test_root();

    auto options = vix::kv::KvOptions::durable(root);

    const bool ok =
        expect_true(
            !options.memory_only,
            "durable options should not be memory_only") &&
        expect_true(
            options.auto_flush,
            "durable options should enable auto_flush by default") &&
        expect_eq<fs::path>(
            options.path,
            root,
            "durable options should preserve path") &&
        expect_true(
            options.validate().is_ok(),
            "durable options should validate");

    fs::remove_all(root);

    return ok;
  }

  bool test_public_options_fast()
  {
    const auto root = make_test_root();

    auto options = vix::kv::KvOptions::fast(root);

    const bool ok =
        expect_true(
            !options.memory_only,
            "fast options should not be memory_only") &&
        expect_true(
            !options.auto_flush,
            "fast options should disable auto_flush") &&
        expect_eq<fs::path>(
            options.path,
            root,
            "fast options should preserve path") &&
        expect_true(
            options.validate().is_ok(),
            "fast options should validate");

    fs::remove_all(root);

    return ok;
  }

  bool test_public_open_memory()
  {
    vix::kv::KvResult<vix::kv::Kv> opened =
        vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should return KvResult<Kv> success"))
    {
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "public memory KV should be open") &&
        expect_true(
            db.options().memory_only,
            "public memory KV should expose memory_only options") &&
        expect_true(
            db.stats().memory_only,
            "public memory KV stats should report memory_only") &&
        expect_true(
            !db.stats().wal_enabled,
            "public memory KV stats should report WAL disabled");

    (void)db.close();

    return ok;
  }

  bool test_public_open_with_options()
  {
    const auto root = make_test_root();

    auto options = vix::kv::KvOptions::durable(root);
    options.auto_flush = false;

    auto opened = vix::kv::open(options);

    if (!expect_true(
            opened.is_ok(),
            "open(options) should return success"))
    {
      fs::remove_all(root);
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "public KV opened with options should be open") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "public KV should preserve options path") &&
        expect_true(
            !db.options().auto_flush,
            "public KV should preserve options auto_flush") &&
        expect_true(
            db.stats().wal_enabled,
            "public KV stats should report WAL enabled for durable options");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_public_direct_open_path()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    const bool ok =
        expect_true(
            db.is_open(),
            "direct open(path) should return an open Kv handle") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "direct open(path) should preserve path") &&
        expect_true(
            db.stats().wal_enabled,
            "direct open(path) should enable WAL");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_public_simple_put_get_api()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    db.put("hello", "world");

    std::optional<std::string> value = db.get("hello");

    const bool ok =
        expect_true(
            value.has_value(),
            "simple get should return optional value") &&
        expect_eq<std::string>(
            value.value(),
            "world",
            "simple get should return stored value") &&
        expect_eq<std::size_t>(
            db.size(),
            1,
            "public KV size should be 1 after simple put");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_public_simple_get_missing()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    auto value = db.get("missing");

    const bool ok =
        expect_true(
            !value.has_value(),
            "simple get should return nullopt for missing key") &&
        expect_true(
            db.empty(),
            "missing simple get should not create entries");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_public_initializer_list_api()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    auto written = db.set(
        {"users", "1", "name"},
        "Ada");

    if (!expect_true(
            written.is_ok(),
            "initializer-list set should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto read = db.get({"users", "1", "name"});

    if (!expect_true(
            read.is_ok(),
            "initializer-list get should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto listed = db.list({"users"});

    if (!expect_true(
            listed.is_ok(),
            "initializer-list list should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            read.value().to_string(),
            "Ada",
            "initializer-list get should return stored value") &&
        expect_true(
            db.contains({"users", "1", "name"}),
            "initializer-list contains should return true") &&
        expect_eq<std::size_t>(
            listed.value().size(),
            1,
            "initializer-list list should return one user entry");

    (void)db.close();

    return ok;
  }

  bool test_public_key_path_api()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    vix::kv::KeyPath key{"settings", "theme"};

    auto written = db.set(key, "dark");

    if (!expect_true(
            written.is_ok(),
            "KeyPath set should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto read = db.get(key);

    if (!expect_true(
            read.is_ok(),
            "KeyPath get should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            read.value().to_string(),
            "dark",
            "KeyPath get should return stored value") &&
        expect_true(
            db.contains(key),
            "KeyPath contains should return true");

    (void)db.close();

    return ok;
  }

  bool test_public_list_default_prefix()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    (void)db.set({"a"}, "one");
    (void)db.set({"b"}, "two");

    auto listed = db.list();

    if (!expect_true(
            listed.is_ok(),
            "list() should succeed with default empty prefix"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            listed.value().size(),
            2,
            "list() should return all live entries");

    (void)db.close();

    return ok;
  }

  bool test_public_erase_api()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    (void)db.set({"hello"}, "world");

    auto erased = db.erase(vix::kv::KeyPath{"hello"});

    if (!expect_true(
            erased.is_ok(),
            "public erase should succeed for existing key"))
    {
      (void)db.close();
      return false;
    }

    auto read = db.get({"hello"});

    const bool ok =
        expect_true(
            read.is_err(),
            "erased key should not be readable") &&
        expect_error_code(
            read.error().code(),
            vix::kv::KvErrorCode::NotFound,
            "erased key should return NotFound") &&
        expect_true(
            !db.contains({"hello"}),
            "contains should return false after erase");

    (void)db.close();

    return ok;
  }

  bool test_public_stats_api()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    (void)db.set({"a"}, "one");
    (void)db.set({"b"}, "two");
    (void)db.erase(vix::kv::KeyPath{"a"});

    vix::kv::KvStats stats = db.stats();

    const bool ok =
        expect_true(
            stats.open,
            "public stats should report open handle") &&
        expect_true(
            stats.memory_only,
            "public stats should report memory_only") &&
        expect_eq<std::size_t>(
            stats.key_count,
            1,
            "public stats key_count should report one live key") &&
        expect_true(
            stats.tombstone_count >= 1,
            "public stats should report tombstones") &&
        expect_true(
            stats.last_sequence >= 3,
            "public stats should report latest sequence") &&
        expect_true(
            stats.write_count() >= 3,
            "public stats write_count should include set and erase");

    (void)db.close();

    return ok;
  }

  bool test_public_error_code_alias()
  {
    const auto code = vix::kv::KvErrorCode::NotFound;

    return expect_eq<std::string>(
        std::string(vix::kv::core::to_string(code)),
        "not_found",
        "public KvErrorCode should work with core::to_string");
  }

  bool test_public_result_alias()
  {
    vix::kv::KvResult<int> result =
        vix::kv::KvResult<int>::ok(42);

    if (!expect_true(
            result.is_ok(),
            "public KvResult alias should expose ok result"))
    {
      return false;
    }

    return expect_eq<int>(
        result.value(),
        42,
        "public KvResult alias should store value");
  }
}

int main()
{
  if (!test_public_type_aliases_are_available())
  {
    return 1;
  }

  if (!test_public_key_path_alias())
  {
    return 1;
  }

  if (!test_public_value_alias())
  {
    return 1;
  }

  if (!test_public_options_memory_only())
  {
    return 1;
  }

  if (!test_public_options_durable())
  {
    return 1;
  }

  if (!test_public_options_fast())
  {
    return 1;
  }

  if (!test_public_open_memory())
  {
    return 1;
  }

  if (!test_public_open_with_options())
  {
    return 1;
  }

  if (!test_public_direct_open_path())
  {
    return 1;
  }

  if (!test_public_simple_put_get_api())
  {
    return 1;
  }

  if (!test_public_simple_get_missing())
  {
    return 1;
  }

  if (!test_public_initializer_list_api())
  {
    return 1;
  }

  if (!test_public_key_path_api())
  {
    return 1;
  }

  if (!test_public_list_default_prefix())
  {
    return 1;
  }

  if (!test_public_erase_api())
  {
    return 1;
  }

  if (!test_public_stats_api())
  {
    return 1;
  }

  if (!test_public_error_code_alias())
  {
    return 1;
  }

  if (!test_public_result_alias())
  {
    return 1;
  }

  std::cout << "kv_test_public_api passed\n";
  return 0;
}
