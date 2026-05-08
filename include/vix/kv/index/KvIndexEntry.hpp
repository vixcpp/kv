/**
 *
 *  @file KvIndexEntry.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Durable index entry metadata
 *
 */

#ifndef VIX_KV_INDEX_KV_INDEX_ENTRY_HPP
#define VIX_KV_INDEX_KV_INDEX_ENTRY_HPP

#include <cstdint>
#include <string>
#include <utility>

namespace vix::kv::index
{
  /**
   * @brief Index metadata for one encoded key.
   *
   * KvIndexEntry maps an encoded key to a physical record location.
   *
   * It is used by:
   * - segment storage
   * - data file lookup
   * - compaction
   * - snapshots
   * - future durable index loading
   *
   * Rules:
   * - key must not be empty.
   * - sequence must be greater than zero.
   * - deleted entries are tombstones.
   * - offset and size point to the encoded record location.
   */
  struct KvIndexEntry
  {
    /**
     * @brief Encoded key.
     */
    std::string key{};

    /**
     * @brief Segment identifier.
     *
     * For WAL-only or memory-only modes, this can stay 0.
     */
    std::uint64_t segment_id{0};

    /**
     * @brief Byte offset of the record inside the segment or data file.
     */
    std::uint64_t offset{0};

    /**
     * @brief Encoded record size in bytes.
     */
    std::uint64_t size{0};

    /**
     * @brief Last sequence that produced this index entry.
     */
    std::uint64_t sequence{0};

    /**
     * @brief Last update timestamp in milliseconds.
     */
    std::uint64_t timestamp_ms{0};

    /**
     * @brief True when this entry represents a deleted key.
     */
    bool deleted{false};

    /**
     * @brief Creates an empty invalid entry.
     */
    KvIndexEntry() = default;

    /**
     * @brief Creates an index entry with explicit fields.
     *
     * @param entry_key Encoded key.
     * @param entry_segment_id Segment identifier.
     * @param entry_offset Record offset.
     * @param entry_size Record size.
     * @param entry_sequence Record sequence.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @param entry_deleted Deleted flag.
     */
    KvIndexEntry(
        std::string entry_key,
        std::uint64_t entry_segment_id,
        std::uint64_t entry_offset,
        std::uint64_t entry_size,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms,
        bool entry_deleted)
        : key(std::move(entry_key)),
          segment_id(entry_segment_id),
          offset(entry_offset),
          size(entry_size),
          sequence(entry_sequence),
          timestamp_ms(entry_timestamp_ms),
          deleted(entry_deleted)
    {
    }

    /**
     * @brief Creates a live index entry.
     *
     * @param entry_key Encoded key.
     * @param entry_segment_id Segment identifier.
     * @param entry_offset Record offset.
     * @param entry_size Record size.
     * @param entry_sequence Record sequence.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @return Live index entry.
     */
    [[nodiscard]] static KvIndexEntry live(
        std::string entry_key,
        std::uint64_t entry_segment_id,
        std::uint64_t entry_offset,
        std::uint64_t entry_size,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms)
    {
      return KvIndexEntry(
          std::move(entry_key),
          entry_segment_id,
          entry_offset,
          entry_size,
          entry_sequence,
          entry_timestamp_ms,
          false);
    }

    /**
     * @brief Creates a tombstone index entry.
     *
     * @param entry_key Encoded key.
     * @param entry_segment_id Segment identifier.
     * @param entry_offset Record offset.
     * @param entry_size Record size.
     * @param entry_sequence Record sequence.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @return Tombstone index entry.
     */
    [[nodiscard]] static KvIndexEntry tombstone(
        std::string entry_key,
        std::uint64_t entry_segment_id,
        std::uint64_t entry_offset,
        std::uint64_t entry_size,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms)
    {
      return KvIndexEntry(
          std::move(entry_key),
          entry_segment_id,
          entry_offset,
          entry_size,
          entry_sequence,
          entry_timestamp_ms,
          true);
    }

    /**
     * @brief Returns true if the entry has a key.
     *
     * @return true when key is not empty.
     */
    [[nodiscard]] bool has_key() const noexcept
    {
      return !key.empty();
    }

    /**
     * @brief Returns true if the entry has a sequence.
     *
     * @return true when sequence is greater than zero.
     */
    [[nodiscard]] bool has_sequence() const noexcept
    {
      return sequence > 0;
    }

    /**
     * @brief Returns true if the entry has a physical record location.
     *
     * @return true when size is greater than zero.
     */
    [[nodiscard]] bool has_location() const noexcept
    {
      return size > 0;
    }

    /**
     * @brief Returns true if this entry is live.
     *
     * @return true when not deleted.
     */
    [[nodiscard]] bool is_live() const noexcept
    {
      return !deleted;
    }

    /**
     * @brief Returns true if this entry is a tombstone.
     *
     * @return true when deleted.
     */
    [[nodiscard]] bool is_tombstone() const noexcept
    {
      return deleted;
    }

    /**
     * @brief Returns true if the entry is structurally valid.
     *
     * @return true when key and sequence are valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return has_key() && has_sequence();
    }

    /**
     * @brief Clears the entry.
     */
    void clear() noexcept
    {
      key.clear();
      segment_id = 0;
      offset = 0;
      size = 0;
      sequence = 0;
      timestamp_ms = 0;
      deleted = false;
    }
  };

} // namespace vix::kv::index

#endif // VIX_KV_INDEX_KV_INDEX_ENTRY_HPP
