/**
 *
 *  @file KvEngineState.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Internal KV engine state
 *
 */

#ifndef VIX_KV_INTERNAL_KV_ENGINE_STATE_HPP
#define VIX_KV_INTERNAL_KV_ENGINE_STATE_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvStats.hpp>
#include <vix/kv/index/KvIndex.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/storage/Segment.hpp>

namespace vix::kv::internal
{
  namespace core = vix::kv::core;
  namespace index = vix::kv::index;
  namespace memtable = vix::kv::memtable;
  namespace storage = vix::kv::storage;

  /**
   * @brief Internal mutable state owned by KvEngine.
   *
   * KvEngineState groups the engine runtime state in one place.
   *
   * It owns:
   * - normalized config
   * - memtable
   * - durable index metadata
   * - known segments
   * - runtime statistics
   * - sequence counters
   *
   * Rules:
   * - open means the engine can accept operations.
   * - next_sequence always returns a strictly increasing value.
   * - last_sequence tracks the highest known sequence.
   */
  struct KvEngineState
  {
    /**
     * @brief Normalized engine configuration.
     */
    core::KvConfig config{};

    /**
     * @brief Materialized in-memory state.
     */
    memtable::MemTable memtable{};

    /**
     * @brief Durable location index.
     */
    index::KvIndex index{};

    /**
     * @brief Known immutable segments.
     */
    std::vector<storage::Segment> segments{};

    /**
     * @brief Runtime statistics.
     */
    core::KvStats stats{};

    /**
     * @brief True when the engine is open.
     */
    bool open{false};

    /**
     * @brief Highest assigned or recovered sequence.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Next segment id to use.
     */
    std::uint64_t next_segment_id{1};

    /**
     * @brief Last snapshot id used.
     */
    std::uint64_t last_snapshot_id{0};

    /**
     * @brief Creates an empty state.
     */
    KvEngineState() = default;

    /**
     * @brief Creates state from config.
     *
     * @param engine_config Normalized engine config.
     */
    explicit KvEngineState(core::KvConfig engine_config)
        : config(std::move(engine_config)),
          memtable(config.initial_capacity),
          index(config.initial_capacity)
    {
      refresh_static_stats();
    }

    /**
     * @brief Returns true if the engine is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept
    {
      return open;
    }

    /**
     * @brief Returns true if this is memory-only mode.
     *
     * @return true when config is memory-only.
     */
    [[nodiscard]] bool is_memory_only() const noexcept
    {
      return config.is_memory_only();
    }

    /**
     * @brief Returns true if WAL is enabled.
     *
     * @return true when WAL is enabled.
     */
    [[nodiscard]] bool wal_enabled() const noexcept
    {
      return config.enable_wal;
    }

    /**
     * @brief Returns next monotonically increasing sequence.
     *
     * @return New sequence number.
     */
    [[nodiscard]] std::uint64_t next_sequence() noexcept
    {
      ++last_sequence;
      stats.last_sequence = last_sequence;
      return last_sequence;
    }

    /**
     * @brief Observes a sequence from recovery or external records.
     *
     * @param sequence Sequence number.
     */
    void observe_sequence(std::uint64_t sequence) noexcept
    {
      if (sequence > last_sequence)
      {
        last_sequence = sequence;
        stats.last_sequence = sequence;
      }
    }

    /**
     * @brief Returns the next segment id and increments it.
     *
     * @return Segment id.
     */
    [[nodiscard]] std::uint64_t allocate_segment_id() noexcept
    {
      const std::uint64_t id = next_segment_id;
      ++next_segment_id;
      return id;
    }

    /**
     * @brief Observes an existing segment id.
     *
     * @param segment_id Segment id.
     */
    void observe_segment_id(std::uint64_t segment_id) noexcept
    {
      if (segment_id >= next_segment_id)
      {
        next_segment_id = segment_id + 1;
      }
    }

    /**
     * @brief Adds a known segment and refreshes segment stats.
     *
     * @param segment Segment metadata.
     */
    void add_segment(storage::Segment segment)
    {
      observe_segment_id(segment.id);
      segments.push_back(std::move(segment));
      refresh_segment_stats();
    }

    /**
     * @brief Returns true if no visible key exists.
     *
     * @return true when memtable has no live entries.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return memtable.empty();
    }

    /**
     * @brief Returns number of live keys.
     *
     * @return Live key count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return memtable.size();
    }

    /**
     * @brief Refreshes static config-related stats.
     */
    void refresh_static_stats() noexcept
    {
      stats.memory_only = config.is_memory_only();
      stats.wal_enabled = config.enable_wal;
      stats.auto_flush = config.auto_flush;
      stats.checksums_enabled = config.enable_checksums;
      stats.read_only = config.read_only;
      stats.auto_compaction_enabled = config.enable_auto_compaction;
    }

    /**
     * @brief Refreshes memtable-related stats.
     */
    void refresh_memtable_stats() noexcept
    {
      const auto live = memtable.size();
      const auto tombstones = memtable.tombstone_count();
      const auto raw = memtable.raw_size();

      stats.key_count = live;
      stats.live_keys = live;

      stats.tombstone_count = tombstones;
      stats.tombstones = tombstones;

      stats.memtable_entries = raw;
      stats.memtable_bytes = memtable.byte_size();

      stats.last_sequence = last_sequence;
    }

    /**
     * @brief Refreshes index-related stats.
     */
    void refresh_index_stats() noexcept
    {
      stats.index_entries = index.raw_size();
      stats.index_live_entries = index.size();
      stats.index_tombstones = index.tombstone_count();
    }

    /**
     * @brief Refreshes segment-related stats.
     */
    void refresh_segment_stats() noexcept
    {
      stats.segment_count = segments.size();
      stats.segment_bytes = 0;
      stats.segment_record_count = 0;

      for (const auto &segment : segments)
      {
        stats.segment_bytes += segment.size_bytes;
        stats.segment_record_count += segment.record_count;
      }
    }

    /**
     * @brief Refreshes all observable stats.
     */
    void refresh_stats() noexcept
    {
      stats.open = open;
      refresh_static_stats();
      refresh_memtable_stats();
      refresh_index_stats();
      refresh_segment_stats();
    }

    /**
     * @brief Clears all runtime state.
     */
    void clear() noexcept
    {
      memtable.clear();
      index.clear();
      segments.clear();
      stats.clear();
      open = false;
      last_sequence = 0;
      next_segment_id = 1;
      last_snapshot_id = 0;
      refresh_static_stats();
    }
  };

} // namespace vix::kv::internal

#endif // VIX_KV_INTERNAL_KV_ENGINE_STATE_HPP
