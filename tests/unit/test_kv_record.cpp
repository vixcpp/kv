/**
 *
 *  @file test_kv_record.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KvRecord unit tests
 *
 */

#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>
#include <vix/kv/records/KvRecordType.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <type_traits>
#include <utility>

namespace
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

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
  void print_value(const T &value)
  {
    if constexpr (std::is_enum_v<T>)
    {
      using Underlying = std::underlying_type_t<T>;

      std::cerr << static_cast<Underlying>(value);
    }
    else
    {
      std::cerr << value;
    }
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

      std::cerr << "  expected: ";
      print_value(expected);
      std::cerr << '\n';

      std::cerr << "  actual  : ";
      print_value(actual);
      std::cerr << '\n';

      return false;
    }

    return true;
  }

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  bool test_default_record_is_invalid()
  {
    const records::KvRecord record;

    return expect_true(
               !record.is_valid(),
               "default record should be invalid") &&
           expect_true(
               !record.has_key(),
               "default record should not have a key") &&
           expect_true(
               !record.has_value(),
               "default record should not have a value") &&
           expect_eq<records::KvRecordType>(
               record.header.type,
               records::KvRecordType::Unknown,
               "default record type should be Unknown");
  }

  bool test_put_record_shape()
  {
    auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        123);

    return expect_true(
               record.is_put(),
               "put record should report is_put") &&
           expect_true(
               !record.is_delete(),
               "put record should not report is_delete") &&
           expect_true(
               record.has_key(),
               "put record should have a key") &&
           expect_true(
               record.has_value(),
               "put record should have a value") &&
           expect_eq<std::string>(
               record.key,
               "v1|5:hello",
               "put record key should match") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               1,
               "put record sequence should match") &&
           expect_eq<std::uint64_t>(
               record.header.timestamp_ms,
               123,
               "put record timestamp should match") &&
           expect_eq<std::uint32_t>(
               record.header.key_size,
               static_cast<std::uint32_t>(record.key.size()),
               "put record header key_size should match key size") &&
           expect_eq<std::uint64_t>(
               record.header.value_size,
               static_cast<std::uint64_t>(record.value.size()),
               "put record header value_size should match value size");
  }

  bool test_put_record_is_valid()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    return expect_true(
        record.is_valid(),
        "put record with key, value, and sequence should be valid");
  }

  bool test_delete_record_shape()
  {
    auto record = records::KvRecord::remove(
        "v1|5:hello",
        2,
        456);

    return expect_true(
               record.is_delete(),
               "delete record should report is_delete") &&
           expect_true(
               !record.is_put(),
               "delete record should not report is_put") &&
           expect_true(
               record.has_key(),
               "delete record should have a key") &&
           expect_true(
               !record.has_value(),
               "delete record should not have a value") &&
           expect_true(
               record.value.empty(),
               "delete record value should be empty") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               2,
               "delete record sequence should match") &&
           expect_eq<std::uint64_t>(
               record.header.timestamp_ms,
               456,
               "delete record timestamp should match") &&
           expect_eq<std::uint32_t>(
               record.header.key_size,
               static_cast<std::uint32_t>(record.key.size()),
               "delete record header key_size should match key size") &&
           expect_eq<std::uint64_t>(
               record.header.value_size,
               0,
               "delete record header value_size should be 0");
  }

  bool test_delete_record_is_valid()
  {
    const auto record = records::KvRecord::remove(
        "v1|5:hello",
        2,
        0);

    return expect_true(
        record.is_valid(),
        "delete record with key and sequence should be valid");
  }

  bool test_snapshot_record_shape()
  {
    auto record = records::KvRecord::snapshot(
        "v1|8:snapshot",
        bytes("payload"),
        3,
        789);

    return expect_true(
               record.is_snapshot(),
               "snapshot record should report is_snapshot") &&
           expect_true(
               !record.is_put(),
               "snapshot record should not report is_put") &&
           expect_true(
               record.has_key(),
               "snapshot record should have a key") &&
           expect_true(
               record.has_value(),
               "snapshot record should have a value") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               3,
               "snapshot record sequence should match") &&
           expect_eq<std::uint64_t>(
               record.header.timestamp_ms,
               789,
               "snapshot record timestamp should match");
  }

  bool test_snapshot_record_is_valid()
  {
    const auto record = records::KvRecord::snapshot(
        "v1|8:snapshot",
        bytes("payload"),
        3,
        0);

    return expect_true(
        record.is_valid(),
        "snapshot record with key, value, and sequence should be valid");
  }

  bool test_compaction_record_shape()
  {
    auto record = records::KvRecord::compaction(
        "v1|10:compaction",
        4,
        987);

    return expect_true(
               record.is_compaction(),
               "compaction record should report is_compaction") &&
           expect_true(
               !record.is_put(),
               "compaction record should not report is_put") &&
           expect_true(
               record.has_key(),
               "compaction record should have a key") &&
           expect_true(
               !record.has_value(),
               "compaction record should not have a value") &&
           expect_true(
               record.value.empty(),
               "compaction record value should be empty") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               4,
               "compaction record sequence should match") &&
           expect_eq<std::uint64_t>(
               record.header.timestamp_ms,
               987,
               "compaction record timestamp should match");
  }

  bool test_compaction_record_is_valid()
  {
    const auto record = records::KvRecord::compaction(
        "v1|10:compaction",
        4,
        0);

    return expect_true(
        record.is_valid(),
        "compaction record with key and sequence should be valid");
  }

  bool test_payload_size()
  {
    const auto record = records::KvRecord::put(
        "abc",
        bytes("defg"),
        1,
        0);

    return expect_eq<std::uint64_t>(
        record.payload_size(),
        7,
        "payload_size should be key size plus value size");
  }

  bool test_total_size()
  {
    const auto record = records::KvRecord::put(
        "abc",
        bytes("defg"),
        1,
        0);

    return expect_eq<std::uint64_t>(
        record.total_size(),
        static_cast<std::uint64_t>(
            records::KvRecordHeader::encoded_size) +
            record.payload_size(),
        "total_size should be header size plus payload size");
  }

  bool test_refresh_sizes()
  {
    records::KvRecord record;

    record.header = records::KvRecordHeader::put(
        1,
        0,
        0,
        0);

    record.key = "v1|5:hello";
    record.value = bytes("world");

    record.refresh_sizes();

    return expect_eq<std::uint32_t>(
               record.header.key_size,
               static_cast<std::uint32_t>(record.key.size()),
               "refresh_sizes should update key_size") &&
           expect_eq<std::uint64_t>(
               record.header.value_size,
               static_cast<std::uint64_t>(record.value.size()),
               "refresh_sizes should update value_size") &&
           expect_eq<std::uint16_t>(
               record.header.header_size,
               records::KvRecordHeader::encoded_size,
               "refresh_sizes should update header_size");
  }

  bool test_consistent_sizes()
  {
    auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    const bool initially_consistent = record.has_consistent_sizes();

    record.header.key_size += 1;

    return expect_true(
               initially_consistent,
               "new record should have consistent sizes") &&
           expect_true(
               !record.has_consistent_sizes(),
               "modified header size should make record inconsistent");
  }

  bool test_valid_payload_sizes_reject_empty_key()
  {
    const auto record = records::KvRecord::put(
        "",
        bytes("world"),
        1,
        0);

    return expect_true(
        !record.has_valid_payload_sizes(),
        "record with empty key should have invalid payload sizes");
  }

  bool test_delete_record_with_value_is_invalid()
  {
    auto record = records::KvRecord::remove(
        "v1|5:hello",
        2,
        0);

    record.value = bytes("should-not-exist");
    record.refresh_sizes();

    return expect_true(
               record.is_delete(),
               "record should still be delete type") &&
           expect_true(
               !record.has_valid_type_shape(),
               "delete record with value should have invalid type shape") &&
           expect_true(
               !record.is_valid(),
               "delete record with value should be invalid");
  }

  bool test_zero_sequence_is_invalid()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        0,
        0);

    return expect_true(
        !record.is_valid(),
        "record with sequence 0 should be invalid");
  }

  bool test_clear_resets_record()
  {
    auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    record.clear();

    return expect_true(
               !record.is_valid(),
               "cleared record should be invalid") &&
           expect_true(
               record.key.empty(),
               "cleared record key should be empty") &&
           expect_true(
               record.value.empty(),
               "cleared record value should be empty") &&
           expect_eq<records::KvRecordType>(
               record.header.type,
               records::KvRecordType::Unknown,
               "cleared record type should be Unknown") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               0,
               "cleared record sequence should be 0");
  }

  bool test_large_key_is_invalid()
  {
    const std::string large_key(
        core::KvLimits::max_key_size + 1U,
        'x');

    const auto record = records::KvRecord::put(
        large_key,
        bytes("value"),
        1,
        0);

    return expect_true(
        !record.has_valid_payload_sizes(),
        "record with key larger than max_key_size should be invalid");
  }

  bool test_large_value_is_invalid()
  {
    const std::vector<std::uint8_t> large_value(
        core::KvLimits::max_value_size + 1U,
        static_cast<std::uint8_t>('x'));

    const auto record = records::KvRecord::put(
        "v1|5:hello",
        large_value,
        1,
        0);

    return expect_true(
        !record.has_valid_payload_sizes(),
        "record with value larger than max_value_size should be invalid");
  }

  bool test_explicit_constructor_refreshes_sizes()
  {
    records::KvRecordHeader header = records::KvRecordHeader::put(
        10,
        20,
        0,
        0);

    records::KvRecord record{
        header,
        "v1|5:hello",
        bytes("world")};

    return expect_eq<std::uint32_t>(
               record.header.key_size,
               static_cast<std::uint32_t>(record.key.size()),
               "explicit constructor should refresh key_size") &&
           expect_eq<std::uint64_t>(
               record.header.value_size,
               static_cast<std::uint64_t>(record.value.size()),
               "explicit constructor should refresh value_size") &&
           expect_true(
               record.is_valid(),
               "explicit constructor should create valid record when fields are valid");
  }
}

