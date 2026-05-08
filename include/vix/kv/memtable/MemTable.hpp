/**
 *
 *  @file MemTable.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  In-memory materialized table
 *
 */

#ifndef VIX_KV_MEMTABLE_MEM_TABLE_HPP
#define VIX_KV_MEMTABLE_MEM_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/memtable/MemTableEntry.hpp>

namespace vix::kv::memtable
{
  namespace core = vix::kv::core;

  /**
   * @brief In-memory materialized KV table.
   *
   * MemTable stores the latest known state for encoded keys.
   *
   * It is used by:
   * - KvEngine
   * - WAL recovery
   * - snapshots
   * - compaction
   *
   * Rules:
   * - keys are already encoded before entering MemTable.
   * - empty keys are invalid.
   * - empty values are allowed.
   * - deletes are stored as tombstones.
   * - older sequences must not overwrite newer sequences.
   */
  class MemTable
  {
  public:
    /**
     * @brief Internal map type.
     */
    using Map = std::unordered_map<std::string, MemTableEntry>;

    /**
     * @brief List result type.
     */
    using List = std::vector<MemTableEntry>;

    /**
     * @brief Creates an empty memtable.
     */
    MemTable() = default;

    /**
     * @brief Creates a memtable and reserves initial capacity.
     *
     * @param initial_capacity Initial map capacity.
     */
    explicit MemTable(std::size_t initial_capacity);

    /**
     * @brief Reserves storage for entries.
     *
     * @param capacity Requested capacity.
     */
    void reserve(std::size_t capacity);

    /**
     * @brief Inserts or replaces a live value.
     *
     * If the key already exists with a newer sequence, the write is ignored.
     *
     * @param key Encoded key.
     * @param value Value bytes.
     * @param sequence Sequence number.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> put(
        std::string_view key,
        const std::vector<std::uint8_t> &value,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Inserts or replaces a live string value.
     *
     * This overload keeps tests and internal usage simple:
     *
     * @code
     * table.put("v1|5:hello", "world", 1, 0);
     * @endcode
     *
     * @param key Encoded key.
     * @param value Text value.
     * @param sequence Sequence number.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> put(
        std::string_view key,
        std::string_view value,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Stores a tombstone for a key.
     *
     * If the key already exists with a newer sequence, the tombstone is ignored.
     *
     * @param key Encoded key.
     * @param sequence Sequence number.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> erase(
        std::string_view key,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Stores a tombstone for a key.
     *
     * @param key Encoded key.
     * @param sequence Sequence number.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> erase(
        std::string_view key,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Applies an entry directly.
     *
     * If the table already contains a newer sequence, the entry is ignored.
     *
     * @param entry Entry to apply.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> apply(MemTableEntry entry);

    /**
     * @brief Returns a copy of a live entry.
     *
     * Missing keys and tombstones return std::nullopt.
     *
     * @param key Encoded key.
     * @return Live entry if found.
     */
    [[nodiscard]] std::optional<MemTableEntry>
    get(std::string_view key) const;

    /**
     * @brief Returns a copy of an entry including tombstones.
     *
     * @param key Encoded key.
     * @return Entry if found.
     */
    [[nodiscard]] std::optional<MemTableEntry>
    get_raw(std::string_view key) const;

    /**
     * @brief Returns true if the key exists as a live entry.
     *
     * Tombstones return false.
     *
     * @param key Encoded key.
     * @return true when key exists and is live.
     */
    [[nodiscard]] bool contains(std::string_view key) const;

    /**
     * @brief Returns true if the key exists, including tombstones.
     *
     * @param key Encoded key.
     * @return true when key exists in the map.
     */
    [[nodiscard]] bool contains_raw(std::string_view key) const;

    /**
     * @brief Lists live entries matching an encoded prefix.
     *
     * If prefix is empty, all live entries are returned.
     *
     * @param prefix Encoded key prefix.
     * @return Matching live entries.
     */
    [[nodiscard]] List list(std::string_view prefix = {}) const;

    /**
     * @brief Lists all entries matching an encoded prefix, including tombstones.
     *
     * If prefix is empty, all raw entries are returned.
     *
     * @param prefix Encoded key prefix.
     * @return Matching entries.
     */
    [[nodiscard]] List list_raw(std::string_view prefix = {}) const;

    /**
     * @brief Removes tombstones from the memtable.
     *
     * This should only be called when compaction or snapshots make it safe.
     *
     * @return Number of tombstones removed.
     */
    std::size_t prune_tombstones();

    /**
     * @brief Removes one key completely from the memtable.
     *
     * This removes live entries and tombstones.
     *
     * @param key Encoded key.
     * @return true if an entry was removed.
     */
    bool remove_raw(std::string_view key);

    /**
     * @brief Returns all raw entries.
     *
     * @return Read-only map.
     */
    [[nodiscard]] const Map &entries() const noexcept;

    /**
     * @brief Returns number of live keys.
     *
     * @return Live key count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns number of raw entries including tombstones.
     *
     * @return Raw entry count.
     */
    [[nodiscard]] std::size_t raw_size() const noexcept;

    /**
     * @brief Returns number of tombstones.
     *
     * @return Tombstone count.
     */
    [[nodiscard]] std::size_t tombstone_count() const noexcept;

    /**
     * @brief Returns true if there are no live entries.
     *
     * @return true when size() is zero.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns true if there are no raw entries.
     *
     * @return true when raw_size() is zero.
     */
    [[nodiscard]] bool raw_empty() const noexcept;

    /**
     * @brief Returns approximate owned bytes.
     *
     * @return Approximate memory used by keys and values.
     */
    [[nodiscard]] std::uint64_t byte_size() const noexcept;

    /**
     * @brief Returns last applied sequence.
     *
     * @return Highest accepted sequence.
     */
    [[nodiscard]] std::uint64_t last_sequence() const noexcept;

    /**
     * @brief Clears all entries.
     */
    void clear() noexcept;

  private:
    /**
     * @brief Validates an encoded key.
     *
     * @param key Encoded key.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_key(std::string_view key);

    /**
     * @brief Validates a sequence.
     *
     * @param sequence Sequence number.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_sequence(std::uint64_t sequence);

    /**
     * @brief Returns true if an existing entry is newer than incoming sequence.
     *
     * @param key Encoded key.
     * @param incoming_sequence Incoming sequence.
     * @return true if incoming sequence should be ignored.
     */
    [[nodiscard]] bool has_newer_entry(
        std::string_view key,
        std::uint64_t incoming_sequence) const;

    /**
     * @brief Returns true if a key starts with prefix.
     *
     * @param key Encoded key.
     * @param prefix Encoded prefix.
     * @return true if prefix matches.
     */
    [[nodiscard]] static bool matches_prefix(
        std::string_view key,
        std::string_view prefix) noexcept;

    /**
     * @brief Updates live and tombstone counters after replacing an entry.
     *
     * @param had_existing true if an old entry existed.
     * @param old_deleted old deleted flag.
     * @param new_deleted new deleted flag.
     */
    void update_counts_after_replace(
        bool had_existing,
        bool old_deleted,
        bool new_deleted) noexcept;

    /**
     * @brief Updates last_sequence_ if sequence is newer.
     *
     * @param sequence Sequence number.
     */
    void update_last_sequence(std::uint64_t sequence) noexcept;

  private:
    Map entries_{};
    std::size_t live_count_{0};
    std::size_t tombstone_count_{0};
    std::uint64_t last_sequence_{0};
  };

} // namespace vix::kv::memtable

#endif // VIX_KV_MEMTABLE_MEM_TABLE_HPP
