/**
 *
 *  @file KvRecordEncoder.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KV record encoder implementation
 *
 */

#include <vix/kv/records/KvRecordEncoder.hpp>
#include <vix/kv/checksum/Crc32.hpp>
#include <vix/kv/utils/Endian.hpp>

namespace vix::kv::records
{
  namespace checksum = vix::kv::checksum;
  namespace utils = vix::kv::utils;

  core::KvResult<KvRecordEncoder::Bytes>
  KvRecordEncoder::encode(KvRecord record)
  {
    record.refresh_sizes();

    auto validation = validate(record);

    if (validation.is_err())
    {
      return core::KvResult<Bytes>::err(validation.error());
    }

    auto payload_checksum = compute_payload_checksum(record);

    if (payload_checksum.is_err())
    {
      return core::KvResult<Bytes>::err(payload_checksum.error());
    }

    record.header.payload_checksum = payload_checksum.value();
    record.header.header_checksum = 0;

    auto header_checksum = compute_header_checksum(record.header);

    if (header_checksum.is_err())
    {
      return core::KvResult<Bytes>::err(header_checksum.error());
    }

    record.header.header_checksum = header_checksum.value();

    Bytes out;
    out.reserve(static_cast<std::size_t>(record.total_size()));

    append_header(out, record.header);
    append_payload(out, record);

    return core::KvResult<Bytes>::ok(std::move(out));
  }

  core::KvResult<KvRecordEncoder::Bytes>
  KvRecordEncoder::encode_header(const KvRecordHeader &header)
  {
    if (!header.has_valid_magic())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "record header magic is invalid"));
    }

    if (!header.has_supported_version())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "record header version is not supported"));
    }

    if (!header.has_valid_header_size())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "record header size is invalid"));
    }

    Bytes out;
    out.reserve(KvRecordHeader::encoded_size);
    append_header(out, header);

    return core::KvResult<Bytes>::ok(std::move(out));
  }

  core::KvResult<KvRecordEncoder::Bytes>
  KvRecordEncoder::encode_header_for_checksum(
      const KvRecordHeader &header)
  {
    KvRecordHeader copy = header;
    copy.header_checksum = 0;

    return encode_header(copy);
  }

  core::KvResult<KvRecordEncoder::Bytes>
  KvRecordEncoder::encode_payload(const KvRecord &record)
  {
    if (!record.has_key())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::invalid_key(
              "record key must not be empty"));
    }

    if (!record.has_valid_payload_sizes())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::invalid_argument(
              "record payload size is invalid"));
    }

    Bytes out;
    out.reserve(static_cast<std::size_t>(record.payload_size()));
    append_payload(out, record);

    return core::KvResult<Bytes>::ok(std::move(out));
  }

  core::KvResult<std::uint32_t>
  KvRecordEncoder::compute_header_checksum(
      const KvRecordHeader &header)
  {
    auto encoded = encode_header_for_checksum(header);

    if (encoded.is_err())
    {
      return core::KvResult<std::uint32_t>::err(encoded.error());
    }

    return core::KvResult<std::uint32_t>::ok(
        checksum::Crc32::compute(encoded.value()));
  }

  core::KvResult<std::uint32_t>
  KvRecordEncoder::compute_payload_checksum(
      const KvRecord &record)
  {
    auto payload = encode_payload(record);

    if (payload.is_err())
    {
      return core::KvResult<std::uint32_t>::err(payload.error());
    }

    return core::KvResult<std::uint32_t>::ok(
        checksum::Crc32::compute(payload.value()));
  }

  core::KvResult<void>
  KvRecordEncoder::validate(const KvRecord &record)
  {
    if (!record.header.has_valid_magic())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header magic is invalid"));
    }

    if (!record.header.has_supported_version())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header version is not supported"));
    }

    if (!record.header.has_valid_header_size())
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "record header size is invalid"));
    }

    if (!record.header.has_sequence())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "record sequence must be greater than zero"));
    }

    if (!record.has_key())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_key(
              "record key must not be empty"));
    }

    if (!record.has_consistent_sizes())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "record header sizes do not match payload sizes"));
    }

    if (!record.has_valid_payload_sizes())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "record payload size is invalid"));
    }

    if (!record.has_valid_type_shape())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "record type shape is invalid"));
    }

    if (!record.header.is_valid())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "record header is invalid"));
    }

    return core::KvResult<void>::ok();
  }

  void KvRecordEncoder::append_header(
      Bytes &out,
      const KvRecordHeader &header)
  {
    utils::Endian::append_u32(out, header.magic);
    utils::Endian::append_u8(out, header.format_version);

    utils::Endian::append_u8(
        out,
        static_cast<std::uint8_t>(header.type));

    utils::Endian::append_u16(out, header.header_size);
    utils::Endian::append_u64(out, header.sequence);
    utils::Endian::append_u64(out, header.timestamp_ms);
    utils::Endian::append_u32(out, header.key_size);
    utils::Endian::append_u64(out, header.value_size);
    utils::Endian::append_u32(out, header.header_checksum);
    utils::Endian::append_u32(out, header.payload_checksum);
  }

  void KvRecordEncoder::append_payload(
      Bytes &out,
      const KvRecord &record)
  {
    out.insert(
        out.end(),
        record.key.begin(),
        record.key.end());

    out.insert(
        out.end(),
        record.value.begin(),
        record.value.end());
  }

} // namespace vix::kv::records
