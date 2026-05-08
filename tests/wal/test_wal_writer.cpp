/**
 *
 *  @file test_wal_writer.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL writer unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/wal/KvWalWriter.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;
  namespace wal = vix::kv::wal;

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
        "vix_kv_test_wal_writer";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    return root;
  }

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  records::KvRecord make_put_record(
      std::uint64_t sequence = 1)
  {
    return records::KvRecord::put(
        std::string{"v1|5:hello"},
        bytes("world"),
        sequence,
        100);
  }

  records::KvRecord make_delete_record(
      std::uint64_t sequence = 2)
  {
    return records::KvRecord::remove(
        std::string{"v1|5:hello"},
        sequence,
        200);
  }

  bool test_default_writer_is_closed()
  {
    const wal::KvWalWriter writer;

    return expect_true(
               !writer.is_open(),
               "default writer should be closed") &&
           expect_true(
               writer.path().empty(),
               "default writer path should be empty") &&
           expect_eq<std::uint64_t>(
               writer.records_written(),
               0,
               "default writer records_written should be 0") &&
           expect_eq<std::uint64_t>(
               writer.bytes_written(),
               0,
               "default writer bytes_written should be 0");
  }

  bool test_open_creates_parent_directory()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    const bool ok =
        expect_true(
            opened.is_ok(),
            "writer.open should create parent directory and succeed") &&
        expect_true(
            writer.is_open(),
            "writer should be open after open()") &&
        expect_true(
            std::filesystem::exists(wal_path.parent_path()),
            "writer.open should create WAL parent directory") &&
        expect_eq<std::filesystem::path>(
            writer.path(),
            wal_path,
            "writer path should match requested path");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_open_twice_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto first = writer.open();
    auto second = writer.open();

    const bool ok =
        expect_true(
            first.is_ok(),
            "first writer.open should succeed") &&
        expect_true(
            second.is_err(),
            "second writer.open should fail") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::AlreadyOpen,
            "opening an already open writer should return AlreadyOpen");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_append_record_writes_bytes()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto record = make_put_record(1);

    auto appended = writer.append(record);

    if (!expect_true(appended.is_ok(), "writer.append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const auto file_size = std::filesystem::file_size(wal_path);

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed") &&
        expect_true(
            std::filesystem::exists(wal_path),
            "WAL file should exist after append") &&
        expect_true(
            file_size > 0,
            "WAL file should contain bytes after append") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            1,
            "records_written should be 1 after one append") &&
        expect_eq<std::uint64_t>(
            writer.bytes_written(),
            static_cast<std::uint64_t>(file_size),
            "bytes_written should match WAL file size");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_append_multiple_records()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto first = writer.append(make_put_record(1));
    auto second = writer.append(make_delete_record(2));

    if (!expect_true(
            first.is_ok() && second.is_ok(),
            "multiple writer.append calls should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed after multiple appends") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            2,
            "records_written should count multiple appends") &&
        expect_true(
            writer.bytes_written() > 0,
            "bytes_written should be greater than 0 after multiple appends") &&
        expect_true(
            std::filesystem::file_size(wal_path) ==
                writer.bytes_written(),
            "WAL file size should match bytes_written");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_append_bytes_writes_encoded_record()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto encoded =
        records::KvRecordEncoder::encode(make_put_record(1));

    if (!expect_true(
            encoded.is_ok(),
            "record encoding should succeed before append_bytes"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const auto expected_size =
        static_cast<std::uint64_t>(encoded.value().size());

    auto appended = writer.append_bytes(encoded.value());

    if (!expect_true(
            appended.is_ok(),
            "writer.append_bytes should succeed for encoded record"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed after append_bytes") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            1,
            "append_bytes should increment records_written") &&
        expect_eq<std::uint64_t>(
            writer.bytes_written(),
            expected_size,
            "append_bytes should update bytes_written") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(wal_path)),
            expected_size,
            "WAL file size should match encoded record size");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_append_without_open_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto result = writer.append(make_put_record(1));

    const bool ok =
        expect_true(
            result.is_err(),
            "append without open should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotOpen,
            "append without open should return NotOpen");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_append_empty_bytes_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const std::vector<std::uint8_t> empty;

    auto result = writer.append_bytes(empty);

    const bool ok =
        expect_true(
            result.is_err(),
            "append_bytes with empty bytes should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "append_bytes with empty bytes should return InvalidArgument") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            0,
            "failed append_bytes should not increment records_written") &&
        expect_eq<std::uint64_t>(
            writer.bytes_written(),
            0,
            "failed append_bytes should not increment bytes_written");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_flush_without_open_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalWriter writer{wal_path};

    auto result = writer.flush();

    const bool ok =
        expect_true(
            result.is_err(),
            "flush without open should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotOpen,
            "flush without open should return NotOpen");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_close_without_open_is_ok()
  {
    wal::KvWalWriter writer;

    auto result = writer.close();

    return expect_true(
        result.is_ok(),
        "close without open should be a no-op success");
  }

  bool test_open_empty_path_is_rejected()
  {
    wal::KvWalWriter writer;

    auto result = writer.open();

    return expect_true(
               result.is_err(),
               "open with empty path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::WalError,
               "open with empty path should return WalError");
  }

  bool test_read_only_config_rejects_writer_open()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    config.read_only = true;

    wal::KvWalWriter writer{config};

    auto result = writer.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "writer.open should fail in read-only mode") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::Unsupported,
            "writer.open in read-only mode should return Unsupported");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_missing_parent_without_create_directories_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    config.wal_path = root / "missing" / "current.wal";
    config.create_directories = false;

    wal::KvWalWriter writer{config};

    auto result = writer.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "writer.open should fail when parent is missing and create_directories is false") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::IoError,
            "missing parent without create_directories should return IoError");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_auto_flush_enabled()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    config.auto_flush = true;

    wal::KvWalWriter writer{config};

    auto opened = writer.open();

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto appended = writer.append(make_put_record(1));

    if (!expect_true(
            appended.is_ok(),
            "writer.append should succeed with auto_flush enabled"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool file_has_bytes =
        std::filesystem::exists(config.wal_path) &&
        std::filesystem::file_size(config.wal_path) > 0;

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed with auto_flush enabled") &&
        expect_true(
            file_has_bytes,
            "auto_flush writer should publish bytes to file");

    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_default_writer_is_closed())
  {
    return 1;
  }

  if (!test_open_creates_parent_directory())
  {
    return 1;
  }

  if (!test_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_append_record_writes_bytes())
  {
    return 1;
  }

  if (!test_append_multiple_records())
  {
    return 1;
  }

  if (!test_append_bytes_writes_encoded_record())
  {
    return 1;
  }

  if (!test_append_without_open_is_rejected())
  {
    return 1;
  }

  if (!test_append_empty_bytes_is_rejected())
  {
    return 1;
  }

  if (!test_flush_without_open_is_rejected())
  {
    return 1;
  }

  if (!test_close_without_open_is_ok())
  {
    return 1;
  }

  if (!test_open_empty_path_is_rejected())
  {
    return 1;
  }

  if (!test_read_only_config_rejects_writer_open())
  {
    return 1;
  }

  if (!test_missing_parent_without_create_directories_is_rejected())
  {
    return 1;
  }

  if (!test_auto_flush_enabled())
  {
    return 1;
  }

  std::cout << "kv_test_wal_writer passed\n";
  return 0;
}
