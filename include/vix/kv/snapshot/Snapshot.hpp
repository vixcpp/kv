/**
 *
 *  @file Snapshot.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot metadata and payload model
 *
 */

#ifndef VIX_KV_SNAPSHOT_SNAPSHOT_HPP
#define VIX_KV_SNAPSHOT_SNAPSHOT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <vix/kv/memtable/MemTableEntry.hpp>

namespace vix::kv::snapshot
{
  namespace memtable = vix::kv::memtable;

  /**
   * @brief Metadata and entries for a KV snapshot.
   *
   * Snapshot represents a point-in-time materialized view of the KV database.
   *
   * It is used by:
   * - SnapshotWriter
   * - SnapshotReader
   * - compaction
   * - recovery optimization
   *
   * Rules:
   * - snapshot id should be greater than zero.
   * - last_sequence should be the highest sequence included.
   * - entries are encoded-key entries.
   * - live entries and tombstones can be stored.
   */
  struct Snapshot
  {
    /**
     * @brief Snapshot identifier.
     */
    std::uint64_t id{0};

    /**
     * @brief Highest sequence included in the snapshot.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Snapshot creation timestamp in milliseconds.
     */
    std::uint64_t created_at_ms{0};

    /**
     * @brief Snapshot entries.
     */
    std::vector<memtable::MemTableEntry> entries{};

    /**
     * @brief Creates an empty snapshot.
     */
    Snapshot() = default;

    /**
     * @brief Creates a snapshot with explicit fields.
     *
     * @param snapshot_id Snapshot identifier.
     * @param snapshot_last_sequence Highest included sequence.
     * @param snapshot_created_at_ms Creation timestamp in milliseconds.
     * @param snapshot_entries Snapshot entries.
     */
    Snapshot(
        std::uint64_t snapshot_id,
        std::uint64_t snapshot_last_sequence,
        std::uint64_t snapshot_created_at_ms,
        std::vector<memtable::MemTableEntry> snapshot_entries)
        : id(snapshot_id),
          last_sequence(snapshot_last_sequence),
          created_at_ms(snapshot_created_at_ms),
          entries(std::move(snapshot_entries))
    {
    }

    /**
     * @brief Creates a snapshot from entries and computes last_sequence.
     *
     * @param snapshot_id Snapshot identifier.
     * @param snapshot_created_at_ms Creation timestamp in milliseconds.
     * @param snapshot_entries Snapshot entries.
     * @return Snapshot.
     */
    [[nodiscard]] static Snapshot make(
        std::uint64_t snapshot_id,
        std::uint64_t snapshot_created_at_ms,
        std::vector<memtable::MemTableEntry> snapshot_entries)
    {
      std::uint64_t max_sequence = 0;

      for (const auto &entry : snapshot_entries)
      {
        if (entry.sequence > max_sequence)
        {
          max_sequence = entry.sequence;
        }
      }

      return Snapshot(
          snapshot_id,
          max_sequence,
          snapshot_created_at_ms,
          std::move(snapshot_entries));
    }

    /**
     * @brief Creates an empty snapshot with metadata.
     *
     * @param snapshot_id Snapshot identifier.
     * @param snapshot_created_at_ms Creation timestamp in milliseconds.
     * @return Empty snapshot.
     */
    [[nodiscard]] static Snapshot empty_snapshot(
        std::uint64_t snapshot_id,
        std::uint64_t snapshot_created_at_ms)
    {
      return Snapshot(
          snapshot_id,
          0,
          snapshot_created_at_ms,
          {});
    }

    /**
     * @brief Returns true if the snapshot has an id.
     *
     * @return true when id is greater than zero.
     */
    [[nodiscard]] bool has_id() const noexcept
    {
      return id > 0;
    }

    /**
     * @brief Returns true if the snapshot has a last sequence.
     *
     * Empty snapshots may have last_sequence = 0.
     *
     * @return true when last_sequence is greater than zero.
     */
    [[nodiscard]] bool has_sequence() const noexcept
    {
      return last_sequence > 0;
    }

