/**
 *
 *  @file test_kv_record_codec.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KvRecord encoder/decoder unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordDecoder.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>
#include <vix/kv/records/KvRecordType.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

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

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  bool same_record(
      const records::KvRecord &left,
      const records::KvRecord &right)
  {
    return left.header.magic == right.header.magic &&
           left.header.format_version == right.header.format_version &&
           left.header.type == right.header.type &&
           left.header.header_size == right.header.header_size &&
           left.header.sequence == right.header.sequence &&
           left.header.timestamp_ms == right.header.timestamp_ms &&
           left.header.key_size == right.header.key_size &&
           left.header.value_size == right.header.value_size &&
           left.header.header_checksum == right.header.header_checksum &&
           left.header.payload_checksum == right.header.payload_checksum &&
           left.key == right.key &&
           left.value == right.value;
  }

  bool test_encode_put_record()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        100);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "put record should encode successfully"))
    {
      return false;
    }

    return expect_eq<std::size_t>(
        encoded.value().size(),
        static_cast<std::size_t>(record.total_size()),
        "encoded put record size should match total_size");
  }

  bool test_decode_put_record()
  {
    const auto original = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        100);

    auto encoded = records::KvRecordEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "put record encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "put record decode should succeed"))
    {
      return false;
    }

    const auto &record = decoded.value();

    return expect_true(
               record.is_put(),
               "decoded record should be put") &&
           expect_eq<std::string>(
               record.key,
               "v1|5:hello",
               "decoded put key should match") &&
           expect_true(
               record.value == bytes("world"),
               "decoded put value should match") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               1,
               "decoded put sequence should match") &&
           expect_eq<std::uint64_t>(
               record.header.timestamp_ms,
               100,
               "decoded put timestamp should match") &&
           expect_true(
               record.is_valid(),
               "decoded put record should be valid");
  }

  bool test_encode_decode_put_roundtrip()
  {
    const auto original = records::KvRecord::put(
        "v1|7:profile",
        bytes("Ada Lovelace"),
        42,
        12345);

    auto encoded = records::KvRecordEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "roundtrip put encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "roundtrip put decode should succeed"))
    {
      return false;
    }

    auto reencoded = records::KvRecordEncoder::encode(decoded.value());

    if (!expect_true(reencoded.is_ok(), "roundtrip put re-encode should succeed"))
    {
      return false;
    }

    return expect_true(
        encoded.value() == reencoded.value(),
        "encoding decoded put record should reproduce identical bytes");
  }

  bool test_encode_decode_delete_roundtrip()
  {
    const auto original = records::KvRecord::remove(
        "v1|5:hello",
        2,
        200);

    auto encoded = records::KvRecordEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "delete record encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "delete record decode should succeed"))
    {
      return false;
    }

    const auto &record = decoded.value();

    return expect_true(
               record.is_delete(),
               "decoded record should be delete") &&
           expect_eq<std::string>(
               record.key,
               "v1|5:hello",
               "decoded delete key should match") &&
           expect_true(
               record.value.empty(),
               "decoded delete value should be empty") &&
           expect_eq<std::uint64_t>(
               record.header.sequence,
               2,
               "decoded delete sequence should match") &&
           expect_true(
               record.is_valid(),
               "decoded delete record should be valid");
  }

  bool test_encode_decode_snapshot_roundtrip()
  {
    const auto original = records::KvRecord::snapshot(
        "v1|8:snapshot",
        bytes("snapshot-payload"),
        3,
        300);

    auto encoded = records::KvRecordEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "snapshot record encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "snapshot record decode should succeed"))
    {
      return false;
    }

    const auto &record = decoded.value();

    return expect_true(
               record.is_snapshot(),
               "decoded record should be snapshot") &&
           expect_eq<std::string>(
               record.key,
               "v1|8:snapshot",
               "decoded snapshot key should match") &&
           expect_true(
               record.value == bytes("snapshot-payload"),
               "decoded snapshot value should match") &&
           expect_true(
               record.is_valid(),
               "decoded snapshot record should be valid");
  }

  bool test_encode_decode_compaction_roundtrip()
  {
    const auto original = records::KvRecord::compaction(
        "v1|10:compaction",
        4,
        400);

    auto encoded = records::KvRecordEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "compaction record encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "compaction record decode should succeed"))
    {
      return false;
    }

    const auto &record = decoded.value();

    return expect_true(
               record.is_compaction(),
               "decoded record should be compaction") &&
           expect_eq<std::string>(
               record.key,
               "v1|10:compaction",
               "decoded compaction key should match") &&
           expect_true(
               record.value.empty(),
               "decoded compaction value should be empty") &&
           expect_true(
               record.is_valid(),
               "decoded compaction record should be valid");
  }

  bool test_encoded_size_matches_header_and_payload()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    return expect_eq<std::size_t>(
        encoded.value().size(),
        static_cast<std::size_t>(
            records::KvRecordHeader::encoded_size +
            record.key.size() +
            record.value.size()),
        "encoded size should equal header size plus key plus value");
  }

  bool test_decode_header()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        7,
        777);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    std::size_t offset = 0;

    auto header = records::KvRecordDecoder::decode_header(
        std::span<const std::uint8_t>(
            encoded.value().data(),
            encoded.value().size()),
        offset);

    if (!expect_true(header.is_ok(), "decode_header should succeed"))
    {
      return false;
    }

    return expect_eq<std::size_t>(
               offset,
               records::KvRecordHeader::encoded_size,
               "decode_header should advance offset by encoded header size") &&
           expect_eq<records::KvRecordType>(
               header.value().type,
               records::KvRecordType::Put,
               "decoded header type should be Put") &&
           expect_eq<std::uint64_t>(
               header.value().sequence,
               7,
               "decoded header sequence should match") &&
           expect_eq<std::uint64_t>(
               header.value().timestamp_ms,
               777,
               "decoded header timestamp should match");
  }

  bool test_encode_invalid_record_is_rejected()
  {
    const records::KvRecord record;

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(
            encoded.is_err(),
            "encoding default invalid record should fail"))
    {
      return false;
    }

    return expect_error_code(
        encoded.error().code(),
        core::KvErrorCode::Corruption,
        "encoding default invalid record should return Corruption");
  }

  bool test_encode_zero_sequence_is_rejected()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        0,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(
            encoded.is_err(),
            "encoding record with zero sequence should fail"))
    {
      return false;
    }

    return expect_error_code(
        encoded.error().code(),
        core::KvErrorCode::InvalidArgument,
        "zero sequence encode should return InvalidArgument");
  }

  bool test_decode_empty_buffer_is_rejected()
  {
    const std::vector<std::uint8_t> data;

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding empty buffer should fail"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::Corruption,
        "empty decode should return Corruption");
  }

  bool test_decode_truncated_header_is_rejected()
  {
    const std::vector<std::uint8_t> data(
        records::KvRecordHeader::encoded_size - 1U,
        0);

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding truncated header should fail"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::Corruption,
        "truncated header should return Corruption");
  }

  bool test_decode_truncated_payload_is_rejected()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();

    data.pop_back();

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding truncated payload should fail"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::Corruption,
        "truncated payload should return Corruption");
  }

  bool test_decode_trailing_bytes_is_rejected()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();
    data.push_back(0xFFU);

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding record with trailing bytes should fail"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::Corruption,
        "trailing bytes should return Corruption");
  }

  bool test_decode_bad_header_checksum_is_rejected()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();

    data[0] ^= 0x01U;

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding bad header checksum should fail"))
    {
      return false;
    }

    return expect_true(
        decoded.error().code() == core::KvErrorCode::Corruption ||
            decoded.error().code() == core::KvErrorCode::ChecksumMismatch,
        "bad header bytes should return Corruption or ChecksumMismatch");
  }

  bool test_decode_bad_payload_checksum_is_rejected()
  {
    const auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    auto data = encoded.move_value();

    data.back() ^= 0x01U;

    auto decoded = records::KvRecordDecoder::decode(data);

    if (!expect_true(
            decoded.is_err(),
            "decoding bad payload checksum should fail"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::ChecksumMismatch,
        "bad payload checksum should return ChecksumMismatch");
  }

  bool test_compute_payload_checksum_is_stable()
  {
    const auto first = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    const auto second = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto first_checksum =
        records::KvRecordEncoder::compute_payload_checksum(first);

    auto second_checksum =
        records::KvRecordEncoder::compute_payload_checksum(second);

    if (!expect_true(
            first_checksum.is_ok() && second_checksum.is_ok(),
            "payload checksum computation should succeed"))
    {
      return false;
    }

    return expect_eq<std::uint32_t>(
        first_checksum.value(),
        second_checksum.value(),
        "same payload should produce same checksum");
  }

  bool test_compute_header_checksum_is_stable()
  {
    auto record = records::KvRecord::put(
        "v1|5:hello",
        bytes("world"),
        1,
        0);

    auto encoded = records::KvRecordEncoder::encode(record);

    if (!expect_true(encoded.is_ok(), "record encode should succeed"))
    {
      return false;
    }

    auto decoded = records::KvRecordDecoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "record decode should succeed"))
    {
      return false;
    }

    auto first_checksum =
        records::KvRecordEncoder::compute_header_checksum(
            decoded.value().header);

    auto second_checksum =
        records::KvRecordEncoder::compute_header_checksum(
            decoded.value().header);

    if (!expect_true(
            first_checksum.is_ok() && second_checksum.is_ok(),
            "header checksum computation should succeed"))
    {
      return false;
    }

    return expect_eq<std::uint32_t>(
        first_checksum.value(),
        second_checksum.value(),
        "same header should produce same checksum");
  }
}

int main()
{
  if (!test_encode_put_record())
  {
    return 1;
  }

  if (!test_decode_put_record())
  {
    return 1;
  }

  if (!test_encode_decode_put_roundtrip())
  {
    return 1;
  }

  if (!test_encode_decode_delete_roundtrip())
  {
    return 1;
  }

  if (!test_encode_decode_snapshot_roundtrip())
  {
    return 1;
  }

  if (!test_encode_decode_compaction_roundtrip())
  {
    return 1;
  }

  if (!test_encoded_size_matches_header_and_payload())
  {
    return 1;
  }

  if (!test_decode_header())
  {
    return 1;
  }

  if (!test_encode_invalid_record_is_rejected())
  {
    return 1;
  }

  if (!test_encode_zero_sequence_is_rejected())
  {
    return 1;
  }

  if (!test_decode_empty_buffer_is_rejected())
  {
    return 1;
  }

  if (!test_decode_truncated_header_is_rejected())
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

  if (!test_decode_bad_header_checksum_is_rejected())
  {
    return 1;
  }

  if (!test_decode_bad_payload_checksum_is_rejected())
  {
    return 1;
  }

  if (!test_compute_payload_checksum_is_stable())
  {
    return 1;
  }

  if (!test_compute_header_checksum_is_stable())
  {
    return 1;
  }

  std::cout << "kv_test_kv_record_codec passed\n";
  return 0;
}
