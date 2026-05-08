/**
 *
 *  @file KvRecordHeader.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Stable binary record header
 *
 */

#ifndef VIX_KV_RECORDS_KV_RECORD_HEADER_HPP
#define VIX_KV_RECORDS_KV_RECORD_HEADER_HPP

#include <cstdint>
#include <cstddef>

#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/records/KvRecordType.hpp>

namespace vix::kv::records
{
  namespace core = vix::kv::core;

  /**
   * @brief Binary header for one KV record.
   *
   * KvRecordHeader describes the metadata stored before each record payload.
   *
   * Binary encoding order:
   *
   * @code
   * uint32 magic
   * uint8  format_version
   * uint8  record_type
   * uint16 header_size
   * uint64 sequence
   * uint64 timestamp_ms
   * uint32 key_size
   * uint64 value_size
   * uint32 header_checksum
   * uint32 payload_checksum
   * @endcode
   *
   * Total encoded header size:
   *
   * @code
   * 44 bytes
   * @endcode
   *
   * Notes:
   * - key bytes come after the header.
   * - value bytes come after key bytes.
   * - Delete records should use value_size = 0.
   * - payload_checksum covers key bytes + value bytes.
   * - header_checksum covers the encoded header with header_checksum set to 0.
   */
  struct KvRecordHeader
  {
    /**
     * @brief Encoded header size in bytes.
     */
    static constexpr std::uint16_t encoded_size = 44;

    /**
     * @brief Record magic value.
     */
    std::uint32_t magic{core::KvLimits::record_magic};

    /**
     * @brief Record format version.
     */
    std::uint8_t format_version{core::KvLimits::record_format_version};

    /**
     * @brief Logical record type.
     */
    KvRecordType type{KvRecordType::Unknown};

    /**
     * @brief Encoded header size.
     */
    std::uint16_t header_size{encoded_size};

    /**
     * @brief Monotonic record sequence number.
     */
    std::uint64_t sequence{0};

    /**
     * @brief Record timestamp in milliseconds.
     */
    std::uint64_t timestamp_ms{0};

    /**
     * @brief Encoded key size in bytes.
     */
    std::uint32_t key_size{0};

    /**
     * @brief Value size in bytes.
     */
    std::uint64_t value_size{0};

    /**
     * @brief Header checksum.
     *
     * Computed over the encoded header with this field set to 0.
     */
    std::uint32_t header_checksum{0};

    /**
     * @brief Payload checksum.
     *
     * Computed over key bytes followed by value bytes.
     */
    std::uint32_t payload_checksum{0};

    /**
     * @brief Creates a default invalid header.
     */
    KvRecordHeader() = default;

    /**
     * @brief Creates a header with the main fields.
     *
     * @param record_type Record type.
     * @param record_sequence Sequence number.
     * @param record_timestamp_ms Timestamp in milliseconds.
     * @param record_key_size Key size.
     * @param record_value_size Value size.
     */
    KvRecordHeader(
        KvRecordType record_type,
        std::uint64_t record_sequence,
        std::uint64_t record_timestamp_ms,
        std::uint32_t record_key_size,
        std::uint64_t record_value_size)
        : type(record_type),
          sequence(record_sequence),
          timestamp_ms(record_timestamp_ms),
          key_size(record_key_size),
          value_size(record_value_size)
    {
    }

    /**
     * @brief Creates a Put header.
     *
     * @param record_sequence Sequence number.
     * @param record_timestamp_ms Timestamp in milliseconds.
     * @param record_key_size Key size.
     * @param record_value_size Value size.
     * @return Header.
     */
    [[nodiscard]] static KvRecordHeader put(
        std::uint64_t record_sequence,
        std::uint64_t record_timestamp_ms,
        std::uint32_t record_key_size,
        std::uint64_t record_value_size) noexcept
    {
      return KvRecordHeader(
          KvRecordType::Put,
          record_sequence,
          record_timestamp_ms,
          record_key_size,
          record_value_size);
    }

