/**
 *
 *  @file KvWalRecovery.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL recovery
 *
 */

#ifndef VIX_KV_WAL_KV_WAL_RECOVERY_HPP
#define VIX_KV_WAL_KV_WAL_RECOVERY_HPP

#include <cstdint>
#include <filesystem>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/wal/KvWalReader.hpp>

namespace vix::kv::wal
{
  namespace core = vix::kv::core;
  namespace memtable = vix::kv::memtable;
  namespace records = vix::kv::records;

  /**
   * @brief Result returned after WAL recovery.
   *
   * RecoveryResult describes what happened while replaying the WAL.
   */
  struct RecoveryResult
  {
    /**
     * @brief Number of records successfully recovered.
     */
    std::uint64_t records_recovered{0};

    /**
     * @brief Number of records skipped.
     *
     * This is reserved for future tolerant recovery modes.
     */
    std::uint64_t records_skipped{0};

    /**
     * @brief Number of corrupted records detected.
     */
    std::uint64_t records_corrupted{0};

    /**
     * @brief Last recovered sequence number.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Number of live keys after recovery.
     */
    std::uint64_t live_keys{0};

    /**
     * @brief Number of tombstones after recovery.
     */
    std::uint64_t tombstones{0};

    /**
     * @brief Number of bytes read from WAL.
     */
    std::uint64_t bytes_read{0};

    /**
     * @brief Returns true if at least one record was recovered.
     *
     * @return true when records_recovered is greater than zero.
     */
    [[nodiscard]] bool recovered_any() const noexcept
    {
      return records_recovered > 0;
    }

    /**
     * @brief Returns true if corruption was detected.
     *
     * @return true when records_corrupted is greater than zero.
     */
    [[nodiscard]] bool has_corruption() const noexcept
    {
      return records_corrupted > 0;
    }

    /**
     * @brief Returns true if records were skipped.
     *
     * @return true when records_skipped is greater than zero.
     */
    [[nodiscard]] bool has_skipped_records() const noexcept
    {
      return records_skipped > 0;
    }
  };

  /**
   * @brief Replays WAL records into a MemTable.
   *
   * KvWalRecovery rebuilds the current in-memory state by reading WAL records
   * sequentially and applying them in sequence order.
   *
   * Rules:
   * - missing WAL file is treated as empty recovery.
   * - Put records create or update live entries.
   * - Delete records create tombstones.
   * - Snapshot and Compaction records are ignored for now.
   * - corrupted records return an explicit error.
   */
  class KvWalRecovery
  {
  public:
    /**
     * @brief Creates recovery from a config.
     *
     * @param config KV configuration.
     */
    explicit KvWalRecovery(core::KvConfig config);

    /**
     * @brief Creates recovery from a WAL path.
     *
     * @param wal_path WAL file path.
     */
    explicit KvWalRecovery(std::filesystem::path wal_path);

    /**
     * @brief Replays the WAL into the given memtable.
     *
     * @param table Target memtable.
     * @return Recovery result or KvError.
     */
    [[nodiscard]] core::KvResult<RecoveryResult>
    recover(memtable::MemTable &table);

    /**
     * @brief Returns the WAL path.
     *
     * @return WAL file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

  private:
    /**
     * @brief Applies one recovered record into the memtable.
     *
     * @param record Recovered record.
     * @param table Target memtable.
     * @param result Recovery counters.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    apply_record(
        const records::KvRecord &record,
        memtable::MemTable &table,
        RecoveryResult &result);

    /**
     * @brief Applies a Put record.
     *
     * @param record Recovered record.
     * @param table Target memtable.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    apply_put(
        const records::KvRecord &record,
        memtable::MemTable &table);

    /**
     * @brief Applies a Delete record.
     *
     * @param record Recovered record.
     * @param table Target memtable.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    apply_delete(
        const records::KvRecord &record,
        memtable::MemTable &table);

    /**
     * @brief Updates recovery counters after a successful record.
     *
     * @param record Recovered record.
     * @param result Recovery result.
     */
    static void update_result(
        const records::KvRecord &record,
        RecoveryResult &result) noexcept;

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
  };

} // namespace vix::kv::wal

#endif // VIX_KV_WAL_KV_WAL_RECOVERY_HPP
