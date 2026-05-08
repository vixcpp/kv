/**
 *
 *  @file KvStats.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Runtime statistics for the KV engine
 *
 */

#ifndef VIX_KV_CORE_KV_STATS_HPP
#define VIX_KV_CORE_KV_STATS_HPP

#include <cstddef>
#include <cstdint>

namespace vix::kv::core
{
  /**
   * @brief Runtime statistics exposed by the KV engine.
   *
   * KvStats is a lightweight snapshot of the current engine state.
   *
   * It is used by:
   * - KvEngine
   * - public Kv API
   * - diagnostics
   * - tests
   * - future CLI or monitoring integrations
   *
   * Rules:
   * - stats must be cheap to copy.
   * - stats must not own engine resources.
   * - stats should describe observable engine state.
   * - lifetime counters should be monotonic unless explicitly reset.
   */
  struct KvStats
  {
    /**
     * @brief True when the KV engine is open.
     */
    bool open{false};

    /**
     * @brief True when the engine is memory-only.
     */
    bool memory_only{false};

    /**
     * @brief True when WAL is enabled.
     */
    bool wal_enabled{false};

    /**
     * @brief True when auto flush is enabled.
     */
    bool auto_flush{false};

    /**
     * @brief True when checksums are enabled.
     */
    bool checksums_enabled{false};

    /**
     * @brief True when the engine is opened in read-only mode.
     */
    bool read_only{false};

    /**
     * @brief True when automatic compaction is enabled.
     */
    bool auto_compaction_enabled{false};

    /**
     * @brief Number of live keys currently visible.
     */
    std::size_t key_count{0};

    /**
     * @brief Number of tombstones currently tracked.
     */
    std::size_t tombstone_count{0};

    /**
     * @brief Number of raw memtable entries.
     *
     * This includes live entries and tombstones.
     */
    std::size_t memtable_entries{0};

    /**
     * @brief Approximate memory used by the memtable in bytes.
     */
    std::uint64_t memtable_bytes{0};

    /**
     * @brief Number of raw index entries.
     *
     * This includes live entries and tombstones.
     */
    std::size_t index_entries{0};

    /**
     * @brief Number of live index entries.
     */
    std::size_t index_live_entries{0};

    /**
     * @brief Number of tombstones tracked by the index.
     */
    std::size_t index_tombstones{0};

    /**
     * @brief Current WAL size in bytes, if known.
     */
    std::uint64_t wal_bytes{0};

    /**
     * @brief Number of WAL records written during the engine lifetime.
     */
    std::uint64_t wal_records_written{0};

    /**
     * @brief Number of bytes written to WAL during the engine lifetime.
     */
    std::uint64_t wal_bytes_written{0};

    /**
     * @brief Number of WAL records read during recovery.
     */
    std::uint64_t wal_records_recovered{0};

    /**
     * @brief Number of WAL records skipped during recovery.
     */
    std::uint64_t wal_records_skipped{0};

    /**
     * @brief Number of corrupted WAL records detected.
     */
    std::uint64_t wal_records_corrupted{0};

    /**
     * @brief Number of bytes read from WAL during recovery.
     */
    std::uint64_t wal_bytes_recovered{0};

    /**
     * @brief Last assigned sequence number.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Last recovered sequence number.
     */
    std::uint64_t last_recovered_sequence{0};

    /**
     * @brief Number of successful set operations.
     */
    std::uint64_t set_count{0};

    /**
     * @brief Number of successful get operations.
     */
    std::uint64_t get_count{0};

    /**
     * @brief Number of get operations that missed.
     */
    std::uint64_t get_miss_count{0};

    /**
     * @brief Number of successful erase operations.
     */
    std::uint64_t erase_count{0};

    /**
     * @brief Number of erase operations for missing keys.
     */
    std::uint64_t erase_miss_count{0};

    /**
     * @brief Number of list operations.
     */
    std::uint64_t list_count{0};

    /**
     * @brief Number of flush operations.
     */
    std::uint64_t flush_count{0};

    /**
     * @brief Number of close operations.
     */
    std::uint64_t close_count{0};

    /**
     * @brief Number of failed operations.
     */
    std::uint64_t error_count{0};

    /**
     * @brief Number of segment files currently known.
     */
    std::size_t segment_count{0};

    /**
     * @brief Total size of segment files in bytes, if known.
     */
    std::uint64_t segment_bytes{0};