int main()
{
  if (!test_default_record_is_invalid())
  {
    return 1;
  }

  if (!test_put_record_shape())
  {
    return 1;
  }

  if (!test_put_record_is_valid())
  {
    return 1;
  }

  if (!test_delete_record_shape())
  {
    return 1;
  }

  if (!test_delete_record_is_valid())
  {
    return 1;
  }

  if (!test_snapshot_record_shape())
  {
    return 1;
  }

  if (!test_snapshot_record_is_valid())
  {
    return 1;
  }

  if (!test_compaction_record_shape())
  {
    return 1;
  }

  if (!test_compaction_record_is_valid())
  {
    return 1;
  }

  if (!test_payload_size())
  {
    return 1;
  }

  if (!test_total_size())
  {
    return 1;
  }

  if (!test_refresh_sizes())
  {
    return 1;
  }

  if (!test_consistent_sizes())
  {
    return 1;
  }

  if (!test_valid_payload_sizes_reject_empty_key())
  {
    return 1;
  }

  if (!test_delete_record_with_value_is_invalid())
  {
    return 1;
  }

  if (!test_zero_sequence_is_invalid())
  {
    return 1;
  }

  if (!test_clear_resets_record())
  {
    return 1;
  }

  if (!test_large_key_is_invalid())
  {
    return 1;
  }

  if (!test_large_value_is_invalid())
  {
    return 1;
  }

  if (!test_explicit_constructor_refreshes_sizes())
  {
    return 1;
  }

  std::cout << "kv_test_kv_record passed\n";
  return 0;
}
