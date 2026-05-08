/**
 *
 *  @file test_segment.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment reader/writer unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/storage/FileLayout.hpp>
#include <vix/kv/storage/Segment.hpp>
#include <vix/kv/storage/SegmentReader.hpp>
#include <vix/kv/storage/SegmentWriter.hpp>

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
        "vix_kv_test_segment";

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

  bool test_default_segment_is_invalid()
  {
    const storage::Segment segment;

    return expect_true(
               !segment.has_id(),
               "default segment should not have id") &&
           expect_true(
               !segment.has_path(),
               "default segment should not have path") &&
           expect_true(
               !segment.has_records(),
               "default segment should not have records") &&
           expect_true(
               !segment.has_sequence_range(),
               "default segment should not have sequence range") &&
           expect_true(
               !segment.has_size(),
               "default segment should not have size") &&
           expect_true(
               !segment.is_valid(),
               "default segment should be invalid");
  }

  bool test_segment_constructor()
  {
    const auto path = std::filesystem::path{"data/segments/000001.seg"};

    const storage::Segment segment{1, path};

    return expect_true(
               segment.has_id(),
               "constructed segment should have id") &&
           expect_true(
               segment.has_path(),
               "constructed segment should have path") &&
           expect_eq<std::uint64_t>(
               segment.id,
               1,
               "constructed segment id should match") &&
           expect_eq<std::filesystem::path>(
               segment.path,
               path,
               "constructed segment path should match") &&
           expect_true(
               segment.is_valid(),
               "constructed empty segment with id and path should be valid");
  }

  bool test_segment_observe_record_updates_metadata()
  {
    storage::Segment segment{3, "segment-3.dat"};

    segment.observe_record(44, 10);
    segment.observe_record(50, 12);

    return expect_eq<std::uint64_t>(
               segment.size_bytes,
               94,
               "observe_record should add record sizes") &&
           expect_eq<std::uint64_t>(
               segment.record_count,
               2,
               "observe_record should increment record count") &&
           expect_eq<std::uint64_t>(
               segment.min_sequence,
               10,
               "observe_record should track min sequence") &&
           expect_eq<std::uint64_t>(
               segment.max_sequence,
               12,
               "observe_record should track max sequence") &&
           expect_true(
               segment.has_records(),
               "segment should have records after observe_record") &&
           expect_true(
               segment.has_sequence_range(),
               "segment should have sequence range after observe_record") &&
           expect_true(
               segment.has_size(),
               "segment should have size after observe_record") &&
           expect_true(
               segment.is_valid(),
               "segment should be valid after observing records");
  }

  bool test_segment_observe_sequence_ignores_zero()
  {
    storage::Segment segment{3, "segment-3.dat"};

    segment.observe_sequence(0);

    return expect_eq<std::uint64_t>(
               segment.min_sequence,
               0,
               "observe_sequence should ignore zero for min") &&
           expect_eq<std::uint64_t>(
               segment.max_sequence,
               0,
               "observe_sequence should ignore zero for max");
  }

  bool test_segment_seal_and_clear()
  {
    storage::Segment segment{3, "segment-3.dat"};

    segment.observe_record(44, 10);
    segment.seal();

    const bool sealed = segment.sealed;

    segment.clear();

    return expect_true(
               sealed,
               "seal should mark segment as sealed") &&
           expect_eq<std::uint64_t>(
               segment.id,
               0,
               "clear should reset id") &&
           expect_true(
               segment.path.empty(),
               "clear should reset path") &&
           expect_eq<std::uint64_t>(
               segment.size_bytes,
               0,
               "clear should reset size") &&
           expect_eq<std::uint64_t>(
               segment.record_count,
               0,
               "clear should reset record count") &&
           expect_eq<std::uint64_t>(
               segment.min_sequence,
               0,
               "clear should reset min sequence") &&
           expect_eq<std::uint64_t>(
               segment.max_sequence,
               0,
               "clear should reset max sequence") &&
           expect_true(
               !segment.sealed,
               "clear should reset sealed flag");
  }

  bool test_default_writer_is_closed()
  {
    const storage::SegmentWriter writer;

    return expect_true(
               !writer.is_open(),
               "default segment writer should be closed") &&
           expect_eq<std::uint64_t>(
               writer.segment().id,
               0,
               "default segment writer id should be 0") &&
           expect_true(
               writer.segment().path.empty(),
               "default segment writer path should be empty");
  }

  bool test_writer_open_creates_segment_path()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    auto opened = writer.open(true);

    const auto expected_path =
        storage::FileLayout::segment_path(config, 1);

    const bool ok =
        expect_true(
            opened.is_ok(),
            "segment writer open should succeed") &&
        expect_true(
            writer.is_open(),
            "segment writer should be open") &&
        expect_eq<std::uint64_t>(
            writer.segment().id,
            1,
            "segment writer should use requested segment id") &&
        expect_eq<std::filesystem::path>(
            writer.segment().path,
            expected_path,
            "segment writer path should match FileLayout segment path") &&
        expect_true(
            std::filesystem::exists(expected_path.parent_path()),
            "segment writer should create parent directory");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_open_twice_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    auto first = writer.open(true);
    auto second = writer.open(true);

    const bool ok =
        expect_true(
            first.is_ok(),
            "first segment writer open should succeed") &&
        expect_true(
            second.is_err(),
            "second segment writer open should fail") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::AlreadyOpen,
            "opening already open segment writer should return AlreadyOpen");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_invalid_segment_id_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 0};

    auto result = writer.open(true);

    const bool ok =
        expect_true(
            result.is_err(),
            "segment writer with id 0 should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::StorageError,
            "segment writer with id 0 should return StorageError");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_record_updates_segment_metadata()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 7};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "segment writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto first = make_put_record(10, "v1|1:a", "one");
    const auto second = make_put_record(12, "v1|1:b", "two");

    auto first_written = writer.append(first);
    auto second_written = writer.append(second);

    if (!expect_true(
            first_written.is_ok() && second_written.is_ok(),
            "segment writer should append both records"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    auto closed = writer.close();

    const auto segment = writer.segment();

    const bool ok =
        expect_true(
            closed.is_ok(),
            "segment writer close should succeed") &&
        expect_true(
            segment.sealed,
            "segment writer close should seal segment") &&
        expect_eq<std::uint64_t>(
            segment.id,
            7,
            "segment id should match writer id") &&
        expect_eq<std::uint64_t>(
            segment.record_count,
            2,
            "segment should count appended records") &&
        expect_eq<std::uint64_t>(
            segment.min_sequence,
            10,
            "segment should track min sequence") &&
        expect_eq<std::uint64_t>(
            segment.max_sequence,
            12,
            "segment should track max sequence") &&
        expect_eq<std::uint64_t>(
            segment.size_bytes,
            first_written.value().size + second_written.value().size,
            "segment size should equal appended encoded bytes") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(segment.path)),
            segment.size_bytes,
            "segment file size should match metadata size") &&
        expect_true(
            segment.is_valid(),
            "written segment should be valid");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_without_open_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    auto result = writer.append(
        make_put_record(1, "v1|5:hello", "world"));

    const bool ok =
        expect_true(
            result.is_err(),
            "segment writer append without open should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotOpen,
            "segment writer append without open should return NotOpen");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_empty_bytes_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "segment writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const std::vector<std::uint8_t> empty;

    auto result = writer.append_bytes(empty, 1);

    const bool ok =
        expect_true(
            result.is_err(),
            "segment writer append_bytes empty should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "segment writer append_bytes empty should return InvalidArgument");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_writer_append_bytes_zero_sequence_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    auto opened = writer.open(true);

    if (!expect_true(opened.is_ok(), "segment writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const std::vector<std::uint8_t> bytes{1, 2, 3};

    auto result = writer.append_bytes(bytes, 0);

    const bool ok =
        expect_true(
            result.is_err(),
            "segment writer append_bytes sequence 0 should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "segment writer append_bytes sequence 0 should return InvalidArgument");

    (void)writer.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_default_reader_is_closed()
  {
    const storage::SegmentReader reader;

    return expect_true(
               !reader.is_open(),
               "default segment reader should be closed") &&
           expect_true(
               !reader.eof(),
               "default segment reader should not be eof before open") &&
           expect_true(
               !reader.segment().has_id(),
               "default segment reader should not have segment id") &&
           expect_true(
               !reader.segment().has_path(),
               "default segment reader should not have segment path");
  }

  bool test_reader_open_missing_segment_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    auto segment = storage::Segment{
        1,
        storage::FileLayout::segment_path(config, 1)};

    storage::SegmentReader reader{segment};

    auto result = reader.open();

    const bool ok =
        expect_true(
            result.is_err(),
            "segment reader missing file should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotFound,
            "segment reader missing file should return NotFound");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_open_invalid_segment_is_rejected()
  {
    storage::SegmentReader reader{storage::Segment{}};

    auto result = reader.open();

    return expect_true(
               result.is_err(),
               "segment reader invalid segment should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::StorageError,
               "segment reader invalid segment should return StorageError");
  }

  bool test_reader_open_twice_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    if (!expect_true(
            writer.append(
                      make_put_record(1, "v1|5:hello", "world"))
                .is_ok(),
            "writer append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    if (!expect_true(writer.close().is_ok(), "writer close should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::SegmentReader reader{writer.segment()};

    auto first = reader.open();
    auto second = reader.open();

    const bool ok =
        expect_true(
            first.is_ok(),
            "first segment reader open should succeed") &&
        expect_true(
            second.is_err(),
            "second segment reader open should fail") &&
        expect_error_code(
            second.error().code(),
            core::KvErrorCode::AlreadyOpen,
            "opening already open segment reader should return AlreadyOpen");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_reads_single_record()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    if (!expect_true(
            writer.append(
                      make_put_record(1, "v1|5:hello", "world"))
                .is_ok(),
            "writer append should succeed"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    if (!expect_true(writer.close().is_ok(), "writer close should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::SegmentReader reader{writer.segment()};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_next();

    if (!expect_true(record.is_ok(), "reader should read first record"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            record.value().is_put(),
            "read record should be put") &&
        expect_eq<std::string>(
            record.value().key,
            "v1|5:hello",
            "read record key should match") &&
        expect_true(
            record.value().value == bytes("world"),
            "read record value should match") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            1,
            "reader should count one record");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_reads_multiple_records_in_order()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.append(make_put_record(1, "v1|1:a", "one"));
    (void)writer.append(make_put_record(2, "v1|1:b", "two"));
    (void)writer.append(make_delete_record(3, "v1|1:a"));

    if (!expect_true(writer.close().is_ok(), "writer close should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    storage::SegmentReader reader{writer.segment()};

    auto opened = reader.open();

    if (!expect_true(opened.is_ok(), "reader open should succeed"))
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
            "reader should count three records");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_eof_returns_not_found()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.append(make_put_record(1, "v1|5:hello", "world"));
    (void)writer.close();

    storage::SegmentReader reader{writer.segment()};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
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
            "reader should report EOF");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_at_offset()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto first = writer.append(make_put_record(1, "v1|1:a", "one"));
    auto second = writer.append(make_put_record(2, "v1|1:b", "two"));

    if (!expect_true(
            first.is_ok() && second.is_ok(),
            "writer should append records"))
    {
      (void)writer.close();
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.close();

    storage::SegmentReader reader{writer.segment()};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto record = reader.read_at(second.value().offset);

    if (!expect_true(record.is_ok(), "read_at should read second record"))
    {
      (void)reader.close();
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            record.value().header.sequence,
            2,
            "read_at should read record at requested offset") &&
        expect_eq<std::string>(
            record.value().key,
            "v1|1:b",
            "read_at should read second key");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_for_each_reads_all_records()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.append(make_put_record(1, "v1|1:a", "one"));
    (void)writer.append(make_put_record(2, "v1|1:b", "two"));
    (void)writer.close();

    storage::SegmentReader reader{writer.segment()};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
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
            "for_each should visit two records") &&
        expect_eq<std::uint64_t>(
            last_sequence,
            2,
            "for_each should preserve order") &&
        expect_eq<std::uint64_t>(
            reader.records_read(),
            2,
            "for_each should update reader record count");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_for_each_callback_error()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.append(make_put_record(1, "v1|1:a", "one"));
    (void)writer.append(make_put_record(2, "v1|1:b", "two"));
    (void)writer.close();

    storage::SegmentReader reader{writer.segment()};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
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

    auto config = core::KvConfig::durable(root);

    storage::SegmentWriter writer{config, 1};

    if (!expect_true(writer.open(true).is_ok(), "writer open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    (void)writer.append(make_put_record(1, "v1|1:a", "one"));
    (void)writer.append(make_put_record(2, "v1|1:b", "two"));
    (void)writer.close();

    storage::SegmentReader reader{writer.segment()};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
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
            "read_all should return two records") &&
        expect_eq<std::uint64_t>(
            records_read.value()[0].header.sequence,
            1,
            "first record sequence should match") &&
        expect_eq<std::uint64_t>(
            records_read.value()[1].header.sequence,
            2,
            "second record sequence should match");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_without_open_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    auto segment = storage::Segment{
        1,
        storage::FileLayout::segment_path(config, 1)};

    storage::SegmentReader reader{segment};

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
    storage::SegmentReader reader;

    auto result = reader.close();

    return expect_true(
        result.is_ok(),
        "segment reader close without open should succeed");
  }

  bool test_reader_detects_corrupted_payload()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    const auto path = storage::FileLayout::segment_path(config, 1);

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

    auto segment = storage::Segment{1, path};
    segment.observe_record(
        static_cast<std::uint64_t>(data.size()),
        1);
    segment.seal();

    storage::SegmentReader reader{segment};

    if (!expect_true(reader.open().is_ok(), "reader open should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto result = reader.read_next();

    const bool ok =
        expect_true(
            result.is_err(),
            "corrupted segment payload should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::ChecksumMismatch,
            "corrupted segment payload should return ChecksumMismatch");

    (void)reader.close();
    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_default_segment_is_invalid())
  {
    return 1;
  }

  if (!test_segment_constructor())
  {
    return 1;
  }

  if (!test_segment_observe_record_updates_metadata())
  {
    return 1;
  }

  if (!test_segment_observe_sequence_ignores_zero())
  {
    return 1;
  }

  if (!test_segment_seal_and_clear())
  {
    return 1;
  }

  if (!test_default_writer_is_closed())
  {
    return 1;
  }

  if (!test_writer_open_creates_segment_path())
  {
    return 1;
  }

  if (!test_writer_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_writer_invalid_segment_id_is_rejected())
  {
    return 1;
  }

  if (!test_writer_append_record_updates_segment_metadata())
  {
    return 1;
  }

  if (!test_writer_append_without_open_is_rejected())
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

  if (!test_default_reader_is_closed())
  {
    return 1;
  }

  if (!test_reader_open_missing_segment_is_rejected())
  {
    return 1;
  }

  if (!test_reader_open_invalid_segment_is_rejected())
  {
    return 1;
  }

  if (!test_reader_open_twice_is_rejected())
  {
    return 1;
  }

  if (!test_reader_reads_single_record())
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

  if (!test_reader_for_each_reads_all_records())
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

  if (!test_reader_read_without_open_is_rejected())
  {
    return 1;
  }

  if (!test_reader_close_without_open_is_ok())
  {
    return 1;
  }

  if (!test_reader_detects_corrupted_payload())
  {
    return 1;
  }

  std::cout << "kv_test_segment passed\n";
  return 0;
}
