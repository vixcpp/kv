/**
 *
 *  @file KvRecordDecoder.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KV record decoder
 *
 */

#ifndef VIX_KV_RECORDS_KV_RECORD_DECODER_HPP
#define VIX_KV_RECORDS_KV_RECORD_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>
#include <vix/kv/records/KvRecordType.hpp>

namespace vix::kv::records
{
  namespace core = vix::kv::core;

  class KvRecordDecoder
  {
  public:
    using Bytes = std::vector<std::uint8_t>;

    [[nodiscard]] static core::KvResult<KvRecord>
    decode(const Bytes &bytes);

    [[nodiscard]] static core::KvResult<KvRecord>
    decode(std::span<const std::uint8_t> bytes);

    [[nodiscard]] static core::KvResult<KvRecordHeader>
    decode_header(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset);

    [[nodiscard]] static core::KvResult<std::string>
    decode_key(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        const KvRecordHeader &header);

    [[nodiscard]] static core::KvResult<Bytes>
    decode_value(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        const KvRecordHeader &header);

    [[nodiscard]] static core::KvResult<void>
    validate_header(const KvRecordHeader &header);

    [[nodiscard]] static core::KvResult<void>
    verify_header_checksum(const KvRecordHeader &header);

    [[nodiscard]] static core::KvResult<void>
    verify_payload_checksum(const KvRecord &record);

    [[nodiscard]] static core::KvResult<void>
    validate_record(const KvRecord &record);

  private:
    [[nodiscard]] static KvRecordType to_record_type(
        std::uint8_t value) noexcept;

    [[nodiscard]] static bool can_read(
        std::span<const std::uint8_t> bytes,
        std::size_t offset,
        std::size_t count) noexcept;
  };

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_DECODER_HPP