    /**
     * @brief Returns true if the snapshot has a timestamp.
     *
     * @return true when created_at_ms is greater than zero.
     */
    [[nodiscard]] bool has_timestamp() const noexcept
    {
      return created_at_ms > 0;
    }

    /**
     * @brief Returns true if the snapshot has entries.
     *
     * @return true when entries is not empty.
     */
    [[nodiscard]] bool has_entries() const noexcept
    {
      return !entries.empty();
    }

    /**
     * @brief Returns number of entries.
     *
     * @return Entry count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return entries.size();
    }

    /**
     * @brief Returns true if there are no entries.
     *
     * @return true when entries is empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return entries.empty();
    }

    /**
     * @brief Returns number of live entries.
     *
     * @return Live entry count.
     */
    [[nodiscard]] std::size_t live_count() const noexcept
    {
      std::size_t count = 0;

      for (const auto &entry : entries)
      {
        if (!entry.deleted)
        {
          ++count;
        }
      }

      return count;
    }

    /**
     * @brief Returns number of tombstone entries.
     *
     * @return Tombstone count.
     */
    [[nodiscard]] std::size_t tombstone_count() const noexcept
    {
      std::size_t count = 0;

      for (const auto &entry : entries)
      {
        if (entry.deleted)
        {
          ++count;
        }
      }

      return count;
    }

    /**
     * @brief Returns approximate payload bytes.
     *
     * @return Approximate owned bytes.
     */
    [[nodiscard]] std::uint64_t byte_size() const noexcept
    {
      std::uint64_t total = 0;

      for (const auto &entry : entries)
      {
        total += entry.byte_size();
      }

      return total;
    }

    /**
     * @brief Returns the highest sequence found in entries.
     *
     * @return Highest entry sequence.
     */
    [[nodiscard]] std::uint64_t compute_last_sequence() const noexcept
    {
      std::uint64_t max_sequence = 0;

      for (const auto &entry : entries)
      {
        if (entry.sequence > max_sequence)
        {
          max_sequence = entry.sequence;
        }
      }

      return max_sequence;
    }

    /**
     * @brief Recomputes last_sequence from entries.
     */
    void refresh_last_sequence() noexcept
    {
      last_sequence = compute_last_sequence();
    }

    /**
     * @brief Reserves storage for entries.
     *
     * @param capacity Entry capacity.
     */
    void reserve(std::size_t capacity)
    {
      entries.reserve(capacity);
    }

    /**
     * @brief Adds one entry and updates last_sequence.
     *
     * @param entry Snapshot entry.
     */
    void add(memtable::MemTableEntry entry)
    {
      if (entry.sequence > last_sequence)
      {
        last_sequence = entry.sequence;
      }

      entries.push_back(std::move(entry));
    }

    /**
     * @brief Returns true if all entries are structurally valid.
     *
     * @return true when all entries are valid.
     */
    [[nodiscard]] bool entries_are_valid() const noexcept
    {
      for (const auto &entry : entries)
      {
        if (!entry.is_valid())
        {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Returns true if last_sequence matches entries.
     *
     * Empty snapshots may have last_sequence = 0.
     *
     * @return true when last_sequence is consistent.
     */
    [[nodiscard]] bool sequence_is_consistent() const noexcept
    {
      if (entries.empty())
      {
        return last_sequence == 0;
      }

      return last_sequence == compute_last_sequence();
    }

    /**
     * @brief Returns true if snapshot is structurally valid.
     *
     * Empty snapshots are allowed, but id must be valid.
     *
     * @return true if valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      if (!has_id())
      {
        return false;
      }

      if (!entries_are_valid())
      {
        return false;
      }

      if (!sequence_is_consistent())
      {
        return false;
      }

      return true;
    }

    /**
     * @brief Clears the snapshot.
     */
    void clear() noexcept
    {
      id = 0;
      last_sequence = 0;
      created_at_ms = 0;
      entries.clear();
    }
  };

} // namespace vix::kv::snapshot

#endif // VIX_KV_SNAPSHOT_SNAPSHOT_HPP
