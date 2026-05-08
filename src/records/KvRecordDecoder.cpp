/**
 *
 *  @file KvRecordDecoder.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KV record decoder implementation
 *
 */

#include <vix/kv/records/KvRecordDecoder.hpp>

#include <cstddef>
#include <utility>

#include <vix/kv/checksum/Crc32.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/utils/Endian.hpp>

namespace vix::kv::records
{
  namespace checksum = vix::kv::checksum;
  namespace utils = vix::kv::utils;

  core::KvResult<KvRecord>
  KvRecordDecoder::decode(const Bytes &bytes)
  {
    return decode(
        std::span<const std::uint8_t>(
            bytes.data(),
            bytes.size()));
  }

  core::KvResult<KvRecord>
  KvRecordDecoder::decode(std::span<const std::uint8_t> bytes)
  {
    std::size_t offset = 0;

    auto header = decode_header(bytes, offset);

    if (header.is_err())
    {
      return core::KvResult<KvRecord>::err(header.error());
    }

    auto header_validation = validate_header(header.value());

    if (header_validation.is_err())
    {
      return core::KvResult<KvRecord>::err(header_validation.error());
    }

    auto header_checksum = verify_header_checksum(header.value());

    if (header_checksum.is_err())
    {
      return core::KvResult<KvRecord>::err(header_checksum.error());
    }

    auto key = decode_key(
        bytes,
        offset,
        header.value());

    if (key.is_err())
    {
      return core::KvResult<KvRecord>::err(key.error());
    }

    auto value = decode_value(
        bytes,
        offset,
        header.value());

    if (value.is_err())
    {
      return core::KvResult<KvRecord>::err(value.error());
    }

    if (offset != bytes.size())
    {
      return core::KvResult<KvRecord>::err(
          core::KvError::corruption(
              "record contains trailing bytes"));
    }

    KvRecord record{
        header.move_value(),
        key.move_value(),
        value.move_value()};

    auto record_validation = validate_record(record);

    if (record_validation.is_err())
    {
      return core::KvResult<KvRecord>::err(
          record_validation.error());
    }

    return core::KvResult<KvRecord>::ok(std::move(record));
  }