    /**
     * @brief Creates a Delete header.
     *
     * @param record_sequence Sequence number.
     * @param record_timestamp_ms Timestamp in milliseconds.
     * @param record_key_size Key size.
     * @return Header.
     */
    [[nodiscard]] static KvRecordHeader remove(
        std::uint64_t record_sequence,
        std::uint64_t record_timestamp_ms,
        std::uint32_t record_key_size) noexcept
    {
      return KvRecordHeader(
          KvRecordType::Delete,
          record_sequence,
          record_timestamp_ms,
          record_key_size,
          0);
    }

    /**
     * @brief Returns true if the magic value is valid.
     *
     * @return true when magic matches record_magic.
     */
    [[nodiscard]] constexpr bool has_valid_magic() const noexcept
    {
      return magic == core::KvLimits::record_magic;
    }

    /**
     * @brief Returns true if the format version is supported.
     *
     * @return true when version matches current record format.
     */
    [[nodiscard]] constexpr bool has_supported_version() const noexcept
    {
      return format_version == core::KvLimits::record_format_version;
    }

    /**
     * @brief Returns true if the header size is valid.
     *
     * @return true when header_size matches encoded_size.
     */
    [[nodiscard]] constexpr bool has_valid_header_size() const noexcept
    {
      return header_size == encoded_size;
    }

    /**
     * @brief Returns true if the record has a non-zero sequence.
     *
     * @return true when sequence is greater than zero.
     */
    [[nodiscard]] constexpr bool has_sequence() const noexcept
    {
      return sequence > 0;
    }

    /**
     * @brief Returns true if the record has a key.
     *
     * @return true when key_size is greater than zero.
     */
    [[nodiscard]] constexpr bool has_key() const noexcept
    {
      return key_size > 0;
    }

    /**
     * @brief Returns true if the record has a value payload.
     *
     * @return true when value_size is greater than zero.
     */
    [[nodiscard]] constexpr bool has_value() const noexcept
    {
      return value_size > 0;
    }

    /**
     * @brief Returns total payload size.
     *
     * @return key_size + value_size.
     */
    [[nodiscard]] constexpr std::uint64_t payload_size() const noexcept
    {
      return static_cast<std::uint64_t>(key_size) + value_size;
    }

    /**
     * @brief Returns total encoded record size.
     *
     * @return header size + payload size.
     */
    [[nodiscard]] constexpr std::uint64_t total_size() const noexcept
    {
      return static_cast<std::uint64_t>(header_size) + payload_size();
    }

    /**
     * @brief Returns true if key and value sizes are within limits.
     *
     * @return true when sizes are allowed.
     */
    [[nodiscard]] constexpr bool has_valid_sizes() const noexcept
    {
      if (key_size == 0)
      {
        return false;
      }

      if (key_size > core::KvLimits::max_key_size)
      {
        return false;
      }

      if (value_size > core::KvLimits::max_value_size)
      {
        return false;
      }

      if (payload_size() > core::KvLimits::max_record_size)
      {
        return false;
      }

      return true;
    }

    /**
     * @brief Returns true if the type and payload sizes match.
     *
     * @return true when the type-specific shape is valid.
     */
    [[nodiscard]] constexpr bool has_valid_type_shape() const noexcept
    {
      if (!vix::kv::records::is_valid(type))
      {
        return false;
      }

      if (type == KvRecordType::Delete)
      {
        return value_size == 0;
      }

      return true;
    }

    /**
     * @brief Returns true if the header is structurally valid.
     *
     * This does not verify checksums.
     *
     * @return true if the header structure is valid.
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
      return has_valid_magic() &&
             has_supported_version() &&
             has_valid_header_size() &&
             has_sequence() &&
             has_valid_sizes() &&
             has_valid_type_shape();
    }

    /**
     * @brief Clears the header and resets it to an invalid default state.
     */
    void clear() noexcept
    {
      magic = core::KvLimits::record_magic;
      format_version = core::KvLimits::record_format_version;
      type = KvRecordType::Unknown;
      header_size = encoded_size;
      sequence = 0;
      timestamp_ms = 0;
      key_size = 0;
      value_size = 0;
      header_checksum = 0;
      payload_checksum = 0;
    }
  };

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_HEADER_HPP
