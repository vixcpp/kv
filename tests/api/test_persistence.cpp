/**
 *
 *  @file test_persistence.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public persistence API tests
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

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
        "vix_kv_test_persistence_api";

    fs::remove_all(root);
    fs::create_directories(root);

    return root;
  }

  bool test_durable_put_survives_reopen()
  {
    const auto root = make_test_root();

    {
      auto db = vix::kv::open(root);

      db.put("hello", "world");

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed before reopening"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto db = vix::kv::open(root);

      const auto value = db.get("hello");

      const bool ok =
          expect_true(
              value.has_value(),
              "durable value should exist after reopen") &&
          expect_eq<std::string>(
              value.value(),
              "world",
              "durable value should match after reopen") &&
          expect_eq<std::size_t>(
              db.size(),
              1,
              "reopened durable database should have one live key");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_durable_initializer_list_key_survives_reopen()
  {
    const auto root = make_test_root();

    {
      auto opened = vix::kv::open_durable(root);

      if (!expect_true(
              opened.is_ok(),
              "open_durable should succeed"))
      {
        fs::remove_all(root);
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
        fs::remove_all(root);
        return false;
      }

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed after initializer-list set"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto opened = vix::kv::open_durable(root);

      if (!expect_true(
              opened.is_ok(),
              "reopen durable database should succeed"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      auto value = db.get({"users", "1", "name"});

      if (!expect_true(
              value.is_ok(),
              "initializer-list key should recover after reopen"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      const bool ok =
          expect_eq<std::string>(
              value.value().to_string(),
              "Ada",
              "recovered initializer-list value should match") &&
          expect_true(
              db.contains({"users", "1", "name"}),
              "contains should find recovered initializer-list key") &&
          expect_eq<std::size_t>(
              db.size(),
              1,
              "reopened database should have one live key");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_multiple_values_survive_reopen()
  {
    const auto root = make_test_root();

    {
      auto db = vix::kv::open(root);

      db.put("users/1/name", "Ada");
      db.put("users/2/name", "Grace");
      db.put("settings/theme", "dark");

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed after multiple writes"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto db = vix::kv::open(root);

      const auto user_1 = db.get("users/1/name");
      const auto user_2 = db.get("users/2/name");
      const auto theme = db.get("settings/theme");

      const bool ok =
          expect_true(
              user_1.has_value(),
              "first value should recover") &&
          expect_true(
              user_2.has_value(),
              "second value should recover") &&
          expect_true(
              theme.has_value(),
              "third value should recover") &&
          expect_eq<std::string>(
              user_1.value(),
              "Ada",
              "first recovered value should match") &&
          expect_eq<std::string>(
              user_2.value(),
              "Grace",
              "second recovered value should match") &&
          expect_eq<std::string>(
              theme.value(),
              "dark",
              "third recovered value should match") &&
          expect_eq<std::size_t>(
              db.size(),
              3,
              "reopened database should recover 3 live keys");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_replaced_value_survives_reopen()
  {
    const auto root = make_test_root();

    {
      auto db = vix::kv::open(root);

      db.put("hello", "old");
      db.put("hello", "new");

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed after replacement"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto db = vix::kv::open(root);

      const auto value = db.get("hello");

      const bool ok =
          expect_true(
              value.has_value(),
              "replaced key should recover") &&
          expect_eq<std::string>(
              value.value(),
              "new",
              "latest value should survive reopen") &&
          expect_eq<std::size_t>(
              db.size(),
              1,
              "replacement should recover as one live key");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_delete_survives_reopen()
  {
    const auto root = make_test_root();

    {
      auto opened = vix::kv::open_durable(root);

      if (!expect_true(
              opened.is_ok(),
              "open_durable should succeed"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      (void)db.set({"hello"}, "world");

      auto erased = db.erase(vix::kv::KeyPath{"hello"});

      if (!expect_true(
              erased.is_ok(),
              "erase should succeed before reopen"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed after erase"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto opened = vix::kv::open_durable(root);

      if (!expect_true(
              opened.is_ok(),
              "reopen durable database should succeed"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      auto value = db.get({"hello"});

      const bool ok =
          expect_true(
              value.is_err(),
              "deleted key should not recover as live value") &&
          expect_error_code(
              value.error().code(),
              vix::kv::KvErrorCode::NotFound,
              "deleted key should return NotFound after reopen") &&
          expect_true(
              !db.contains({"hello"}),
              "contains should be false for deleted recovered key") &&
          expect_eq<std::size_t>(
              db.size(),
              0,
              "reopened database should have no live keys after delete");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_put_after_delete_survives_reopen()
  {
    const auto root = make_test_root();

    {
      auto db = vix::kv::open(root);

      db.put("hello", "old");

      auto erased = db.erase(vix::kv::KeyPath{"hello"});

      if (!expect_true(
              erased.is_ok(),
              "erase should succeed before restore"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      db.put("hello", "new");

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed after put after delete"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto db = vix::kv::open(root);

      const auto value = db.get("hello");

      const bool ok =
          expect_true(
              value.has_value(),
              "put after delete should recover as live key") &&
          expect_eq<std::string>(
              value.value(),
              "new",
              "put after delete should recover latest value") &&
          expect_eq<std::size_t>(
              db.size(),
              1,
              "put after delete should recover one live key");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_memory_database_does_not_persist()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed"))
    {
      return false;
    }

    {
      auto db = opened.move_value();

      db.put("hello", "world");

      const auto value = db.get("hello");

      if (!expect_true(
              value.has_value(),
              "memory database should store value while open"))
      {
        (void)db.close();
        return false;
      }

      (void)db.close();
    }

    auto reopened = vix::kv::open_memory();

    if (!expect_true(
            reopened.is_ok(),
            "second open_memory should succeed"))
    {
      return false;
    }

    auto db = reopened.move_value();

    const auto value = db.get("hello");

    const bool ok =
        expect_true(
            !value.has_value(),
            "new memory database should not persist previous value") &&
        expect_eq<std::size_t>(
            db.size(),
            0,
            "new memory database should start empty");

    (void)db.close();

    return ok;
  }

  bool test_fast_database_persists_after_manual_flush()
  {
    const auto root = make_test_root();

    {
      auto opened = vix::kv::open_fast(root);

      if (!expect_true(
              opened.is_ok(),
              "open_fast should succeed"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      auto written = db.set({"hello"}, "world");

      if (!expect_true(
              written.is_ok(),
              "open_fast set should succeed"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "manual flush should persist open_fast data"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto opened = vix::kv::open_fast(root);

      if (!expect_true(
              opened.is_ok(),
              "reopen fast database should succeed"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      auto value = db.get({"hello"});

      if (!expect_true(
              value.is_ok(),
              "manually flushed fast database should recover value"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      const bool ok =
          expect_eq<std::string>(
              value.value().to_string(),
              "world",
              "open_fast recovered value should match") &&
          expect_eq<std::size_t>(
              db.size(),
              1,
              "open_fast recovered database should have one live key");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_stats_report_recovery_after_reopen()
  {
    const auto root = make_test_root();

    {
      auto db = vix::kv::open(root);

      db.put("a", "one");
      db.put("b", "two");

      auto flushed = db.flush();

      if (!expect_true(
              flushed.is_ok(),
              "flush should succeed before stats recovery test"))
      {
        (void)db.close();
        fs::remove_all(root);
        return false;
      }

      (void)db.close();
    }

    {
      auto db = vix::kv::open(root);

      const auto stats = db.stats();

      const bool ok =
          expect_true(
              stats.wal_enabled,
              "reopened durable stats should report WAL enabled") &&
          expect_true(
              stats.wal_records_recovered >= 2,
              "reopened durable stats should report recovered WAL records") &&
          expect_true(
              stats.last_recovered_sequence >= 2,
              "reopened durable stats should report recovered sequence") &&
          expect_eq<std::size_t>(
              stats.key_count,
              2,
              "reopened durable stats should report recovered key count");

      (void)db.close();
      fs::remove_all(root);

      return ok;
    }
  }

  bool test_persistence_directory_is_created()
  {
    const auto root =
        fs::temp_directory_path() /
        "vix_kv_test_persistence_api_created";

    fs::remove_all(root);

    {
      auto opened = vix::kv::open_durable(root);

      if (!expect_true(
              opened.is_ok(),
              "open_durable should create missing root directory"))
      {
        fs::remove_all(root);
        return false;
      }

      auto db = opened.move_value();

      const bool ok =
          expect_true(
              fs::exists(root),
              "root directory should exist after open_durable") &&
          expect_true(
              db.is_open(),
              "database should be open after creating directory");

      (void)db.close();

      if (!ok)
      {
        fs::remove_all(root);
        return false;
      }
    }

    fs::remove_all(root);

    return true;
  }
}

int main()
{
  if (!test_durable_put_survives_reopen())
  {
    return 1;
  }

  if (!test_durable_initializer_list_key_survives_reopen())
  {
    return 1;
  }

  if (!test_multiple_values_survive_reopen())
  {
    return 1;
  }

  if (!test_replaced_value_survives_reopen())
  {
    return 1;
  }

  if (!test_delete_survives_reopen())
  {
    return 1;
  }

  if (!test_put_after_delete_survives_reopen())
  {
    return 1;
  }

  if (!test_memory_database_does_not_persist())
  {
    return 1;
  }

  if (!test_fast_database_persists_after_manual_flush())
  {
    return 1;
  }

  if (!test_stats_report_recovery_after_reopen())
  {
    return 1;
  }

  if (!test_persistence_directory_is_created())
  {
    return 1;
  }

  std::cout << "kv_test_persistence passed\n";
  return 0;
}