  core::KvResult<KvRecordHeader>
  KvRecordDecoder::decode_header(
      std::span<const std::uint8_t> bytes,
      std::size_t &offset)
  {
    if (!can_read(bytes, offset, KvRecordHeader::encoded_size))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "record header is truncated"));
    }

    KvRecordHeader header;

    if (!utils::Endian::read_u32(bytes, offset, header.magic))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record magic"));
    }

    if (!utils::Endian::read_u8(bytes, offset, header.format_version))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record format version"));
    }

    std::uint8_t raw_type = 0;

    if (!utils::Endian::read_u8(bytes, offset, raw_type))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record type"));
    }

    header.type = to_record_type(raw_type);

    if (!utils::Endian::read_u16(bytes, offset, header.header_size))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record header size"));
    }

    if (!utils::Endian::read_u64(bytes, offset, header.sequence))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record sequence"));
    }

    if (!utils::Endian::read_u64(bytes, offset, header.timestamp_ms))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record timestamp"));
    }

    if (!utils::Endian::read_u32(bytes, offset, header.key_size))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record key size"));
    }

    if (!utils::Endian::read_u64(bytes, offset, header.value_size))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record value size"));
    }

    if (!utils::Endian::read_u32(bytes, offset, header.header_checksum))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record header checksum"));
    }

    if (!utils::Endian::read_u32(bytes, offset, header.payload_checksum))
    {
      return core::KvResult<KvRecordHeader>::err(
          core::KvError::corruption(
              "failed to read record payload checksum"));
    }

    return core::KvResult<KvRecordHeader>::ok(header);
  }

  core::KvResult<std::string>
  KvRecordDecoder::decode_key(
      std::span<const std::uint8_t> bytes,
      std::size_t &offset,
      const KvRecordHeader &header)
  {
    const std::size_t key_size =
        static_cast<std::size_t>(header.key_size);

    if (!can_read(bytes, offset, key_size))
    {
      return core::KvResult<std::string>::err(
          core::KvError::corruption(
              "record key is truncated"));
    }

    std::string key(
        reinterpret_cast<const char *>(bytes.data() + offset),
        key_size);

    offset += key_size;

    return core::KvResult<std::string>::ok(std::move(key));
  }

  core::KvResult<KvRecordDecoder::Bytes>
  KvRecordDecoder::decode_value(
      std::span<const std::uint8_t> bytes,
      std::size_t &offset,
      const KvRecordHeader &header)
  {
    if (header.value_size >
        static_cast<std::uint64_t>(
            static_cast<std::size_t>(-1)))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "record value size cannot fit in memory size"));
    }

    const std::size_t value_size =
        static_cast<std::size_t>(header.value_size);

    if (!can_read(bytes, offset, value_size))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "record value is truncated"));
    }

    Bytes value(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes.begin() + static_cast<std::ptrdiff_t>(offset + value_size));

    offset += value_size;

    return core::KvResult<Bytes>::ok(std::move(value));
  }

  core::KvResult<void>
  KvRecordDecoder::validate_header(const KvRecordHeader &header)
  {
    if (!header.has_valid_magic())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header magic is invalid"));
    }

    if (!header.has_supported_version())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header version is not supported"));
    }

    if (!header.has_valid_header_size())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header size is invalid"));
    }

    if (!header.has_sequence())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record sequence must be greater than zero"));
    }

    if (!header.has_valid_sizes())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record payload size is invalid"));
    }

    if (!header.has_valid_type_shape())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record type shape is invalid"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  KvRecordDecoder::verify_header_checksum(
      const KvRecordHeader &header)
  {
    auto expected = KvRecordEncoder::compute_header_checksum(header);

    if (expected.is_err())
    {
      return core::KvResult<void>::err(expected.error());
    }

    if (expected.value() != header.header_checksum)
    {
      return core::KvResult<void>::err(
          core::KvError::checksum_mismatch(
              "record header checksum mismatch"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  KvRecordDecoder::verify_payload_checksum(const KvRecord &record)
  {
    auto expected = KvRecordEncoder::compute_payload_checksum(record);

    if (expected.is_err())
    {
      return core::KvResult<void>::err(expected.error());
    }

    if (expected.value() != record.header.payload_checksum)
    {
      return core::KvResult<void>::err(
          core::KvError::checksum_mismatch(
              "record payload checksum mismatch"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  KvRecordDecoder::validate_record(const KvRecord &record)
  {
    auto header_validation = validate_header(record.header);

    if (header_validation.is_err())
    {
      return header_validation;
    }

    if (!record.is_valid())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "decoded record is invalid"));
    }

    auto payload_checksum = verify_payload_checksum(record);

    if (payload_checksum.is_err())
    {
      return payload_checksum;
    }

    return core::KvResult<void>::ok();
  }

  KvRecordType KvRecordDecoder::to_record_type(
      std::uint8_t value) noexcept
  {
    switch (value)
    {
    case static_cast<std::uint8_t>(KvRecordType::Put):
      return KvRecordType::Put;

    case static_cast<std::uint8_t>(KvRecordType::Delete):
      return KvRecordType::Delete;

    case static_cast<std::uint8_t>(KvRecordType::Snapshot):
      return KvRecordType::Snapshot;

    case static_cast<std::uint8_t>(KvRecordType::Compaction):
      return KvRecordType::Compaction;

    default:
      return KvRecordType::Unknown;
    }
  }

  bool KvRecordDecoder::can_read(
      std::span<const std::uint8_t> bytes,
      std::size_t offset,
      std::size_t count) noexcept
  {
    return offset <= bytes.size() &&
           count <= bytes.size() - offset;
  }

} // namespace vix::kv::records
