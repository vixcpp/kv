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

#include <cstdint>
#include <span>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>

namespace vix::kv::records
{
  namespace core = vix::kv::core;

  /**
   * @brief Decodes stable binary KV records.
   *
   * Binary format:
   *
   * @code
   * header
   * key bytes
   * value bytes
   * @endcode
   *
   * All integers are decoded as little-endian.
   *
   * Decoder checks:
   * - record magic
   * - format version
   * - header size
   * - record type
   * - payload sizes
   * - header checksum
   * - payload checksum
   * - full buffer consumption
   */
  class KvRecordDecoder
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Decodes a complete record from a byte vector.
     *
     * @param bytes Encoded record bytes.
     * @return KvRecord or KvError.
     */
    [[nodiscard]] static core::KvResult<KvRecord>
    decode(const Bytes &bytes);

    /**
     * @brief Decodes a complete record from a byte span.
     *
     * @param bytes Encoded record bytes.
     * @return KvRecord or KvError.
     */
    [[nodiscard]] static core::KvResult<KvRecord>
    decode(std::span<const std::uint8_t> bytes);

    /**
     * @brief Decodes a record header from a byte span.
     *
     * @param bytes Encoded bytes.
     * @param offset Current offset, advanced on success.
     * @return KvRecordHeader or KvError.
     */
    [[nodiscard]] static core::KvResult<KvRecordHeader>
    decode_header(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset);

    /**
     * @brief Decodes the key payload using header sizes.
     *
     * @param bytes Encoded bytes.
     * @param offset Current offset, advanced on success.
     * @param header Decoded header.
     * @return Encoded key string or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string>
    decode_key(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        const KvRecordHeader &header);

    /**
     * @brief Decodes the value payload using header sizes.
     *
     * @param bytes Encoded bytes.
     * @param offset Current offset, advanced on success.
     * @param header Decoded header.
     * @return Value bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    decode_value(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        const KvRecordHeader &header);

    /**
     * @brief Validates a decoded header structure.
     *
     * This does not verify checksums.
     *
     * @param header Header.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_header(const KvRecordHeader &header);

    /**
     * @brief Verifies header checksum.
     *
     * @param header Header.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    verify_header_checksum(const KvRecordHeader &header);

    /**
     * @brief Verifies payload checksum.
     *
     * @param record Record.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    verify_payload_checksum(const KvRecord &record);

    /**
     * @brief Validates a decoded record structure and checksums.
     *
     * @param record Record.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_record(const KvRecord &record);

  private:
    /**
     * @brief Converts a raw record type byte to KvRecordType.
     *
     * @param value Raw type value.
     * @return Record type.
     */
    [[nodiscard]] static KvRecordType to_record_type(
        std::uint8_t value) noexcept;

    /**
     * @brief Returns true if count bytes can be read safely.
     *
     * @param bytes Input bytes.
     * @param offset Current offset.
     * @param count Number of bytes.
     * @return true when the read is safe.
     */
    [[nodiscard]] static bool can_read(
        std::span<const std::uint8_t> bytes,
        std::size_t offset,
        std::size_t count) noexcept;
  };

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_DECODER_HPP
