/**
 *
 *  @file test_open.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public open API tests
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
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
        "vix_kv_test_open_api";

    fs::remove_all(root);
    fs::create_directories(root);

    return root;
  }

  bool test_open_default_returns_result()
  {
    auto opened = vix::kv::open();

    if (!expect_true(
            opened.is_ok(),
            "vix::kv::open() should return an open result"))
    {
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "default opened database should be open") &&
        expect_true(
            db.empty(),
            "default opened database should be empty") &&
        expect_eq<std::size_t>(
            db.size(),
            0,
            "default opened database size should be 0");

    (void)db.close();

    return ok;
  }

  bool test_open_memory_returns_result()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "vix::kv::open_memory() should succeed"))
    {
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "memory database should be open") &&
        expect_true(
            db.options().memory_only_mode,
            "memory database options should be memory_only") &&
        expect_true(
            !db.stats().wal_enabled,
            "memory database should not enable WAL") &&
        expect_true(
            db.empty(),
            "memory database should start empty");

    (void)db.close();

    return ok;
  }

  bool test_open_durable_returns_result()
  {
    const auto root = make_test_root();

    auto opened = vix::kv::open_durable(root);

    if (!expect_true(
            opened.is_ok(),
            "vix::kv::open_durable(path) should succeed"))
    {
      fs::remove_all(root);
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "durable database should be open") &&
        expect_true(
            !db.options().memory_only_mode,
            "durable database should not be memory_only") &&
        expect_true(
            db.stats().wal_enabled,
            "durable database should enable WAL") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "durable database path should match requested root") &&
        expect_true(
            fs::exists(root),
            "durable open should create root directory");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_open_fast_returns_result()
  {
    const auto root = make_test_root();

    auto opened = vix::kv::open_fast(root);

    if (!expect_true(
            opened.is_ok(),
            "vix::kv::open_fast(path) should succeed"))
    {
      fs::remove_all(root);
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "fast database should be open") &&
        expect_true(
            !db.options().memory_only_mode,
            "fast database should not be memory_only") &&
        expect_true(
            db.stats().wal_enabled,
            "fast database should keep WAL enabled") &&
        expect_true(
            !db.options().auto_flush,
            "fast database should disable auto_flush") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "fast database path should match requested root");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_open_with_options_returns_result()
  {
    const auto root = make_test_root();

    auto options = vix::kv::KvOptions::durable(root);
    options.auto_flush = false;

    auto opened = vix::kv::open(options);

    if (!expect_true(
            opened.is_ok(),
            "vix::kv::open(options) should succeed"))
    {
      fs::remove_all(root);
      return false;
    }

    auto db = opened.move_value();

    const bool ok =
        expect_true(
            db.is_open(),
            "database opened with options should be open") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "database options path should be preserved") &&
        expect_true(
            !db.options().auto_flush,
            "database options auto_flush should be preserved");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_direct_open_path_returns_kv()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    const bool ok =
        expect_true(
            db.is_open(),
            "vix::kv::open(path) should return an open Kv handle") &&
        expect_true(
            db.stats().wal_enabled,
            "direct open(path) should create a durable KV") &&
        expect_eq<fs::path>(
            db.options().path,
            root,
            "direct open(path) should preserve root path");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_direct_open_path_can_write_and_read()
  {
    const auto root = make_test_root();

    auto db = vix::kv::open(root);

    db.put("hello", "world");

    const auto value = db.get("hello");

    const bool ok =
        expect_true(
            value.has_value(),
            "direct opened KV should read written value") &&
        expect_eq<std::string>(
            value.value(),
            "world",
            "direct opened KV should preserve written value");

    (void)db.close();
    fs::remove_all(root);

    return ok;
  }

  bool test_open_invalid_options_returns_error()
  {
    auto options = vix::kv::KvOptions::durable({});

    auto opened = vix::kv::open(options);

    if (!expect_true(
            opened.is_err(),
            "opening invalid options should fail"))
    {
      return false;
    }

    return expect_error_code(
        opened.error().code(),
        vix::kv::KvErrorCode::ConfigError,
        "invalid options should return ConfigError");
  }

  bool test_open_durable_empty_path_returns_error()
  {
    auto opened = vix::kv::open_durable({});

    if (!expect_true(
            opened.is_err(),
            "open_durable with empty path should fail"))
    {
      return false;
    }

    return expect_error_code(
        opened.error().code(),
        vix::kv::KvErrorCode::ConfigError,
        "open_durable empty path should return ConfigError");
  }

  bool test_open_fast_empty_path_returns_error()
  {
    auto opened = vix::kv::open_fast({});

    if (!expect_true(
            opened.is_err(),
            "open_fast with empty path should fail"))
    {
      return false;
    }

    return expect_error_code(
        opened.error().code(),
        vix::kv::KvErrorCode::ConfigError,
        "open_fast empty path should return ConfigError");
  }

  bool test_direct_open_empty_path_throws()
  {
    bool thrown = false;

    try
    {
      auto db = vix::kv::open(fs::path{});
      (void)db;
    }
    catch (const std::runtime_error &)
    {
      thrown = true;
    }

    return expect_true(
        thrown,
        "direct open(empty path) should throw std::runtime_error");
  }

  bool test_close_after_open()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed before close test"))
    {
      return false;
    }

    auto db = opened.move_value();

    auto closed = db.close();

    return expect_true(
               closed.is_ok(),
               "close after open should succeed") &&
           expect_true(
               !db.is_open(),
               "database should be closed after close()");
  }

  bool test_close_twice_is_ok()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed before close twice test"))
    {
      return false;
    }

    auto db = opened.move_value();

    auto first = db.close();
    auto second = db.close();

    return expect_true(
               first.is_ok(),
               "first close should succeed") &&
           expect_true(
               second.is_ok(),
               "second close should be a no-op success") &&
           expect_true(
               !db.is_open(),
               "database should remain closed");
  }

  bool test_move_opened_handle_preserves_state()
  {
    auto opened = vix::kv::open_memory();

    if (!expect_true(
            opened.is_ok(),
            "open_memory should succeed before move test"))
    {
      return false;
    }

    auto db = opened.move_value();

    db.put("hello", "world");

    auto moved = std::move(db);

    const auto value = moved.get("hello");

    const bool ok =
        expect_true(
            moved.is_open(),
            "moved database should remain open") &&
        expect_true(
            value.has_value(),
            "moved database should keep stored value") &&
        expect_eq<std::string>(
            value.value(),
            "world",
            "moved database value should match");

    (void)moved.close();

    return ok;
  }
}

int main()
{
  if (!test_open_default_returns_result())
  {
    return 1;
  }

  if (!test_open_memory_returns_result())
  {
    return 1;
  }

  if (!test_open_durable_returns_result())
  {
    return 1;
  }

  if (!test_open_fast_returns_result())
  {
    return 1;
  }

  if (!test_open_with_options_returns_result())
  {
    return 1;
  }

  if (!test_direct_open_path_returns_kv())
  {
    return 1;
  }

  if (!test_direct_open_path_can_write_and_read())
  {
    return 1;
  }

  if (!test_open_invalid_options_returns_error())
  {
    return 1;
  }

  if (!test_open_durable_empty_path_returns_error())
  {
    return 1;
  }

  if (!test_open_fast_empty_path_returns_error())
  {
    return 1;
  }

  if (!test_direct_open_empty_path_throws())
  {
    return 1;
  }

  if (!test_close_after_open())
  {
    return 1;
  }

  if (!test_close_twice_is_ok())
  {
    return 1;
  }

  if (!test_move_opened_handle_preserves_state())
  {
    return 1;
  }

  std::cout << "kv_test_open passed\n";
  return 0;
}
