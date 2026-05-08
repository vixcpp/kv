/**
 *
 *  @file KvRecordType.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Stable record type identifiers
 *
 */

#ifndef VIX_KV_RECORDS_KV_RECORD_TYPE_HPP
#define VIX_KV_RECORDS_KV_RECORD_TYPE_HPP

#include <cstdint>
#include <string_view>

namespace vix::kv::records
{
  /**
   * @brief Type of record stored in the KV log or storage files.
   *
   * KvRecordType describes the logical mutation or storage event represented
   * by a record.
   *
   * Rules:
   * - values must remain stable after release.
   * - do not reorder existing values.
   * - do not remove existing values.
   * - add new values only at the end.
   */
  enum class KvRecordType : std::uint8_t
  {
    /**
     * @brief Unknown or invalid record type.
     */
    Unknown = 0,

    /**
     * @brief Insert or replace a value for a key.
     */
    Put = 1,

    /**
     * @brief Remove a key.
     */
    Delete = 2,

    /**
     * @brief Snapshot marker or snapshot payload record.
     */
    Snapshot = 3,

    /**
     * @brief Compaction marker record.
     */
    Compaction = 4
  };

  /**
   * @brief Converts a record type to a stable string.
   *
   * @param type Record type.
   * @return Stable string representation.
   */
  [[nodiscard]] constexpr std::string_view
  to_string(KvRecordType type) noexcept
  {
    switch (type)
    {
    case KvRecordType::Unknown:
      return "unknown";

    case KvRecordType::Put:
      return "put";

    case KvRecordType::Delete:
      return "delete";

    case KvRecordType::Snapshot:
      return "snapshot";

    case KvRecordType::Compaction:
      return "compaction";

    default:
      return "invalid";
    }
  }

  /**
   * @brief Returns true when the record type is usable.
   *
   * Unknown is intentionally invalid.
   *
   * @param type Record type.
   * @return true for known usable record types.
   */
  [[nodiscard]] constexpr bool is_valid(KvRecordType type) noexcept
  {
    return type == KvRecordType::Put ||
           type == KvRecordType::Delete ||
           type == KvRecordType::Snapshot ||
           type == KvRecordType::Compaction;
  }

  /**
   * @brief Returns true if the record writes a value payload.
   *
   * @param type Record type.
   * @return true for Put and Snapshot.
   */
  [[nodiscard]] constexpr bool writes_value(KvRecordType type) noexcept
  {
    return type == KvRecordType::Put ||
           type == KvRecordType::Snapshot;
  }

  /**
   * @brief Returns true if the record removes a key.
   *
   * @param type Record type.
   * @return true for Delete.
   */
  [[nodiscard]] constexpr bool deletes_value(KvRecordType type) noexcept
  {
    return type == KvRecordType::Delete;
  }

  /**
   * @brief Returns true if the record is a metadata/control record.
   *
   * @param type Record type.
   * @return true for Snapshot and Compaction.
   */
  [[nodiscard]] constexpr bool is_control_record(KvRecordType type) noexcept
  {
    return type == KvRecordType::Snapshot ||
           type == KvRecordType::Compaction;
  }

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_TYPE_HPP
