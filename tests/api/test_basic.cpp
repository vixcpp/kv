/**
 *
 *  @file test_basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Basic public API tests
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
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
        "vix_kv_test_basic_api";

    fs::remove_all(root);
    fs::create_directories(root);

    return root;
  }

  bool test_direct_put_and_get()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    db.put("hello", "world");

    const auto value = db.get("hello");

    const bool ok =
        expect_true(
            value.has_value(),
            "get should return value after put") &&
        expect_eq<std::string>(
            value.value(),
            "world",
            "get should return stored string value") &&
        expect_true(
            db.is_open(),
            "database should remain open") &&
        expect_eq<std::size_t>(
            db.size(),
            1,
            "database size should be 1 after one put") &&
        expect_true(
            !db.empty(),
            "database should not be empty after one put");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_direct_get_missing_returns_nullopt()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    const auto value = db.get("missing");

    const bool ok =
        expect_true(
            !value.has_value(),
            "direct get should return nullopt for missing key") &&
        expect_eq<std::size_t>(
            db.size(),
            0,
            "missing read should not create a key");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_direct_put_replaces_value()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    db.put("hello", "old");
    db.put("hello", "new");

    const auto value = db.get("hello");

    const bool ok =
        expect_true(
            value.has_value(),
            "replaced key should still exist") &&
        expect_eq<std::string>(
            value.value(),
            "new",
            "second put should replace previous value") &&
        expect_eq<std::size_t>(
            db.size(),
            1,
            "replacing a key should keep size at 1");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_direct_slash_key_is_split_into_segments()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    db.put("users/1/name", "Ada");

    const auto direct_value = db.get("users/1/name");
    const auto structured_value = db.get({"users", "1", "name"});

    if (!expect_true(
            direct_value.has_value(),
            "slash key direct get should return value"))
    {
      (void)db.close();
      fs::remove_all(root);
      return false;
    }

    if (!expect_true(
            structured_value.is_ok(),
            "structured get should read value written with slash key"))
    {
      (void)db.close();
      fs::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            direct_value.value(),
            "Ada",
            "direct slash key value should match") &&
        expect_eq<std::string>(
            structured_value.value().to_string(),
            "Ada",
            "structured key value should match");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_set_with_initializer_list()
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
            "set initializer-list key should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto value = db.get({"users", "1", "name"});

    if (!expect_true(
            value.is_ok(),
            "get initializer-list key should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            value.value().to_string(),
            "Ada",
            "initializer-list get should return stored value") &&
        expect_true(
            db.contains({"users", "1", "name"}),
            "contains initializer-list should return true") &&
        expect_eq<std::size_t>(
            db.size(),
            1,
            "database size should be 1 after initializer-list set");

    (void)db.close();

    return ok;
  }

  bool test_set_with_key_path()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    const vix::kv::KeyPath key{"settings", "theme"};

    auto written = db.set(key, "dark");

    if (!expect_true(
            written.is_ok(),
            "set KeyPath should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto value = db.get(key);

    if (!expect_true(
            value.is_ok(),
            "get KeyPath should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            value.value().to_string(),
            "dark",
            "KeyPath get should return stored value") &&
        expect_true(
            db.contains(key),
            "contains KeyPath should return true");

    (void)db.close();

    return ok;
  }

  bool test_empty_value_is_allowed()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    auto written = db.set({"empty"}, "");

    if (!expect_true(
            written.is_ok(),
            "empty string value should be accepted"))
    {
      (void)db.close();
      return false;
    }

    auto value = db.get({"empty"});

    if (!expect_true(
            value.is_ok(),
            "empty value should be readable"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            value.value().to_string(),
            "",
            "empty value should remain empty") &&
        expect_true(
            db.contains({"empty"}),
            "empty value key should still exist");

    (void)db.close();

    return ok;
  }

  bool test_erase_existing_key()
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
            "erase existing key should succeed"))
    {
      (void)db.close();
      return false;
    }

    auto value = db.get({"hello"});

    const bool ok =
        expect_true(
            value.is_err(),
            "get erased key should fail") &&
        expect_error_code(
            value.error().code(),
            vix::kv::KvErrorCode::NotFound,
            "get erased key should return NotFound") &&
        expect_true(
            !db.contains({"hello"}),
            "contains should be false after erase") &&
        expect_eq<std::size_t>(
            db.size(),
            0,
            "size should be 0 after erasing only key");

    (void)db.close();

    return ok;
  }

  bool test_list_by_prefix()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    (void)db.set({"users", "1", "name"}, "Ada");
    (void)db.set({"users", "2", "name"}, "Grace");
    (void)db.set({"settings", "theme"}, "dark");

    auto users = db.list({"users"});

    if (!expect_true(
            users.is_ok(),
            "list users prefix should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            users.value().size(),
            2,
            "list users prefix should return 2 entries") &&
        expect_eq<std::string>(
            users.value()[0].first.at(0),
            "users",
            "first listed key should have users prefix") &&
        expect_eq<std::string>(
            users.value()[1].first.at(0),
            "users",
            "second listed key should have users prefix");

    (void)db.close();

    return ok;
  }

  bool test_list_empty_prefix_returns_all()
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
    (void)db.set({"c"}, "three");

    auto all = db.list();

    if (!expect_true(
            all.is_ok(),
            "list empty prefix should succeed"))
    {
      (void)db.close();
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            all.value().size(),
            3,
            "empty prefix should return all live entries");

    (void)db.close();

    return ok;
  }

  bool test_stats_after_basic_operations()
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

    const auto stats = db.stats();

    const bool ok =
        expect_true(
            stats.open,
            "stats should report open database") &&
        expect_true(
            stats.memory_only,
            "stats should report memory-only database") &&
        expect_eq<std::size_t>(
            stats.key_count,
            1,
            "stats key_count should report one live key") &&
        expect_true(
            stats.tombstone_count >= 1,
            "stats should report at least one tombstone") &&
        expect_true(
            stats.last_sequence >= 3,
            "stats should report latest sequence");

    (void)db.close();

    return ok;
  }

  bool test_operations_after_close_return_errors()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    auto closed = db.close();

    if (!expect_true(
            closed.is_ok(),
            "close should succeed"))
    {
      return false;
    }

    auto set_result = db.set({"hello"}, "world");
    auto get_result = db.get({"hello"});
    auto erase_result = db.erase(vix::kv::KeyPath{"hello"});
    auto list_result = db.list({"hello"});

    return expect_true(
               set_result.is_err(),
               "set after close should fail") &&
           expect_error_code(
               set_result.error().code(),
               vix::kv::KvErrorCode::NotOpen,
               "set after close should return NotOpen") &&
           expect_true(
               get_result.is_err(),
               "get after close should fail") &&
           expect_error_code(
               get_result.error().code(),
               vix::kv::KvErrorCode::NotOpen,
               "get after close should return NotOpen") &&
           expect_true(
               erase_result.is_err(),
               "erase after close should fail") &&
           expect_error_code(
               erase_result.error().code(),
               vix::kv::KvErrorCode::NotOpen,
               "erase after close should return NotOpen") &&
           expect_true(
               list_result.is_err(),
               "list after close should fail") &&
           expect_error_code(
               list_result.error().code(),
               vix::kv::KvErrorCode::NotOpen,
               "list after close should return NotOpen");
  }

  bool test_direct_put_after_close_throws()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    (void)db.close();

    bool thrown = false;

    try
    {
      db.put("hello", "world");
    }
    catch (const std::runtime_error &)
    {
      thrown = true;
    }

    fs::remove_all(root);

    return expect_true(
        thrown,
        "direct put after close should throw std::runtime_error");
  }
}

int main()
{
  if (!test_direct_put_and_get())
  {
    return 1;
  }

  if (!test_direct_get_missing_returns_nullopt())
  {
    return 1;
  }

  if (!test_direct_put_replaces_value())
  {
    return 1;
  }

  if (!test_direct_slash_key_is_split_into_segments())
  {
    return 1;
  }

  if (!test_set_with_initializer_list())
  {
    return 1;
  }

  if (!test_set_with_key_path())
  {
    return 1;
  }

  if (!test_empty_value_is_allowed())
  {
    return 1;
  }

  if (!test_erase_existing_key())
  {
    return 1;
  }

  if (!test_list_by_prefix())
  {
    return 1;
  }

  if (!test_list_empty_prefix_returns_all())
  {
    return 1;
  }

  if (!test_stats_after_basic_operations())
  {
    return 1;
  }

  if (!test_operations_after_close_return_errors())
  {
    return 1;
  }

  if (!test_direct_put_after_close_throws())
  {
    return 1;
  }

  std::cout << "kv_test_basic passed\n";
  return 0;
}
