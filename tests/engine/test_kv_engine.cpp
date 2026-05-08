/**
 *
 *  @file test_kv_engine.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KvEngine unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/internal/KvEngine.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/values/KvValue.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace internal = vix::kv::internal;
  namespace keys = vix::kv::keys;
  namespace values = vix::kv::values;

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
      core::KvErrorCode actual,
      core::KvErrorCode expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: "
                << core::to_string(expected)
                << '\n';
      std::cerr << "  actual  : "
                << core::to_string(actual)
                << '\n';
      return false;
    }

    return true;
  }

  std::filesystem::path make_test_root()
  {
    auto root =
        std::filesystem::temp_directory_path() /
        "vix_kv_test_engine";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    return root;
  }

  keys::KeyPath key(std::initializer_list<std::string_view> parts)
  {
    return keys::KeyPath{parts};
  }

  values::KvValue value(std::string text)
  {
    return values::KvValue::from_string(text);
  }

  bool test_memory_engine_default_state()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    return expect_true(
               !engine.is_open(),
               "new engine should be closed") &&
           expect_true(
               engine.empty(),
               "new engine should be empty") &&
           expect_eq<std::size_t>(
               engine.size(),
               0,
               "new engine size should be 0");
  }

  bool test_open_memory_engine()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    auto opened = engine.open();

    const bool ok =
        expect_true(
            opened.is_ok(),
            "memory engine open should succeed") &&
        expect_true(
            engine.is_open(),
            "engine should be open after open()") &&
        expect_true(
            engine.empty(),
            "opened memory engine should be empty") &&
        expect_eq<std::size_t>(
            engine.size(),
            0,
            "opened memory engine size should be 0");

    (void)engine.close();

    return ok;
  }

  bool test_open_twice_is_rejected()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    auto first = engine.open();
    auto second = engine.open();

    const bool ok =
        expect_true(
            first.is_ok(),
            "first engine open should succeed") &&
        expect_true(
            second.is_err(),
            "second engine open should fail") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::AlreadyOpen,
            "opening an already open engine should return AlreadyOpen");

    (void)engine.close();

    return ok;
  }

  bool test_close_without_open_is_ok()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    auto closed = engine.close();

    return expect_true(
        closed.is_ok(),
        "closing a closed engine should be a no-op success");
  }

  bool test_set_and_get_memory()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    auto opened = engine.open();

    if (!expect_true(opened.is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto written = engine.set(
        key({"users", "1", "name"}),
        value("Ada"));

    if (!expect_true(written.is_ok(), "engine set should succeed"))
    {
      (void)engine.close();
      return false;
    }

    auto read = engine.get(key({"users", "1", "name"}));

    if (!expect_true(read.is_ok(), "engine get should succeed"))
    {
      (void)engine.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            read.value().to_string(),
            "Ada",
            "engine get should return stored value") &&
        expect_true(
            engine.contains(key({"users", "1", "name"})),
            "engine contains should find stored key") &&
        expect_eq<std::size_t>(
            engine.size(),
            1,
            "engine size should be 1 after set") &&
        expect_true(
            !engine.empty(),
            "engine should not be empty after set");

    (void)engine.close();

    return ok;
  }

  bool test_set_replaces_existing_value()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto first = engine.set(
        key({"users", "1", "name"}),
        value("Ada"));

    auto second = engine.set(
        key({"users", "1", "name"}),
        value("Grace"));

    if (!expect_true(
            first.is_ok() && second.is_ok(),
            "both set calls should succeed"))
    {
      (void)engine.close();
      return false;
    }

    auto read = engine.get(key({"users", "1", "name"}));

    if (!expect_true(read.is_ok(), "engine get should succeed"))
    {
      (void)engine.close();
      return false;
    }

    const bool ok =
        expect_eq<std::string>(
            read.value().to_string(),
            "Grace",
            "newer set should replace previous value") &&
        expect_eq<std::size_t>(
            engine.size(),
            1,
            "replace should keep size at 1");

    (void)engine.close();

    return ok;
  }

  bool test_get_missing_key_returns_not_found()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto result = engine.get(key({"missing"}));

    const bool ok =
        expect_true(
            result.is_err(),
            "get missing key should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotFound,
            "get missing key should return NotFound");

    (void)engine.close();

    return ok;
  }

  bool test_erase_existing_key()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto written = engine.set(
        key({"users", "1", "name"}),
        value("Ada"));

    auto erased = engine.erase(
        key({"users", "1", "name"}));

    if (!expect_true(
            written.is_ok() && erased.is_ok(),
            "set and erase should succeed"))
    {
      (void)engine.close();
      return false;
    }

    auto read = engine.get(key({"users", "1", "name"}));

    const bool ok =
        expect_true(
            read.is_err(),
            "get erased key should fail") &&
        expect_error_code(
            read.error().code(),
            core::KvErrorCode::NotFound,
            "get erased key should return NotFound") &&
        expect_true(
            !engine.contains(key({"users", "1", "name"})),
            "contains should be false after erase") &&
        expect_eq<std::size_t>(
            engine.size(),
            0,
            "engine live size should be 0 after erase");

    (void)engine.close();

    return ok;
  }

  bool test_erase_missing_key_creates_no_live_value()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto erased = engine.erase(key({"missing"}));

    const bool ok =
        expect_true(
            erased.is_ok(),
            "erase missing key should succeed as tombstone write") &&
        expect_true(
            !engine.contains(key({"missing"})),
            "contains should be false after erasing missing key") &&
        expect_eq<std::size_t>(
            engine.size(),
            0,
            "live size should remain 0 after erasing missing key");

    (void)engine.close();

    return ok;
  }

  bool test_list_prefix()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    (void)engine.set(key({"users", "1", "name"}), value("Ada"));
    (void)engine.set(key({"users", "2", "name"}), value("Grace"));
    (void)engine.set(key({"settings", "theme"}), value("dark"));

    auto listed = engine.list(key({"users"}));

    if (!expect_true(listed.is_ok(), "engine list should succeed"))
    {
      (void)engine.close();
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            listed.value().size(),
            2,
            "list users prefix should return 2 values") &&
        expect_eq<std::string>(
            listed.value()[0].first.at(0),
            "users",
            "first listed key should keep decoded prefix") &&
        expect_eq<std::string>(
            listed.value()[1].first.at(0),
            "users",
            "second listed key should keep decoded prefix");

    (void)engine.close();

    return ok;
  }

  bool test_list_empty_prefix_returns_all_live_values()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    (void)engine.set(key({"b"}), value("two"));
    (void)engine.set(key({"a"}), value("one"));

    auto listed = engine.list(keys::KeyPath{});

    if (!expect_true(listed.is_ok(), "empty prefix list should succeed"))
    {
      (void)engine.close();
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            listed.value().size(),
            2,
            "empty prefix should list all live values");

    (void)engine.close();

    return ok;
  }

  bool test_invalid_key_is_rejected()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto result = engine.set(
        keys::KeyPath{},
        value("bad"));

    const bool ok =
        expect_true(
            result.is_err(),
            "set with empty key should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidKey,
            "set with empty key should return InvalidKey");

    (void)engine.close();

    return ok;
  }

  bool test_operations_without_open_are_rejected()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    auto set_result = engine.set(
        key({"hello"}),
        value("world"));

    auto get_result = engine.get(key({"hello"}));

    auto erase_result = engine.erase(key({"hello"}));

    auto list_result = engine.list(key({"hello"}));

    return expect_true(
               set_result.is_err(),
               "set without open should fail") &&
           expect_error_code(
               set_result.error().code(),
               core::KvErrorCode::NotOpen,
               "set without open should return NotOpen") &&
           expect_true(
               get_result.is_err(),
               "get without open should fail") &&
           expect_error_code(
               get_result.error().code(),
               core::KvErrorCode::NotOpen,
               "get without open should return NotOpen") &&
           expect_true(
               erase_result.is_err(),
               "erase without open should fail") &&
           expect_error_code(
               erase_result.error().code(),
               core::KvErrorCode::NotOpen,
               "erase without open should return NotOpen") &&
           expect_true(
               list_result.is_err(),
               "list without open should fail") &&
           expect_error_code(
               list_result.error().code(),
               core::KvErrorCode::NotOpen,
               "list without open should return NotOpen");
  }

  bool test_flush_memory_engine()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    auto flushed = engine.flush();

    const bool ok = expect_true(
        flushed.is_ok(),
        "flush on memory engine should succeed");

    (void)engine.close();

    return ok;
  }

  bool test_durable_engine_creates_wal_and_recovers()
  {
    const auto root = make_test_root();

    {
      auto config = core::KvConfig::durable(root);

      internal::KvEngine engine{config};

      if (!expect_true(engine.open().is_ok(), "durable engine open should succeed"))
      {
        std::filesystem::remove_all(root);
        return false;
      }

      auto written = engine.set(
          key({"users", "1", "name"}),
          value("Ada"));

      if (!expect_true(written.is_ok(), "durable engine set should succeed"))
      {
        (void)engine.close();
        std::filesystem::remove_all(root);
        return false;
      }

      auto flushed = engine.flush();

      if (!expect_true(flushed.is_ok(), "durable engine flush should succeed"))
      {
        (void)engine.close();
        std::filesystem::remove_all(root);
        return false;
      }

      (void)engine.close();
    }

    {
      auto config = core::KvConfig::durable(root);

      internal::KvEngine recovered{config};

      if (!expect_true(
              recovered.open().is_ok(),
              "recovered durable engine open should succeed"))
      {
        std::filesystem::remove_all(root);
        return false;
      }

      auto read = recovered.get(
          key({"users", "1", "name"}));

      if (!expect_true(
              read.is_ok(),
              "recovered engine should read value from WAL"))
      {
        (void)recovered.close();
        std::filesystem::remove_all(root);
        return false;
      }

      const bool ok =
          expect_eq<std::string>(
              read.value().to_string(),
              "Ada",
              "recovered value should match original value") &&
          expect_true(
              recovered.contains(key({"users", "1", "name"})),
              "recovered engine should contain restored key") &&
          expect_eq<std::size_t>(
              recovered.size(),
              1,
              "recovered engine should have one live key");

      (void)recovered.close();
      std::filesystem::remove_all(root);

      return ok;
    }
  }

  bool test_durable_engine_recovers_delete()
  {
    const auto root = make_test_root();

    {
      auto config = core::KvConfig::durable(root);

      internal::KvEngine engine{config};

      if (!expect_true(engine.open().is_ok(), "durable engine open should succeed"))
      {
        std::filesystem::remove_all(root);
        return false;
      }

      (void)engine.set(
          key({"users", "1", "name"}),
          value("Ada"));

      (void)engine.erase(
          key({"users", "1", "name"}));

      (void)engine.flush();
      (void)engine.close();
    }

    {
      auto config = core::KvConfig::durable(root);

      internal::KvEngine recovered{config};

      if (!expect_true(
              recovered.open().is_ok(),
              "recovered engine open should succeed"))
      {
        std::filesystem::remove_all(root);
        return false;
      }

      auto read = recovered.get(
          key({"users", "1", "name"}));

      const bool ok =
          expect_true(
              read.is_err(),
              "deleted key should not recover as live value") &&
          expect_error_code(
              read.error().code(),
              core::KvErrorCode::NotFound,
              "deleted recovered key should return NotFound") &&
          expect_true(
              !recovered.contains(key({"users", "1", "name"})),
              "contains should be false for recovered tombstone") &&
          expect_eq<std::size_t>(
              recovered.size(),
              0,
              "recovered engine live size should be 0 after delete");

      (void)recovered.close();
      std::filesystem::remove_all(root);

      return ok;
    }
  }

  bool test_stats_update_after_operations()
  {
    auto config = core::KvConfig::memory_only();

    internal::KvEngine engine{config};

    if (!expect_true(engine.open().is_ok(), "engine open should succeed"))
    {
      return false;
    }

    (void)engine.set(key({"a"}), value("one"));
    (void)engine.set(key({"b"}), value("two"));
    (void)engine.erase(key({"a"}));

    const auto stats = engine.stats();

    const bool ok =
        expect_true(
            stats.live_keys >= 1,
            "stats should report at least one live key") &&
        expect_true(
            stats.tombstones >= 1,
            "stats should report at least one tombstone") &&
        expect_true(
            stats.last_sequence >= 3,
            "stats should report latest sequence");

    (void)engine.close();

    return ok;
  }

  bool test_config_accessor()
  {
    auto config = core::KvConfig::memory_only();
    config.auto_flush = false;

    internal::KvEngine engine{config};

    return expect_true(
        engine.config().memory_only,
        "engine config accessor should expose memory_only config");
  }
}

int main()
{
  if (!test_memory_engine_default_state())
  {
    return 1;
  }

  if (!test_open_memory_engine())
  {
    return 1;
  }

  if (!test_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_close_without_open_is_ok())
  {
    return 1;
  }

  if (!test_set_and_get_memory())
  {
    return 1;
  }

  if (!test_set_replaces_existing_value())
  {
    return 1;
  }

  if (!test_get_missing_key_returns_not_found())
  {
    return 1;
  }

  if (!test_erase_existing_key())
  {
    return 1;
  }

  if (!test_erase_missing_key_creates_no_live_value())
  {
    return 1;
  }

  if (!test_list_prefix())
  {
    return 1;
  }

  if (!test_list_empty_prefix_returns_all_live_values())
  {
    return 1;
  }

  if (!test_invalid_key_is_rejected())
  {
    return 1;
  }

  if (!test_operations_without_open_are_rejected())
  {
    return 1;
  }

  if (!test_flush_memory_engine())
  {
    return 1;
  }

  if (!test_durable_engine_creates_wal_and_recovers())
  {
    return 1;
  }

  if (!test_durable_engine_recovers_delete())
  {
    return 1;
  }

  if (!test_stats_update_after_operations())
  {
    return 1;
  }

  if (!test_config_accessor())
  {
    return 1;
  }

  std::cout << "kv_test_kv_engine passed\n";
  return 0;
}
