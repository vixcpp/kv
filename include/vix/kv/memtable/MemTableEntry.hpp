/**
 *
 *  @file MemTableEntry.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  In-memory materialized entry
 *
 */

#ifndef VIX_KV_MEMTABLE_MEM_TABLE_ENTRY_HPP
#define VIX_KV_MEMTABLE_MEM_TABLE_ENTRY_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace vix::kv::memtable
{
  /**
   * @brief Materialized in-memory entry for the KV engine.
   *
   * MemTableEntry represents the latest known state for one encoded key.
   *
   * It can represent:
   * - a live value
   * - a tombstone
   *
   * Tombstones are useful during recovery, snapshots, and compaction because
   * they preserve delete intent until it is safe to remove old state.
   */
  struct MemTableEntry
  {
    /**
     * @brief Encoded key.
     */
    std::string key{};

    /**
     * @brief Value bytes.
     *
     * Empty values are allowed for live records.
     * Delete records usually keep this empty.
     */
    std::vector<std::uint8_t> value{};

    /**
     * @brief Last sequence number that produced this entry.
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
    MemTableEntry() = default;

    /**
     * @brief Creates an entry with explicit fields.
     *
     * @param entry_key Encoded key.
     * @param entry_value Value bytes.
     * @param entry_sequence Sequence number.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @param entry_deleted Deleted flag.
     */
    MemTableEntry(
        std::string entry_key,
        std::vector<std::uint8_t> entry_value,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms,
        bool entry_deleted)
        : key(std::move(entry_key)),
          value(std::move(entry_value)),
          sequence(entry_sequence),
          timestamp_ms(entry_timestamp_ms),
          deleted(entry_deleted)
    {
    }

    /**
     * @brief Creates a live entry.
     *
     * @param entry_key Encoded key.
     * @param entry_value Value bytes.
     * @param entry_sequence Sequence number.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @return Live entry.
     */
    [[nodiscard]] static MemTableEntry live(
        std::string entry_key,
        std::vector<std::uint8_t> entry_value,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms)
    {
      return MemTableEntry(
          std::move(entry_key),
          std::move(entry_value),
          entry_sequence,
          entry_timestamp_ms,
          false);
    }

    /**
     * @brief Creates a tombstone entry.
     *
     * @param entry_key Encoded key.
     * @param entry_sequence Sequence number.
     * @param entry_timestamp_ms Timestamp in milliseconds.
     * @return Tombstone entry.
     */
    [[nodiscard]] static MemTableEntry tombstone(
        std::string entry_key,
        std::uint64_t entry_sequence,
        std::uint64_t entry_timestamp_ms)
    {
      return MemTableEntry(
          std::move(entry_key),
          {},
          entry_sequence,
          entry_timestamp_ms,
          true);
    }

    /**
     * @brief Returns true if this entry has a non-empty key.
     *
     * @return true when key is not empty.
     */
    [[nodiscard]] bool has_key() const noexcept
    {
      return !key.empty();
    }

    /**
     * @brief Returns true if this entry has a sequence number.
     *
     * @return true when sequence is greater than zero.
     */
    [[nodiscard]] bool has_sequence() const noexcept
    {
      return sequence > 0;
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
     * @brief Returns true if the entry has a value payload.
     *
     * Empty live values are allowed, so this is only a payload check.
     *
     * @return true when value has at least one byte.
     */
    [[nodiscard]] bool has_value_payload() const noexcept
    {
      return !value.empty();
    }

    /**
     * @brief Returns approximate memory usage in bytes.
     *
     * @return Approximate owned bytes.
     */
    [[nodiscard]] std::uint64_t byte_size() const noexcept
    {
      return static_cast<std::uint64_t>(key.size()) +
             static_cast<std::uint64_t>(value.size());
    }

    /**
     * @brief Returns true if the entry is structurally valid.
     *
     * Tombstones may have an empty value.
     * Live entries may also have an empty value because empty values are valid.
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
      value.clear();
      sequence = 0;
      timestamp_ms = 0;
      deleted = false;
    }
  };

} // namespace vix::kv::memtable

#endif // VIX_KV_MEMTABLE_MEM_TABLE_ENTRY_HPP
