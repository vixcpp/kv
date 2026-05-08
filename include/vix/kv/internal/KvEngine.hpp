/**
 *
 *  @file KvEngine.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Internal KV engine
 *
 */

#ifndef VIX_KV_INTERNAL_KV_ENGINE_HPP
#define VIX_KV_INTERNAL_KV_ENGINE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/core/KvStats.hpp>
#include <vix/kv/index/KvIndex.hpp>
#include <vix/kv/internal/KvEngineState.hpp>
#include <vix/kv/keys/KeyEncoder.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/memtable/MemTableEntry.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/values/KvValue.hpp>
#include <vix/kv/wal/KvWal.hpp>

namespace vix::kv::internal
{
  namespace core = vix::kv::core;
  namespace index = vix::kv::index;
  namespace keys = vix::kv::keys;
  namespace memtable = vix::kv::memtable;
  namespace records = vix::kv::records;
  namespace values = vix::kv::values;
  namespace wal = vix::kv::wal;

  /**
   * @brief Internal KV engine.
   *
   * KvEngine owns the storage workflow used by the public Kv API.
   *
   * It coordinates:
   * - key encoding
   * - value validation
   * - WAL append
   * - memtable mutation
   * - recovery
   * - stats
   *
   * Current engine mode is WAL + memtable.
   * Segment, snapshot, and compaction infrastructure is prepared for durable
   * production storage evolution.
   */
  class KvEngine
  {
  public:
    /**
     * @brief List result type.
     */
    using ListResult =
        std::vector<std::pair<keys::KeyPath, values::KvValue>>;

    /**
     * @brief Creates an engine with default config.
     */
    KvEngine();

    /**
     * @brief Creates an engine with config.
     *
     * @param config Engine config.
     */
    explicit KvEngine(core::KvConfig config);

    /**
     * @brief Non-copyable.
     */
    KvEngine(const KvEngine &) = delete;

    /**
     * @brief Non-copyable.
     */
    KvEngine &operator=(const KvEngine &) = delete;

    /**
     * @brief Movable.
     */
    KvEngine(KvEngine &&) noexcept = default;

    /**
     * @brief Movable.
     */
    KvEngine &operator=(KvEngine &&) noexcept = default;

    /**
     * @brief Closes the engine.
     */
    ~KvEngine();

    /**
     * @brief Opens the engine and performs recovery if enabled.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open();

    /**
     * @brief Closes the engine.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Flushes durable state.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush();

    /**
     * @brief Stores or replaces a value.
     *
     * @param key Public key path.
     * @param value Value bytes.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> set(
        const keys::KeyPath &key,
        const values::KvValue &value);

    /**
     * @brief Reads a value.
     *
     * Missing keys return NotFound.
     *
     * @param key Public key path.
     * @return Value or KvError.
     */
    [[nodiscard]] core::KvResult<values::KvValue> get(
        const keys::KeyPath &key) const;

    /**
     * @brief Removes a key.
     *
     * Deleting a missing key is treated as NotFound.
     *
     * @param key Public key path.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> erase(
        const keys::KeyPath &key);

    /**
     * @brief Checks whether a key exists as a live entry.
     *
     * @param key Public key path.
     * @return true if key exists.
     */
    [[nodiscard]] bool contains(const keys::KeyPath &key) const;

    /**
     * @brief Lists live entries by prefix.
     *
     * Empty prefix lists all entries.
     *
     * @param prefix Public prefix path.
     * @return List of key/value pairs or KvError.
     */
    [[nodiscard]] core::KvResult<ListResult> list(
        const keys::KeyPath &prefix = {}) const;

    /**
     * @brief Returns number of live keys.
     *
     * @return Live key count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns true if no live key exists.
     *
     * @return true when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns true if the engine is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns engine config.
     *
     * @return Config.
     */
    [[nodiscard]] const core::KvConfig &config() const noexcept;

    /**
     * @brief Returns current stats snapshot.
     *
     * @return Stats.
     */
    [[nodiscard]] core::KvStats stats() const;

    /**
     * @brief Returns raw memtable entries.
     *
     * @return Memtable map.
     */
    [[nodiscard]] const memtable::MemTable::Map &entries() const noexcept;

  private:
    /**
     * @brief Returns success if engine is open.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Returns success if engine is writable.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> require_writable() const;

    /**
     * @brief Encodes a public key path.
     *
     * @param key Public key path.
     * @return Encoded key or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string>
    encode_key(const keys::KeyPath &key);

    /**
     * @brief Decodes an encoded key.
     *
     * @param encoded Encoded key.
     * @return Public key path or KvError.
     */
    [[nodiscard]] static core::KvResult<keys::KeyPath>
    decode_key(const std::string &encoded);

    /**
     * @brief Converts a memtable entry to a KvValue.
     *
     * @param entry Memtable entry.
     * @return KvValue or KvError.
     */
    [[nodiscard]] static core::KvResult<values::KvValue>
    decode_value(const memtable::MemTableEntry &entry);

    /**
     * @brief Builds a Put record.
     *
     * @param encoded_key Encoded key.
     * @param value Value bytes.
     * @return Record.
     */
    [[nodiscard]] records::KvRecord make_put_record(
        std::string encoded_key,
        std::vector<std::uint8_t> value);

    /**
     * @brief Builds a Delete record.
     *
     * @param encoded_key Encoded key.
     * @return Record.
     */
    [[nodiscard]] records::KvRecord make_delete_record(
        std::string encoded_key);

    /**
     * @brief Appends a record to WAL if WAL is enabled.
     *
     * @param record Record to append.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> append_to_wal(
        const records::KvRecord &record);

    /**
     * @brief Applies a record to the in-memory state.
     *
     * @param record Record to apply.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> apply_to_memtable(
        const records::KvRecord &record);

    /**
     * @brief Performs WAL recovery into memtable.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> recover();

    /**
     * @brief Opens WAL writer if enabled.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open_wal();

    /**
     * @brief Updates stats after recovery.
     *
     * @param result Recovery result.
     */
    void observe_recovery(const wal::RecoveryResult &result) noexcept;

    /**
     * @brief Increments error counter.
     */
    void observe_error() noexcept;

  private:
    KvEngineState state_{};
    wal::KvWal wal_{};
  };

} // namespace vix::kv::internal

#endif // VIX_KV_INTERNAL_KV_ENGINE_HPP
