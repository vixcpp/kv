/**
 *
 *  @file test_wal_reader.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL reader unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/wal/KvWalReader.hpp>
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
        "vix_kv_test_wal_reader";

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
      std::uint64_t sequence,
      std::string key,
      std::string value)
  {
    return records::KvRecord::put(
        std::move(key),
        bytes(std::move(value)),
        sequence,
        sequence * 100);
  }

  records::KvRecord make_delete_record(
      std::uint64_t sequence,
      std::string key)
  {
    return records::KvRecord::remove(
        std::move(key),
        sequence,
        sequence * 100);
  }

  bool write_records(
      const std::filesystem::path &wal_path,
      const std::vector<records::KvRecord> &items)
  {
    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (opened.is_err())
    {
      std::cerr << "failed to open writer: "
                << opened.error().message()
                << '\n';
      return false;
    }

    for (const auto &record : items)
    {
      auto appended = writer.append(record);

      if (appended.is_err())
      {
        std::cerr << "failed to append record: "
                  << appended.error().message()
                  << '\n';
        (void)writer.close();
        return false;
      }
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      std::cerr << "failed to close writer: "
                << closed.error().message()
                << '\n';
      return false;
    }

    return true;
  }

  bool write_raw_bytes(
      const std::filesystem::path &path,
      const std::vector<std::uint8_t> &data)
  {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream stream(
        path,
        std::ios::binary | std::ios::out | std::ios::trunc);

    if (!stream.is_open())
    {
      return false;
    }

    stream.write(
        reinterpret_cast<const char *>(data.data()),
        static_cast<std::streamsize>(data.size()));

    return static_cast<bool>(stream);
  }

  bool test_default_reader_is_closed()
  {
    const wal::KvWalReader reader;

    return expect_true(
               !reader.is_open(),
               "default reader should be closed") &&
           expect_true(
               !reader.eof(),
               "default reader should not be eof before open") &&
           expect_true(
               reader.path().empty(),
               "default reader path should be empty") &&
           expect_eq<std::uint64_t>(
               reader.records_read(),
               0,
               "default reader records_read should be 0") &&
           expect_eq<std::uint64_t>(
               reader.bytes_read(),
               0,
               "default reader bytes_read should be 0");
  }

  bool test_open_missing_wal_is_eof_success()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    const bool ok =
        expect_true(
            opened.is_ok(),
            "opening a missing WAL should succeed as empty") &&
        expect_true(
            reader.is_open(),
            "reader should be considered open for missing WAL") &&
        expect_true(
            reader.eof(),
            "missing WAL should put reader at eof") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            0,
            "missing WAL should read 0 records") &&
        expect_eq<std::uint64_t>(
            reader.bytes_read(),
            0,
            "missing WAL should read 0 bytes");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_next_missing_wal_returns_not_found()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_next();

    const bool ok =
        expect_true(
            record.is_err(),
            "read_next on missing WAL should fail") &&
        expect_error_code(
            record.error().code(),
            core::KvErrorCode::NotFound,
            "read_next on missing WAL should return NotFound");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_open_twice_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto first = reader.open();
    auto second = reader.open();

    const bool ok =
        expect_true(
            first.is_ok(),
            "first reader.open should succeed") &&
        expect_true(
            second.is_err(),
            "second reader.open should fail") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::AlreadyOpen,
            "opening an already open reader should return AlreadyOpen");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_single_record()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_next();

    if (!expect_true(record.is_ok(), "read_next should read first record"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const auto &value = record.value();

    const bool ok =
        expect_true(
            value.is_put(),
            "read record should be Put") &&
        expect_eq<std::string>(
            value.key,
            "v1|5:hello",
            "read record key should match") &&
        expect_true(
            value.value == bytes("world"),
            "read record value should match") &&
        expect_eq<std::uint64_t>(
            value.header.sequence,
            1,
            "read record sequence should match") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            1,
            "records_read should be 1 after one record") &&
        expect_true(
            reader.bytes_read() > 0,
            "bytes_read should be greater than 0 after one record");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_multiple_records_in_order()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {
                make_put_record(1, "v1|5:hello", "world"),
                make_put_record(2, "v1|5:users", "Ada"),
                make_delete_record(3, "v1|5:hello"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto first = reader.read_next();
    auto second = reader.read_next();
    auto third = reader.read_next();

    if (!expect_true(
            first.is_ok() && second.is_ok() && third.is_ok(),
            "reader should read all three records"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            first.value().header.sequence,
            1,
            "first record sequence should match") &&
        expect_eq<std::uint64_t>(
            second.value().header.sequence,
            2,
            "second record sequence should match") &&
        expect_eq<std::uint64_t>(
            third.value().header.sequence,
            3,
            "third record sequence should match") &&
        expect_true(
            third.value().is_delete(),
            "third record should be delete") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            3,
            "records_read should be 3 after three records");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_next_after_end_returns_not_found()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto first = reader.read_next();
    auto second = reader.read_next();

    const bool ok =
        expect_true(
            first.is_ok(),
            "first read_next should succeed") &&
        expect_true(
            second.is_err(),
            "second read_next should fail at EOF") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::NotFound,
            "read_next after end should return NotFound") &&
        expect_true(
            reader.eof(),
            "reader should be eof after reading past end");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_for_each_reads_all_records()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
                make_delete_record(3, "v1|1:a"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    std::uint64_t count = 0;
    std::uint64_t last_sequence = 0;

    auto result = reader.for_each(
        [&](const records::KvRecord &record)
        {
          ++count;
          last_sequence = record.header.sequence;
          return core::KvResult<void>::ok();
        });

    const bool ok =
        expect_true(
            result.is_ok(),
            "for_each should succeed") &&
        expect_eq<std::uint64_t>(
            count,
            3,
            "for_each should visit 3 records") &&
        expect_eq<std::uint64_t>(
            last_sequence,
            3,
            "for_each should visit records in order") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            3,
            "records_read should be 3 after for_each");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_for_each_stops_on_callback_error()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    std::uint64_t count = 0;

    auto result = reader.for_each(
        [&](const records::KvRecord &)
        {
          ++count;

          return core::KvResult<void>::err(
              core::KvError::make(
                  core::KvErrorCode::InternalError,
                  "forced callback failure"));
        });

    const bool ok =
        expect_true(
            result.is_err(),
            "for_each should stop on callback error") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InternalError,
            "for_each should return callback error") &&
        expect_eq<std::uint64_t>(
            count,
            1,
            "for_each should stop after first callback error");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_all_reads_all_records()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    if (!write_records(
            wal_path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto records_read = reader.read_all();

    if (!expect_true(records_read.is_ok(), "read_all should succeed"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            records_read.value().size(),
            2,
            "read_all should return 2 records") &&
        expect_eq<std::uint64_t>(
            records_read.value()[0].header.sequence,
            1,
            "read_all first sequence should match") &&
        expect_eq<std::uint64_t>(
            records_read.value()[1].header.sequence,
            2,
            "read_all second sequence should match");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_read_without_open_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    wal::KvWalReader reader{wal_path};

    auto result = reader.read_next();

    const bool ok =
        expect_true(
            result.is_err(),
            "read_next without open should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotOpen,
            "read_next without open should return NotOpen");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_close_without_open_is_ok()
  {
    wal::KvWalReader reader;

    auto result = reader.close();

    return expect_true(
        result.is_ok(),
        "close without open should be a no-op success");
  }

  bool test_open_empty_path_is_rejected()
  {
    wal::KvWalReader reader;

    auto result = reader.open();

    return expect_true(
               result.is_err(),
               "open with empty path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::WalError,
               "open with empty path should return WalError");
  }

  bool test_open_directory_path_is_rejected()
  {
    const auto root = make_test_root();

    wal::KvWalReader reader{root};

    auto result = reader.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "opening directory path as WAL should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::IoError,
            "opening directory path should return IoError");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_truncated_header_is_corruption()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    const std::vector<std::uint8_t> partial_header(
        records::KvRecordHeader::encoded_size - 1U,
        0x00U);

    if (!write_raw_bytes(wal_path, partial_header))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto result = reader.read_next();

    const bool ok =
        expect_true(
            result.is_err(),
            "truncated header should fail read_next") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::Corruption,
            "truncated header should return Corruption");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_truncated_payload_is_corruption()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    auto encoded =
        records::KvRecordEncoder::encode(
            make_put_record(1, "v1|5:hello", "world"));

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto data = encoded.move_value();

    data.pop_back();

    if (!write_raw_bytes(wal_path, data))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto result = reader.read_next();

    const bool ok =
        expect_true(
            result.is_err(),
            "truncated payload should fail read_next") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::Corruption,
            "truncated payload should return Corruption");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_corrupted_payload_checksum_is_rejected()
  {
    const auto root = make_test_root();
    const auto wal_path = root / "wal" / "current.wal";

    auto encoded =
        records::KvRecordEncoder::encode(
            make_put_record(1, "v1|5:hello", "world"));

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto data = encoded.move_value();
    data.back() ^= 0x01U;

    if (!write_raw_bytes(wal_path, data))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalReader reader{wal_path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto result = reader.read_next();

    const bool ok =
        expect_true(
            result.is_err(),
            "corrupted payload checksum should fail read_next") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::ChecksumMismatch,
            "corrupted payload should return ChecksumMismatch");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_default_reader_is_closed())
  {
    return 1;
  }

  if (!test_open_missing_wal_is_eof_success())
  {
    return 1;
  }

  if (!test_read_next_missing_wal_returns_not_found())
  {
    return 1;
  }

  if (!test_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_read_single_record())
  {
    return 1;
  }

  if (!test_read_multiple_records_in_order())
  {
    return 1;
  }

  if (!test_read_next_after_end_returns_not_found())
  {
    return 1;
  }

  if (!test_for_each_reads_all_records())
  {
    return 1;
  }

  if (!test_for_each_stops_on_callback_error())
  {
    return 1;
  }

  if (!test_read_all_reads_all_records())
  {
    return 1;
  }

  if (!test_read_without_open_is_rejected())
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

  if (!test_open_directory_path_is_rejected())
  {
    return 1;
  }

  if (!test_truncated_header_is_corruption())
  {
    return 1;
  }

  if (!test_truncated_payload_is_corruption())
  {
    return 1;
  }

  if (!test_corrupted_payload_checksum_is_rejected())
  {
    return 1;
  }

  std::cout << "kv_test_wal_reader passed\n";
  return 0;
}
