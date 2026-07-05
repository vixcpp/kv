/**
 *
 *  @file test_snapshot.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot reader/writer unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/memtable/MemTableEntry.hpp>
#include <vix/kv/snapshot/Snapshot.hpp>
#include <vix/kv/snapshot/SnapshotReader.hpp>
#include <vix/kv/snapshot/SnapshotWriter.hpp>
#include <vix/kv/storage/FileLayout.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace memtable = vix::kv::memtable;
  namespace snapshot = vix::kv::snapshot;
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
        "vix_kv_test_snapshot";

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

  memtable::MemTableEntry live_entry(
      std::string key,
      std::string value,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return memtable::MemTableEntry::live(
        std::move(key),
        bytes(std::move(value)),
        sequence,
        timestamp_ms);
  }

  memtable::MemTableEntry tombstone_entry(
      std::string key,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return memtable::MemTableEntry::tombstone(
        std::move(key),
        sequence,
        timestamp_ms);
  }

  snapshot::Snapshot make_snapshot()
  {
    return snapshot::Snapshot::make(
        7,
        700,
        {
            live_entry("v1|5:users1:1", "Ada", 1, 100),
            live_entry("v1|5:users1:2", "Grace", 2, 200),
            tombstone_entry("v1|5:users1:3", 3, 300),
        });
  }

bool same_entry(
      const memtable::MemTableEntry &left,
      const memtable::MemTableEntry &right)
  {
    return left.key == right.key &&
           left.value == right.value &&
           left.sequence == right.sequence &&
           left.timestamp_ms == right.timestamp_ms &&
           left.deleted == right.deleted;
  }

  bool test_default_snapshot_is_invalid()
  {
    const snapshot::Snapshot item;

    return expect_true(
               !item.has_id(),
               "default snapshot should not have id") &&
           expect_true(
               !item.has_sequence(),
               "default snapshot should not have sequence") &&
           expect_true(
               !item.has_entries(),
               "default snapshot should not have entries") &&
           expect_true(
               item.empty(),
               "default snapshot should be empty") &&
           expect_eq<std::size_t>(
               item.size(),
               0,
               "default snapshot size should be 0") &&
           expect_true(
               !item.is_valid(),
               "default snapshot should be invalid because id is 0");
  }

  bool test_make_computes_last_sequence()
  {
    auto item = snapshot::Snapshot::make(
        3,
        1000,
        {
            live_entry("a", "one", 10, 100),
            tombstone_entry("b", 12, 120),
            live_entry("c", "three", 11, 110),
        });

    return expect_eq<std::uint64_t>(
               item.id,
               3,
               "Snapshot::make should preserve id") &&
           expect_eq<std::uint64_t>(
               item.created_at_ms,
               1000,
               "Snapshot::make should preserve timestamp") &&
           expect_eq<std::uint64_t>(
               item.last_sequence,
               12,
               "Snapshot::make should compute max sequence") &&
           expect_eq<std::size_t>(
               item.size(),
               3,
               "Snapshot::make should preserve entries") &&
           expect_true(
               item.is_valid(),
               "Snapshot::make should create a valid snapshot");
  }

  bool test_counts_and_byte_size()
  {
    auto item = make_snapshot();

    const std::uint64_t expected_bytes =
        item.entries[0].byte_size() +
        item.entries[1].byte_size() +
        item.entries[2].byte_size();

    return expect_eq<std::size_t>(
               item.live_count(),
               2,
               "snapshot live_count should count live entries") &&
           expect_eq<std::size_t>(
               item.tombstone_count(),
               1,
               "snapshot tombstone_count should count tombstones") &&
           expect_eq<std::uint64_t>(
               item.byte_size(),
               expected_bytes,
               "snapshot byte_size should sum entry byte sizes");
  }

  bool test_add_updates_last_sequence()
  {
    snapshot::Snapshot item;
    item.id = 1;
    item.created_at_ms = 100;

    item.add(live_entry("a", "one", 2, 20));
    item.add(live_entry("b", "two", 5, 50));
    item.add(tombstone_entry("c", 3, 30));

    return expect_eq<std::size_t>(
               item.size(),
               3,
               "add should append entries") &&
           expect_eq<std::uint64_t>(
               item.last_sequence,
               5,
               "add should keep max last_sequence") &&
           expect_true(
               item.is_valid(),
               "snapshot should be valid after adding entries");
  }

  bool test_refresh_last_sequence()
  {
    snapshot::Snapshot item{
        1,
        999,
        100,
        {
            live_entry("a", "one", 1, 10),
            live_entry("b", "two", 7, 70),
            tombstone_entry("c", 4, 40),
        }};

    item.refresh_last_sequence();

    return expect_eq<std::uint64_t>(
        item.last_sequence,
        7,
        "refresh_last_sequence should recompute max sequence");
  }

  bool test_clear_resets_snapshot()
  {
    auto item = make_snapshot();

    item.clear();

    return expect_eq<std::uint64_t>(
               item.id,
               0,
               "clear should reset id") &&
           expect_eq<std::uint64_t>(
               item.last_sequence,
               0,
               "clear should reset last_sequence") &&
           expect_eq<std::uint64_t>(
               item.created_at_ms,
               0,
               "clear should reset timestamp") &&
           expect_true(
               item.entries.empty(),
               "clear should remove entries") &&
           expect_true(
               !item.is_valid(),
               "cleared snapshot should be invalid");
  }

  bool test_writer_validate_accepts_valid_snapshot()
  {
    const auto item = make_snapshot();

    auto result = snapshot::SnapshotWriter::validate(item);

    return expect_true(
        result.is_ok(),
        "SnapshotWriter::validate should accept valid snapshot");
  }

  bool test_writer_validate_rejects_zero_id()
  {
    auto item = make_snapshot();
    item.id = 0;

    auto result = snapshot::SnapshotWriter::validate(item);

    if (!expect_true(
            result.is_err(),
            "SnapshotWriter::validate should reject id 0"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "snapshot id 0 should return InvalidArgument");
  }

  bool test_writer_validate_rejects_invalid_entry()
  {
    auto item = make_snapshot();
    item.entries.push_back(memtable::MemTableEntry{});

    auto result = snapshot::SnapshotWriter::validate(item);

    if (!expect_true(
            result.is_err(),
            "SnapshotWriter::validate should reject invalid entries"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "invalid snapshot entry should return InvalidArgument");
  }

  bool test_encode_decode_roundtrip()
  {
    const auto item = make_snapshot();

    auto encoded = snapshot::SnapshotWriter::encode(item);

    if (!expect_true(encoded.is_ok(), "snapshot encode should succeed"))
    {
      return false;
    }

    auto decoded = snapshot::SnapshotReader::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "snapshot decode should succeed"))
    {
      return false;
    }

    const auto &out = decoded.value();

    if (!expect_eq<std::uint64_t>(
            out.id,
            item.id,
            "decoded snapshot id should match"))
    {
      return false;
    }

    if (!expect_eq<std::uint64_t>(
            out.last_sequence,
            item.last_sequence,
            "decoded snapshot last_sequence should match"))
    {
      return false;
    }

    if (!expect_eq<std::uint64_t>(
            out.created_at_ms,
            item.created_at_ms,
            "decoded snapshot timestamp should match"))
    {
      return false;
    }

    if (!expect_eq<std::size_t>(
            out.entries.size(),
            item.entries.size(),
            "decoded snapshot entry count should match"))
    {
      return false;
    }

    for (std::size_t index = 0; index < item.entries.size(); ++index)
    {
      if (!expect_true(
              same_entry(out.entries[index], item.entries[index]),
              "decoded snapshot entry should match original entry"))
      {
        return false;
      }
    }

    return expect_true(
        out.is_valid(),
        "decoded snapshot should be valid");
  }

  bool test_write_uses_layout_path()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    auto item = make_snapshot();

    snapshot::SnapshotWriter writer{config};

    auto written = writer.write(item);

    if (!expect_true(written.is_ok(), "snapshot writer.write should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto expected_path =
        storage::FileLayout::snapshot_path(config, item.id);

    const bool ok =
        expect_true(
            written.value().is_valid(),
            "snapshot write result should be valid") &&
        expect_eq<std::uint64_t>(
            written.value().snapshot_id,
            item.id,
            "write result snapshot id should match") &&
        expect_eq<std::filesystem::path>(
            written.value().path,
            expected_path,
            "writer.write should use FileLayout snapshot path") &&
        expect_eq<std::uint64_t>(
            written.value().entry_count,
            static_cast<std::uint64_t>(item.entries.size()),
            "write result entry count should match") &&
        expect_eq<std::uint64_t>(
            written.value().last_sequence,
            item.last_sequence,
            "write result last_sequence should match") &&
        expect_true(
            std::filesystem::exists(expected_path),
            "snapshot file should exist after write") &&
        expect_eq<std::uint64_t>(
            static_cast<std::uint64_t>(
                std::filesystem::file_size(expected_path)),
            written.value().size_bytes,
            "snapshot file size should match write result");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_write_to_custom_path()
  {
    const auto root = make_test_root();

    auto item = make_snapshot();
    const auto path = root / "custom" / "manual.snapshot";

    snapshot::SnapshotWriter writer{root / "unused.snapshot"};

    auto written = writer.write_to(item, path);

    if (!expect_true(written.is_ok(), "snapshot write_to should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::filesystem::path>(
            written.value().path,
            path,
            "write_to should use explicit path") &&
        expect_true(
            std::filesystem::exists(path),
            "write_to should create custom snapshot file") &&
        expect_true(
            !std::filesystem::exists(path.string() + ".tmp"),
            "write_to should not leave temporary file behind");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_from_written_file()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    auto item = make_snapshot();

    snapshot::SnapshotWriter writer{config};

    auto written = writer.write(item);

    if (!expect_true(written.is_ok(), "snapshot write should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    snapshot::SnapshotReader reader{config};

    auto read = reader.read_snapshot(item.id);

    if (!expect_true(read.is_ok(), "reader.read_snapshot should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            read.value().id,
            item.id,
            "read snapshot id should match") &&
        expect_eq<std::uint64_t>(
            read.value().last_sequence,
            item.last_sequence,
            "read snapshot last_sequence should match") &&
        expect_eq<std::size_t>(
            read.value().entries.size(),
            item.entries.size(),
            "read snapshot entry count should match");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_read_from_explicit_path()
  {
    const auto root = make_test_root();

    auto item = make_snapshot();
    const auto path = root / "snapshots" / "explicit.snapshot";

    snapshot::SnapshotWriter writer{path};

    auto written = writer.write(item);

    if (!expect_true(written.is_ok(), "snapshot explicit write should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    snapshot::SnapshotReader reader{path};

    auto read = reader.read();

    if (!expect_true(read.is_ok(), "reader.read should read explicit path"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            read.value().id,
            item.id,
            "explicit reader id should match") &&
        expect_eq<std::size_t>(
            read.value().entries.size(),
            item.entries.size(),
            "explicit reader entries should match");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_missing_file_is_not_found()
  {
    const auto root = make_test_root();

    const auto path = root / "missing.snapshot";

    snapshot::SnapshotReader reader{path};

    auto result = reader.read();

    const bool ok =
        expect_true(
            result.is_err(),
            "reading missing snapshot should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotFound,
            "missing snapshot should return NotFound");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_reader_empty_path_is_rejected()
  {
    snapshot::SnapshotReader reader{std::filesystem::path{}};

    auto result = reader.read();

    return expect_true(
               result.is_err(),
               "reading empty snapshot path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::StorageError,
               "empty snapshot path should return StorageError");
  }

  bool test_read_snapshot_zero_id_is_rejected()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);
    snapshot::SnapshotReader reader{config};

    auto result = reader.read_snapshot(0);

    const bool ok =
        expect_true(
            result.is_err(),
            "read_snapshot with id 0 should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "read_snapshot id 0 should return InvalidArgument");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_decode_too_small_buffer_is_corruption()
  {
    const std::vector<std::uint8_t> data{1, 2, 3};

    auto result = snapshot::SnapshotReader::decode(data);

    return expect_true(
               result.is_err(),
               "decoding too small snapshot should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::Corruption,
               "too small snapshot should return Corruption");
  }

  bool test_decode_bad_checksum_is_rejected()
  {
    auto encoded = snapshot::SnapshotWriter::encode(make_snapshot());

    if (!expect_true(encoded.is_ok(), "snapshot encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();
    data.back() ^= 0x01U;

    auto result = snapshot::SnapshotReader::decode(data);

    return expect_true(
               result.is_err(),
               "snapshot with bad checksum should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::ChecksumMismatch,
               "bad snapshot checksum should return ChecksumMismatch");
  }

  bool test_decode_truncated_payload_is_rejected()
  {
    auto encoded = snapshot::SnapshotWriter::encode(make_snapshot());

    if (!expect_true(encoded.is_ok(), "snapshot encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();

    if (!data.empty())
    {
      data.erase(data.begin() + static_cast<std::ptrdiff_t>(data.size() / 2));
    }

    auto result = snapshot::SnapshotReader::decode(data);

    return expect_true(
        result.is_err(),
        "truncated snapshot payload should fail");
  }

  bool test_decode_trailing_bytes_is_rejected()
  {
    auto encoded = snapshot::SnapshotWriter::encode(make_snapshot());

    if (!expect_true(encoded.is_ok(), "snapshot encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();

    const auto checksum = std::vector<std::uint8_t>(
        data.end() - static_cast<std::ptrdiff_t>(sizeof(std::uint32_t)),
        data.end());

    data.erase(
        data.end() - static_cast<std::ptrdiff_t>(sizeof(std::uint32_t)),
        data.end());

    data.push_back(0xFFU);
    data.insert(data.end(), checksum.begin(), checksum.end());

    auto result = snapshot::SnapshotReader::decode(data);

    return expect_true(
        result.is_err(),
        "snapshot with trailing bytes should fail");
  }

  bool test_reader_directory_path_is_io_error()
  {
    const auto root = make_test_root();

    snapshot::SnapshotReader reader{root};

    auto result = reader.read();

    const bool ok =
        expect_true(
            result.is_err(),
            "reading directory as snapshot should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::IoError,
            "directory snapshot path should return IoError");

    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_default_snapshot_is_invalid())
  {
    return 1;
  }

  if (!test_make_computes_last_sequence())
  {
    return 1;
  }

  if (!test_counts_and_byte_size())
  {
    return 1;
  }

  if (!test_add_updates_last_sequence())
  {
    return 1;
  }

  if (!test_refresh_last_sequence())
  {
    return 1;
  }

  if (!test_clear_resets_snapshot())
  {
    return 1;
  }

  if (!test_writer_validate_accepts_valid_snapshot())
  {
    return 1;
  }

  if (!test_writer_validate_rejects_zero_id())
  {
    return 1;
  }

  if (!test_writer_validate_rejects_invalid_entry())
  {
    return 1;
  }

  if (!test_encode_decode_roundtrip())
  {
    return 1;
  }

  if (!test_write_uses_layout_path())
  {
    return 1;
  }

  if (!test_write_to_custom_path())
  {
    return 1;
  }

  if (!test_reader_read_from_written_file())
  {
    return 1;
  }

  if (!test_reader_read_from_explicit_path())
  {
    return 1;
  }

  if (!test_reader_missing_file_is_not_found())
  {
    return 1;
  }

  if (!test_reader_empty_path_is_rejected())
  {
    return 1;
  }

  if (!test_read_snapshot_zero_id_is_rejected())
  {
    return 1;
  }

  if (!test_decode_too_small_buffer_is_corruption())
  {
    return 1;
  }

  if (!test_decode_bad_checksum_is_rejected())
  {
    return 1;
  }

  if (!test_decode_truncated_payload_is_rejected())
  {
    return 1;
  }

  if (!test_decode_trailing_bytes_is_rejected())
  {
    return 1;
  }

  if (!test_reader_directory_path_is_io_error())
  {
    return 1;
  }

  std::cout << "kv_test_snapshot passed\n";
  return 0;
}