    /**
     * @brief Number of segment records currently known.
     */
    std::uint64_t segment_record_count{0};

    /**
     * @brief Number of snapshot files currently known.
     */
    std::size_t snapshot_count{0};

    /**
     * @brief Total size of snapshot files in bytes, if known.
     */
    std::uint64_t snapshot_bytes{0};

    /**
     * @brief Last snapshot id written or loaded.
     */
    std::uint64_t last_snapshot_id{0};

    /**
     * @brief Last snapshot sequence included.
     */
    std::uint64_t last_snapshot_sequence{0};

    /**
     * @brief Number of compaction runs completed.
     */
    std::uint64_t compaction_count{0};

    /**
     * @brief Number of input records processed by compaction.
     */
    std::uint64_t compaction_input_records{0};

    /**
     * @brief Number of output records produced by compaction.
     */
    std::uint64_t compaction_output_records{0};

    /**
     * @brief Number of obsolete records skipped by compaction.
     */
    std::uint64_t compacted_obsolete_records{0};

    /**
     * @brief Number of tombstones skipped or removed by compaction.
     */
    std::uint64_t compacted_tombstones{0};

    /**
     * @brief Number of bytes read by compaction.
     */
    std::uint64_t compaction_input_bytes{0};

    /**
     * @brief Number of bytes written by compaction.
     */
    std::uint64_t compaction_output_bytes{0};

    /**
     * @brief Number of bytes reclaimed by compaction, if known.
     */
    std::uint64_t compacted_bytes{0};

    /**
     * @brief Returns true if the engine currently contains no visible keys.
     *
     * @return true when key_count is zero.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return key_count == 0;
    }

    /**
     * @brief Returns true if there are raw entries in memory.
     *
     * @return true when memtable_entries is greater than zero.
     */
    [[nodiscard]] bool has_memtable_entries() const noexcept
    {
      return memtable_entries > 0;
    }

    /**
     * @brief Returns true if there are tombstones.
     *
     * @return true when tombstone_count is greater than zero.
     */
    [[nodiscard]] bool has_tombstones() const noexcept
    {
      return tombstone_count > 0;
    }

    /**
     * @brief Returns true if the engine has observed at least one error.
     *
     * @return true when error_count is greater than zero.
     */
    [[nodiscard]] bool has_errors() const noexcept
    {
      return error_count > 0;
    }

    /**
     * @brief Returns true if recovery detected corrupted WAL records.
     *
     * @return true when wal_records_corrupted is greater than zero.
     */
    [[nodiscard]] bool has_corruption() const noexcept
    {
      return wal_records_corrupted > 0;
    }

    /**
     * @brief Returns true if WAL recovery recovered at least one record.
     *
     * @return true when wal_records_recovered is greater than zero.
     */
    [[nodiscard]] bool recovered_from_wal() const noexcept
    {
      return wal_records_recovered > 0;
    }

    /**
     * @brief Returns true if WAL recovery skipped records.
     *
     * @return true when wal_records_skipped is greater than zero.
     */
    [[nodiscard]] bool skipped_wal_records() const noexcept
    {
      return wal_records_skipped > 0;
    }

    /**
     * @brief Returns true if compaction has run at least once.
     *
     * @return true when compaction_count is greater than zero.
     */
    [[nodiscard]] bool has_compacted() const noexcept
    {
      return compaction_count > 0;
    }

    /**
     * @brief Returns true if a snapshot is known.
     *
     * @return true when last_snapshot_id is greater than zero.
     */
    [[nodiscard]] bool has_snapshot() const noexcept
    {
      return last_snapshot_id > 0;
    }

    /**
     * @brief Returns total durable bytes known by the engine.
     *
     * This includes WAL, segments, and snapshots when those values are known.
     *
     * @return Total durable bytes.
     */
    [[nodiscard]] std::uint64_t durable_bytes() const noexcept
    {
      return wal_bytes + segment_bytes + snapshot_bytes;
    }

    /**
     * @brief Returns total successful write-like operations.
     *
     * @return set_count + erase_count.
     */
    [[nodiscard]] std::uint64_t write_count() const noexcept
    {
      return set_count + erase_count;
    }

    /**
     * @brief Returns total read-like operations.
     *
     * @return get_count + get_miss_count + list_count.
     */
    [[nodiscard]] std::uint64_t read_count() const noexcept
    {
      return get_count + get_miss_count + list_count;
    }

