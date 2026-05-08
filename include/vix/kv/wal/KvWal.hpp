/**
 *
 *  @file KvWal.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL facade
 *
 */

#ifndef VIX_KV_WAL_KV_WAL_HPP
#define VIX_KV_WAL_KV_WAL_HPP

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/wal/KvWalReader.hpp>
#include <vix/kv/wal/KvWalRecovery.hpp>
#include <vix/kv/wal/KvWalWriter.hpp>

namespace vix::kv::wal
{
  namespace core = vix::kv::core;
  namespace memtable = vix::kv::memtable;
  namespace records = vix::kv::records;

  /**
   * @brief High-level WAL facade.
   *
   * KvWal groups the common WAL operations used by KvEngine:
   * - open writer
   * - append records
   * - flush records
   * - close writer
   * - read records
   * - recover into a memtable
   *
   * KvWal does not apply records to the engine by itself except through
   * recover(), which delegates to KvWalRecovery.
   *
   * Rules:
   * - open() must be called before append().
   * - append() is write-only.
   * - recover() reads WAL from disk into a given MemTable.
   * - missing WAL file is treated as empty recovery.
   */
  class KvWal
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a WAL facade with default configuration.
     */
    KvWal() = default;

    /**
     * @brief Creates a WAL facade from a config.
     *
     * @param config KV configuration.
     */
    explicit KvWal(core::KvConfig config)
        : config_(std::move(config)),
          path_(config_.wal_path),
          writer_(config_)
    {
    }

    /**
     * @brief Creates a WAL facade from a WAL path.
     *
     * @param wal_path WAL file path.
     */
    explicit KvWal(std::filesystem::path wal_path)
        : path_(std::move(wal_path)),
          writer_(path_)
    {
      config_ = core::KvConfig::durable(
          path_.has_parent_path()
              ? path_.parent_path().parent_path()
              : std::filesystem::path{"data/kv"});

      config_.wal_path = path_;
    }

    /**
     * @brief Non-copyable.
     */
    KvWal(const KvWal &) = delete;

    /**
     * @brief Non-copyable.
     */
    KvWal &operator=(const KvWal &) = delete;

    /**
     * @brief Movable.
     */
    KvWal(KvWal &&) noexcept = default;

    /**
     * @brief Movable.
     */
    KvWal &operator=(KvWal &&) noexcept = default;

    /**
     * @brief Default destructor.
     */
    ~KvWal() = default;

    /**
     * @brief Opens the WAL writer.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open()
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<void>::err(
            core::KvError::make(
                core::KvErrorCode::Unsupported,
                "WAL is disabled"));
      }

      return writer_.open();
    }

    /**
     * @brief Appends a record to the WAL.
     *
     * @param record Record to append.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> append(
        const records::KvRecord &record)
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<void>::err(
            core::KvError::make(
                core::KvErrorCode::Unsupported,
                "cannot append record because WAL is disabled"));
      }

      return writer_.append(record);
    }

    /**
     * @brief Appends already encoded record bytes to the WAL.
     *
     * @param bytes Encoded record bytes.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> append_bytes(
        const Bytes &bytes)
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<void>::err(
            core::KvError::make(
                core::KvErrorCode::Unsupported,
                "cannot append bytes because WAL is disabled"));
      }

      return writer_.append_bytes(bytes);
    }

    /**
     * @brief Flushes the WAL writer.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush()
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<void>::ok();
      }

      return writer_.flush();
    }

    /**
     * @brief Closes the WAL writer.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close()
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<void>::ok();
      }

      return writer_.close();
    }

    /**
     * @brief Recovers WAL records into a memtable.
     *
     * This method can be called before opening the writer.
     *
     * @param table Target memtable.
     * @return Recovery result or KvError.
     */
    [[nodiscard]] core::KvResult<RecoveryResult> recover(
        memtable::MemTable &table) const
    {
      if (!config_.enable_wal || !config_.recover_on_open)
      {
        return core::KvResult<RecoveryResult>::ok(RecoveryResult{});
      }

      KvWalRecovery recovery{config_};

      return recovery.recover(table);
    }

    /**
     * @brief Reads all WAL records.
     *
     * @return Vector of records or KvError.
     */
    [[nodiscard]] core::KvResult<std::vector<records::KvRecord>>
    read_all() const
    {
      if (!config_.enable_wal)
      {
        return core::KvResult<std::vector<records::KvRecord>>::ok({});
      }

      KvWalReader reader{config_};

      auto opened = reader.open();

      if (opened.is_err())
      {
        return core::KvResult<std::vector<records::KvRecord>>::err(
            opened.error());
      }

      auto records = reader.read_all();

      auto closed = reader.close();

      if (closed.is_err())
      {
        return core::KvResult<std::vector<records::KvRecord>>::err(
            closed.error());
      }

      return records;
    }

    /**
     * @brief Returns true if the WAL writer is open.
     *
     * @return true when writer is open.
     */
    [[nodiscard]] bool is_open() const noexcept
    {
      return writer_.is_open();
    }

    /**
     * @brief Returns true if WAL is enabled.
     *
     * @return true when WAL is enabled.
     */
    [[nodiscard]] bool enabled() const noexcept
    {
      return config_.enable_wal;
    }

    /**
     * @brief Returns the WAL path.
     *
     * @return WAL file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
      return path_;
    }

    /**
     * @brief Returns the WAL config.
     *
     * @return KV config.
     */
    [[nodiscard]] const core::KvConfig &config() const noexcept
    {
      return config_;
    }

    /**
     * @brief Returns number of records written by the writer.
     *
     * @return Record count.
     */
    [[nodiscard]] std::uint64_t records_written() const noexcept
    {
      return writer_.records_written();
    }

    /**
     * @brief Returns number of bytes written by the writer.
     *
     * @return Byte count.
     */
    [[nodiscard]] std::uint64_t bytes_written() const noexcept
    {
      return writer_.bytes_written();
    }

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
    KvWalWriter writer_{};
  };

} // namespace vix::kv::wal

#endif // VIX_KV_WAL_KV_WAL_HPP
