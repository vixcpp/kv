/**
 *
 *  @file KvIndex.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  In-memory index over durable record locations
 *
 */

#ifndef VIX_KV_INDEX_KV_INDEX_HPP
#define VIX_KV_INDEX_KV_INDEX_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/index/KvIndexEntry.hpp>

namespace vix::kv::index
{
  namespace core = vix::kv::core;

  /**
   * @brief In-memory index for encoded keys.
   *
   * KvIndex maps an encoded key to the latest known physical record location.
   *
   * It is separate from MemTable:
   * - MemTable stores current values.
   * - KvIndex stores where those values came from on disk.
   *
   * Rules:
   * - keys are encoded before entering the index.
   * - empty keys are invalid.
   * - older sequences must not replace newer sequences.
   * - tombstones are tracked until compaction makes removal safe.
   */
  class KvIndex
  {
  public:
    /**
     * @brief Internal map type.
     */
    using Map = std::unordered_map<std::string, KvIndexEntry>;

    /**
     * @brief List result type.
     */
    using List = std::vector<KvIndexEntry>;

    /**
     * @brief Creates an empty index.
     */
    KvIndex() = default;

    /**
     * @brief Creates an index and reserves initial capacity.
     *
     * @param initial_capacity Initial map capacity.
     */
    explicit KvIndex(std::size_t initial_capacity);

    /**
     * @brief Reserves storage for entries.
     *
     * @param capacity Requested capacity.
     */
    void reserve(std::size_t capacity);

    /**
     * @brief Inserts or updates a live index entry.
     *
     * @param key Encoded key.
     * @param segment_id Segment identifier.
     * @param offset Record offset.
     * @param size Encoded record size.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> put(
        std::string_view key,
        std::uint64_t segment_id,
        std::uint64_t offset,
        std::uint64_t size,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Inserts or updates a tombstone index entry.
     *
     * @param key Encoded key.
     * @param segment_id Segment identifier.
     * @param offset Record offset.
     * @param size Encoded record size.
     * @param sequence Record sequence.
     * @param timestamp_ms Timestamp in milliseconds.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> erase(
        std::string_view key,
        std::uint64_t segment_id,
        std::uint64_t offset,
        std::uint64_t size,
        std::uint64_t sequence,
        std::uint64_t timestamp_ms);

    /**
     * @brief Applies an index entry directly.
     *
     * If the index already contains a newer sequence, the entry is ignored.
     *
     * @param entry Entry to apply.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> apply(KvIndexEntry entry);

    /**
     * @brief Returns a live index entry.
     *
     * Missing keys and tombstones return std::nullopt.
     *
     * @param key Encoded key.
     * @return Live index entry if found.
     */
    [[nodiscard]] std::optional<KvIndexEntry>
    get(std::string_view key) const;

    /**
     * @brief Returns an index entry including tombstones.
     *
     * @param key Encoded key.
     * @return Raw index entry if found.
     */
    [[nodiscard]] std::optional<KvIndexEntry>
    get_raw(std::string_view key) const;

    /**
     * @brief Returns true if key exists as a live entry.
     *
     * Tombstones return false.
     *
     * @param key Encoded key.
     * @return true when key is live.
     */
    [[nodiscard]] bool contains(std::string_view key) const;

    /**
     * @brief Returns true if key exists, including tombstones.
     *
     * @param key Encoded key.
     * @return true when key exists in index.
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
     * If prefix is empty, all entries are returned.
     *
     * @param prefix Encoded key prefix.
     * @return Matching entries.
     */
    [[nodiscard]] List list_raw(std::string_view prefix = {}) const;

    /**
     * @brief Removes tombstone entries.
     *
     * This should only be used after compaction makes removal safe.
     *
     * @return Number of tombstones removed.
     */
    std::size_t prune_tombstones();

    /**
     * @brief Removes one raw entry.
     *
     * This removes live entries and tombstones.
     *
     * @param key Encoded key.
     * @return true if removed.
     */
    bool remove_raw(std::string_view key);

    /**
     * @brief Returns all raw entries.
     *
     * @return Read-only map.
     */
    [[nodiscard]] const Map &entries() const noexcept;

    /**
     * @brief Returns number of live entries.
     *
     * @return Live entry count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns raw entry count including tombstones.
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
     * @brief Validates a physical record size.
     *
     * @param size Encoded record size.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_record_size(std::uint64_t size);

    /**
     * @brief Returns true if existing entry is newer than incoming sequence.
     *
     * @param key Encoded key.
     * @param incoming_sequence Incoming sequence.
     * @return true if incoming entry should be ignored.
     */
    [[nodiscard]] bool has_newer_entry(
        std::string_view key,
        std::uint64_t incoming_sequence) const;

    /**
     * @brief Returns true if a key starts with a prefix.
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
     * @param had_existing true if old entry existed.
     * @param old_deleted old deleted flag.
     * @param new_deleted new deleted flag.
     */
    void update_counts_after_replace(
        bool had_existing,
        bool old_deleted,
        bool new_deleted) noexcept;

    /**
     * @brief Updates last sequence if needed.
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

} // namespace vix::kv::index

#endif // VIX_KV_INDEX_KV_INDEX_HPP