    /**
     * @brief Returns total successful user operations.
     *
     * @return successful reads and writes.
     */
    [[nodiscard]] std::uint64_t success_count() const noexcept
    {
      return set_count +
             get_count +
             erase_count +
             list_count +
             flush_count;
    }

    /**
     * @brief Returns total observed user operations.
     *
     * @return successful operations, misses, and errors.
     */
    [[nodiscard]] std::uint64_t operation_count() const noexcept
    {
      return success_count() +
             get_miss_count +
             erase_miss_count +
             error_count;
    }

    /**
     * @brief Returns total bytes processed by compaction.
     *
     * @return compaction input + output bytes.
     */
    [[nodiscard]] std::uint64_t compaction_bytes_processed() const noexcept
    {
      return compaction_input_bytes + compaction_output_bytes;
    }

    /**
     * @brief Returns true if compaction reclaimed bytes.
     *
     * @return true when compacted_bytes is greater than zero.
     */
    [[nodiscard]] bool reclaimed_bytes() const noexcept
    {
      return compacted_bytes > 0;
    }

    /**
     * @brief Resets lifetime user operation counters.
     *
     * This keeps structural state such as key_count, WAL size, segment size,
     * snapshot size, and sequence numbers.
     */
    void reset_operation_counters() noexcept
    {
      set_count = 0;
      get_count = 0;
      get_miss_count = 0;
      erase_count = 0;
      erase_miss_count = 0;
      list_count = 0;
      flush_count = 0;
      close_count = 0;
      error_count = 0;
    }

    /**
     * @brief Resets recovery-related counters.
     */
    void reset_recovery_counters() noexcept
    {
      wal_records_recovered = 0;
      wal_records_skipped = 0;
      wal_records_corrupted = 0;
      wal_bytes_recovered = 0;
      last_recovered_sequence = 0;
    }

    /**
     * @brief Resets WAL write counters.
     */
    void reset_wal_write_counters() noexcept
    {
      wal_records_written = 0;
      wal_bytes_written = 0;
    }

    /**
     * @brief Resets compaction-related counters.
     */
    void reset_compaction_counters() noexcept
    {
      compaction_count = 0;
      compaction_input_records = 0;
      compaction_output_records = 0;
      compacted_obsolete_records = 0;
      compacted_tombstones = 0;
      compaction_input_bytes = 0;
      compaction_output_bytes = 0;
      compacted_bytes = 0;
    }

    /**
     * @brief Resets snapshot-related counters.
     */
    void reset_snapshot_counters() noexcept
    {
      snapshot_count = 0;
      snapshot_bytes = 0;
      last_snapshot_id = 0;
      last_snapshot_sequence = 0;
    }

    /**
     * @brief Resets all counters and state.
     */
    void clear() noexcept
    {
      open = false;
      memory_only = false;
      wal_enabled = false;
      auto_flush = false;
      checksums_enabled = false;
      read_only = false;
      auto_compaction_enabled = false;

      key_count = 0;
      tombstone_count = 0;
      memtable_entries = 0;
      memtable_bytes = 0;

      index_entries = 0;
      index_live_entries = 0;
      index_tombstones = 0;

      wal_bytes = 0;
      wal_records_written = 0;
      wal_bytes_written = 0;
      wal_records_recovered = 0;
      wal_records_skipped = 0;
      wal_records_corrupted = 0;
      wal_bytes_recovered = 0;

      last_sequence = 0;
      last_recovered_sequence = 0;

      set_count = 0;
      get_count = 0;
      get_miss_count = 0;
      erase_count = 0;
      erase_miss_count = 0;
      list_count = 0;
      flush_count = 0;
      close_count = 0;
      error_count = 0;

      segment_count = 0;
      segment_bytes = 0;
      segment_record_count = 0;

      snapshot_count = 0;
      snapshot_bytes = 0;
      last_snapshot_id = 0;
      last_snapshot_sequence = 0;

      compaction_count = 0;
      compaction_input_records = 0;
      compaction_output_records = 0;
      compacted_obsolete_records = 0;
      compacted_tombstones = 0;
      compaction_input_bytes = 0;
      compaction_output_bytes = 0;
      compacted_bytes = 0;
    }
  };

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_STATS_HPP
