/**
 *
 *  @file test_data_file.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Data file reader/writer unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/storage/DataFileReader.hpp>
#include <vix/kv/storage/DataFileWriter.hpp>

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
  namespace storage = vix::kv::storage;

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
        "vix_kv_test_data_file";

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
      const std::filesystem::path &path,
      const std::vector<records::KvRecord> &items,
      std::vector<storage::DataFileWriteResult> *results = nullptr,
      bool truncate = true)
  {
    storage::DataFileWriter writer{path};

    auto opened = writer.open(truncate);

    if (opened.is_err())
    {
      std::cerr << "failed to open data writer: "
                << opened.error().message()
                << '\n';
      return false;
    }

    for (const auto &record : items)
    {
      auto written = writer.append(record);

      if (written.is_err())
      {
        std::cerr << "failed to append data record: "
                  << written.error().message()
                  << '\n';

        (void)writer.close();
        return false;
      }

      if (results != nullptr)
      {
        results->push_back(written.value());
      }
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      std::cerr << "failed to close data writer: "
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

  bool test_default_writer_is_closed()
  {
    const storage::DataFileWriter writer;

    return expect_true(
               !writer.is_open(),
               "default writer should be closed") &&
           expect_true(
               writer.path().empty(),
               "default writer path should be empty") &&
           expect_eq<std::uint64_t>(
               writer.offset(),
               0,
               "default writer offset should be 0") &&
           expect_eq<std::uint64_t>(
               writer.records_written(),
               0,
               "default writer records_written should be 0") &&
           expect_eq<std::uint64_t>(
               writer.bytes_written(),
               0,
               "default writer bytes_written should be 0");
  }

  bool test_default_reader_is_closed()
  {
    const storage::DataFileReader reader;

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
               reader.offset(),
               0,
               "default reader offset should be 0") &&
           expect_eq<std::uint64_t>(
               reader.records_read(),
               0,
               "default reader records_read should be 0") &&
           expect_eq<std::uint64_t>(
               reader.bytes_read(),
               0,
               "default reader bytes_read should be 0");
  }

  bool test_writer_open_creates_parent_directory()
  {
    const auto root = make_test_root();
    const auto path = root / "segments" / "data.log";

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    const bool ok =
        expect_true(
            opened.is_ok(),
            "writer.open should create parent directory and succeed") &&
        expect_true(
            writer.is_open(),
            "writer should be open after open") &&
        expect_true(
            std::filesystem::exists(path.parent_path()),
            "writer.open should create parent directory") &&
        expect_eq<std::filesystem::path>(
            writer.path(),
            path,
            "writer path should match requested path");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_open_twice_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileWriter writer{path};

    auto first = writer.open(true);
    auto second = writer.open(true);

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

  bool test_writer_empty_path_is_rejected()
  {
    storage::DataFileWriter writer;

    auto result = writer.open(true);

    return expect_true(
               result.is_err(),
               "writer.open with empty path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::StorageError,
               "writer.open with empty path should return StorageError");
  }

  bool test_writer_append_without_open_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileWriter writer{path};

    auto result = writer.append(
        make_put_record(1, "v1|5:hello", "world"));

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

  bool test_writer_append_record()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto record = make_put_record(1, "v1|5:hello", "world");

    auto written = writer.append(record);

    if (!expect_true(written.is_ok(), "writer.append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed") &&
        expect_true(
            written.value().is_valid(),
            "write result should be valid") &&
        expect_eq<std::uint64_t>(
            written.value().offset,
            0,
            "first write offset should be 0") &&
        expect_eq<std::uint64_t>(
            written.value().sequence,
            1,
            "write result sequence should match record sequence") &&
        expect_true(
            written.value().size > 0,
            "write result size should be greater than 0") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            1,
            "records_written should be 1") &&
        expect_eq<std::uint64_t>(
            writer.bytes_written(),
            written.value().size,
            "bytes_written should match write result size") &&
        expect_eq<std::uint64_t>(
            writer.offset(),
            written.value().size,
            "writer offset should advance by record size") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(path)),
            written.value().size,
            "file size should match written size");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_bytes()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    auto encoded =
        records::KvRecordEncoder::encode(
            make_put_record(1, "v1|5:hello", "world"));

    if (!expect_true(encoded.is_ok(), "record encoding should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto written = writer.append_bytes(encoded.value(), 1);

    if (!expect_true(written.is_ok(), "append_bytes should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed") &&
        expect_eq<std::uint64_t>(
            written.value().offset,
            0,
            "append_bytes first offset should be 0") &&
        expect_eq<std::uint64_t>(
            written.value().size,
            static_cast<std::uint64_t>(encoded.value().size()),
            "append_bytes size should match encoded bytes") &&
        expect_eq<std::uint64_t>(
            written.value().sequence,
            1,
            "append_bytes sequence should match input sequence") &&
        expect_eq<std::uint64_t>(
            writer.records_written(),
            1,
            "append_bytes should increment records_written") &&
        expect_eq<std::uint64_t>(
            writer.bytes_written(),
            static_cast<std::uint64_t>(encoded.value().size()),
            "append_bytes should increment bytes_written");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_empty_bytes_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const std::vector<std::uint8_t> empty;

    auto result = writer.append_bytes(empty, 1);

    const bool ok =
        expect_true(
            result.is_err(),
            "append_bytes with empty bytes should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "append_bytes with empty bytes should return InvalidArgument");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_bytes_zero_sequence_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    std::vector<std::uint8_t> data{1, 2, 3};

    auto result = writer.append_bytes(data, 0);

    const bool ok =
        expect_true(
            result.is_err(),
            "append_bytes with sequence 0 should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "append_bytes with sequence 0 should return InvalidArgument");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_mode_preserves_existing_bytes()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    std::vector<storage::DataFileWriteResult> first_results;

    if (!write_records(
            path,
            {make_put_record(1, "v1|1:a", "one")},
            &first_results,
            true))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileWriter writer{path};

    auto opened = writer.open(false);

    if (!expect_true(opened.is_ok(), "writer.open append mode should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto written = writer.append(
        make_put_record(2, "v1|1:b", "two"));

    if (!expect_true(
            written.is_ok(),
            "append mode writer.append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed after append mode") &&
        expect_eq<std::uint64_t>(
            written.value().offset,
            first_results[0].size,
            "append mode should start at existing file size") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(path)),
            first_results[0].size + written.value().size,
            "append mode should preserve existing bytes and append new bytes");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_truncate_mode_resets_existing_file()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileWriter writer{path};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "truncate writer.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto written = writer.append(
        make_put_record(3, "v1|1:c", "three"));

    if (!expect_true(
            written.is_ok(),
            "truncate writer.append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "writer.close should succeed after truncate") &&
        expect_eq<std::uint64_t>(
            written.value().offset,
            0,
            "truncate mode should reset offset to 0") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(path)),
            written.value().size,
            "truncate mode should replace existing file content");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_open_missing_file_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "missing.log";

    storage::DataFileReader reader{path};

    auto result = reader.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "reader.open missing file should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotFound,
            "reader.open missing file should return NotFound");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_open_empty_path_is_rejected()
  {
    storage::DataFileReader reader;

    auto result = reader.open();

    return expect_true(
               result.is_err(),
               "reader.open with empty path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::StorageError,
               "reader.open with empty path should return StorageError");
  }

  bool test_reader_open_directory_is_rejected()
  {
    const auto root = make_test_root();

    storage::DataFileReader reader{root};

    auto result = reader.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "reader.open directory path should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::IoError,
            "reader.open directory path should return IoError");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_open_twice_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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

  bool test_reader_read_single_record()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_next();

    if (!expect_true(record.is_ok(), "read_next should read one record"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const auto &value = record.value();

    const bool ok =
        expect_true(
            value.is_put(),
            "read record should be put") &&
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
            "reader records_read should be 1") &&
        expect_true(
            reader.bytes_read() > 0,
            "reader bytes_read should be greater than 0") &&
        expect_true(
            reader.offset() > 0,
            "reader offset should advance");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_reads_multiple_records_in_order()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
                make_delete_record(3, "v1|1:a"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            "reader should read all records"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            first.value().header.sequence,
            1,
            "first sequence should match") &&
        expect_eq<std::uint64_t>(
            second.value().header.sequence,
            2,
            "second sequence should match") &&
        expect_eq<std::uint64_t>(
            third.value().header.sequence,
            3,
            "third sequence should match") &&
        expect_true(
            third.value().is_delete(),
            "third record should be delete") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            3,
            "reader records_read should be 3");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_eof_returns_not_found()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            "first read should succeed") &&
        expect_true(
            second.is_err(),
            "second read should hit EOF") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::NotFound,
            "EOF should return NotFound") &&
        expect_true(
            reader.eof(),
            "reader should report eof after EOF");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_at_offset()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    std::vector<storage::DataFileWriteResult> results;

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            },
            &results,
            true))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto second = reader.read_at(results[1].offset);

    if (!expect_true(second.is_ok(), "read_at should read second record"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            second.value().header.sequence,
            2,
            "read_at should read record at requested offset") &&
        expect_eq<std::string>(
            second.value().key,
            "v1|1:b",
            "read_at should read second key");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_seek_then_read()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    std::vector<storage::DataFileWriteResult> results;

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            },
            &results,
            true))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader.open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto seeked = reader.seek(results[1].offset);

    if (!expect_true(seeked.is_ok(), "seek should succeed"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_next();

    if (!expect_true(record.is_ok(), "read after seek should succeed"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            record.value().header.sequence,
            2,
            "seek should move reader to second record") &&
        expect_eq<std::string>(
            record.value().key,
            "v1|1:b",
            "seek should read second key");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_for_each()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            2,
            "for_each should visit 2 records") &&
        expect_eq<std::uint64_t>(
            last_sequence,
            2,
            "for_each should preserve order") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            2,
            "for_each should update records_read");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_for_each_callback_error()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
                  "forced callback error"));
        });

    const bool ok =
        expect_true(
            result.is_err(),
            "for_each should return callback error") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InternalError,
            "for_each should propagate callback error") &&
        expect_eq<std::uint64_t>(
            count,
            1,
            "for_each should stop after first callback error");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_all()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    if (!write_records(
            path,
            {
                make_put_record(1, "v1|1:a", "one"),
                make_put_record(2, "v1|1:b", "two"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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

  bool test_reader_without_open_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    storage::DataFileReader reader{path};

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

  bool test_reader_close_without_open_is_ok()
  {
    storage::DataFileReader reader;

    auto result = reader.close();

    return expect_true(
        result.is_ok(),
        "reader.close without open should be success");
  }

  bool test_truncated_header_is_corruption()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

    const std::vector<std::uint8_t> partial_header(
        records::KvRecordHeader::encoded_size - 1U,
        0x00U);

    if (!write_raw_bytes(path, partial_header))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            "truncated header should fail") &&
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
    const auto path = root / "data.log";

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

    if (!write_raw_bytes(path, data))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            "truncated payload should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::Corruption,
            "truncated payload should return Corruption");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_bad_payload_checksum_is_rejected()
  {
    const auto root = make_test_root();
    const auto path = root / "data.log";

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

    if (!write_raw_bytes(path, data))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::DataFileReader reader{path};

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
            "bad payload checksum should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::ChecksumMismatch,
            "bad payload checksum should return ChecksumMismatch");

    (void)reader.close();
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

  if (!test_default_reader_is_closed())
  {
    return 1;
  }

  if (!test_writer_open_creates_parent_directory())
  {
    return 1;
  }

  if (!test_writer_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_writer_empty_path_is_rejected())
  {
    return 1;
  }

  if (!test_writer_append_without_open_is_rejected())
  {
    return 1;
  }

  if (!test_writer_append_record())
  {
    return 1;
  }

  if (!test_writer_append_bytes())
  {
    return 1;
  }

  if (!test_writer_append_empty_bytes_is_rejected())
  {
    return 1;
  }

  if (!test_writer_append_bytes_zero_sequence_is_rejected())
  {
    return 1;
  }

  if (!test_writer_append_mode_preserves_existing_bytes())
  {
    return 1;
  }

  if (!test_writer_truncate_mode_resets_existing_file())
  {
    return 1;
  }

  if (!test_reader_open_missing_file_is_rejected())
  {
    return 1;
  }

  if (!test_reader_open_empty_path_is_rejected())
  {
    return 1;
  }

  if (!test_reader_open_directory_is_rejected())
  {
    return 1;
  }

  if (!test_reader_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_reader_read_single_record())
  {
    return 1;
  }

  if (!test_reader_reads_multiple_records_in_order())
  {
    return 1;
  }

  if (!test_reader_eof_returns_not_found())
  {
    return 1;
  }

  if (!test_reader_read_at_offset())
  {
    return 1;
  }

  if (!test_reader_seek_then_read())
  {
    return 1;
  }

  if (!test_reader_for_each())
  {
    return 1;
  }

  if (!test_reader_for_each_callback_error())
  {
    return 1;
  }

  if (!test_reader_read_all())
  {
    return 1;
  }

  if (!test_reader_without_open_is_rejected())
  {
    return 1;
  }

  if (!test_reader_close_without_open_is_ok())
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

  if (!test_bad_payload_checksum_is_rejected())
  {
    return 1;
  }

  std::cout << "kv_test_data_file passed\n";
  return 0;
}
