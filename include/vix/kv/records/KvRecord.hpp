/**
 *
 *  @file KvRecord.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Complete KV record
 *
 */

#ifndef VIX_KV_RECORDS_KV_RECORD_HPP
#define VIX_KV_RECORDS_KV_RECORD_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>
#include <vix/kv/records/KvRecordType.hpp>

namespace vix::kv::records
{
  namespace core = vix::kv::core;

  /**
   * @brief Complete KV record.
   *
   * KvRecord is the logical representation of one encoded KV record.
   *
   * A record contains:
   * - a stable binary header
   * - an encoded key
   * - an optional value payload
   *
   * Rules:
   * - key must not be empty.
   * - key size must match header.key_size.
   * - value size must match header.value_size.
   * - Delete records must have an empty value.
   * - sequence must be greater than zero.
   */
  struct KvRecord
  {
    /**
     * @brief Record header.
     */
    KvRecordHeader header{};

    /**
     * @brief Encoded key bytes.
     */
    std::string key{};

    /**
     * @brief Value bytes.
     */
    std::vector<std::uint8_t> value{};

    /**
     * @brief Creates an empty invalid record.
     */
    KvRecord() = default;

    /**
     * @brief Creates a record with explicit fields.
     *
     * @param record_header Record header.
     * @param record_key Encoded key.
     * @param record_value Value bytes.
     */
    KvRecord(
        KvRecordHeader record_header,
        std::string record_key,
        std::vector<std::uint8_t> record_value)
        : header(std::move(record_header)),
          key(std::move(record_key)),
          value(std::move(record_value))
    {
      refresh_sizes();
    }

    /**
     * @brief Creates a Put record.
     *
     * @param encoded_key Encoded key.
     * @param record_value Value bytes.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Put record.
     */
    [[nodiscard]] static KvRecord put(
        std::string encoded_key,
        std::vector<std::uint8_t> record_value,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms)
    {
      KvRecord record;

      record.key = std::move(encoded_key);
      record.value = std::move(record_value);

      record.header = KvRecordHeader::put(
          sequence,
          timestamp_ms,
          static_cast<std::uint32_t>(record.key.size()),
          static_cast<std::uint64_t>(record.value.size()));

      return record;
    }

    /**
     * @brief Creates a Delete record.
     *
     * @param encoded_key Encoded key.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Delete record.
     */
    [[nodiscard]] static KvRecord remove(
        std::string encoded_key,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms)
    {
      KvRecord record;

      record.key = std::move(encoded_key);
      record.value.clear();

      record.header = KvRecordHeader::remove(
          sequence,
          timestamp_ms,
          static_cast<std::uint32_t>(record.key.size()));

      return record;
    }

    /**
     * @brief Creates a Snapshot record.
     *
     * @param encoded_key Encoded key.
     * @param record_value Snapshot value bytes.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Snapshot record.
     */
    [[nodiscard]] static KvRecord snapshot(
        std::string encoded_key,
        std::vector<std::uint8_t> record_value,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms)
    {
      KvRecord record;

      record.key = std::move(encoded_key);
      record.value = std::move(record_value);

      record.header = KvRecordHeader(
          KvRecordType::Snapshot,
          sequence,
          timestamp_ms,
          static_cast<std::uint32_t>(record.key.size()),
          static_cast<std::uint64_t>(record.value.size()));

      return record;
    }

    /**
     * @brief Creates a Compaction marker record.
     *
     * @param encoded_key Marker key.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Compaction record.
     */
    [[nodiscard]] static KvRecord compaction(
        std::string encoded_key,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms)
    {
      KvRecord record;

      record.key = std::move(encoded_key);
      record.value.clear();

      record.header = KvRecordHeader(
          KvRecordType::Compaction,
          sequence,
          timestamp_ms,
          static_cast<std::uint32_t>(record.key.size()),
          0);

      return record;
    }

    /**
     * @brief Returns true if the record is a Put.
     *
     * @return true for Put.
     */
    [[nodiscard]] bool is_put() const noexcept
    {
      return header.type == KvRecordType::Put;
    }

    /**
     * @brief Returns true if the record is a Delete.
     *
     * @return true for Delete.
     */
    [[nodiscard]] bool is_delete() const noexcept
    {
      return header.type == KvRecordType::Delete;
    }

    /**
     * @brief Returns true if the record is a Snapshot.
     *
     * @return true for Snapshot.
     */
    [[nodiscard]] bool is_snapshot() const noexcept
    {
      return header.type == KvRecordType::Snapshot;
    }

    /**
     * @brief Returns true if the record is a Compaction marker.
     *
     * @return true for Compaction.
     */
    [[nodiscard]] bool is_compaction() const noexcept
    {
      return header.type == KvRecordType::Compaction;
    }

    /**
     * @brief Returns true if the record has an encoded key.
     *
     * @return true when key is not empty.
     */
    [[nodiscard]] bool has_key() const noexcept
    {
      return !key.empty();
    }

    /**
     * @brief Returns true if the record has a value payload.
     *
     * @return true when value is not empty.
     */
    [[nodiscard]] bool has_value() const noexcept
    {
      return !value.empty();
    }

    /**
     * @brief Returns payload size.
     *
     * @return key size + value size.
     */
    [[nodiscard]] std::uint64_t payload_size() const noexcept
    {
      return static_cast<std::uint64_t>(key.size()) +
             static_cast<std::uint64_t>(value.size());
    }

    /**
     * @brief Returns total encoded record size.
     *
     * @return header size + payload size.
     */
    [[nodiscard]] std::uint64_t total_size() const noexcept
    {
      return static_cast<std::uint64_t>(KvRecordHeader::encoded_size) +
             payload_size();
    }

    /**
     * @brief Refreshes header key and value sizes from payload fields.
     */
    void refresh_sizes() noexcept
    {
      header.key_size = static_cast<std::uint32_t>(key.size());
      header.value_size = static_cast<std::uint64_t>(value.size());
      header.header_size = KvRecordHeader::encoded_size;
    }

    /**
     * @brief Returns true if header sizes match payload sizes.
     *
     * @return true when sizes are consistent.
     */
    [[nodiscard]] bool has_consistent_sizes() const noexcept
    {
      return header.key_size == key.size() &&
             header.value_size == value.size();
    }

    /**
     * @brief Returns true if the record payload is within limits.
     *
     * @return true when key and value sizes are allowed.
     */
    [[nodiscard]] bool has_valid_payload_sizes() const noexcept
    {
      if (key.empty())
      {
        return false;
      }

      if (key.size() > core::KvLimits::max_key_size)
      {
        return false;
      }

      if (value.size() > core::KvLimits::max_value_size)
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
     * @brief Returns true if type-specific payload shape is valid.
     *
     * @return true when payload matches record type.
     */
    [[nodiscard]] bool has_valid_type_shape() const noexcept
    {
      if (!vix::kv::records::is_valid(header.type))
      {
        return false;
      }

      if (header.type == KvRecordType::Delete)
      {
        return value.empty();
      }

      return true;
    }

    /**
     * @brief Returns true if the record is structurally valid.
     *
     * This does not verify checksums.
     *
     * @return true when the record structure is valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return header.is_valid() &&
             has_consistent_sizes() &&
             has_valid_payload_sizes() &&
             has_valid_type_shape();
    }

    /**
     * @brief Clears the record.
     */
    void clear() noexcept
    {
      header.clear();
      key.clear();
      value.clear();
    }
  };

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_HPP
