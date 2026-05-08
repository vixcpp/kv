/**
 *
 *  @file KvWalWriter.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL writer
 *
 */

#ifndef VIX_KV_WAL_KV_WAL_WRITER_HPP
#define VIX_KV_WAL_KV_WAL_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>

namespace vix::kv::wal
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Append-only WAL writer for KV records.
   *
   * KvWalWriter is responsible for durable append operations.
   *
   * It does not apply records to memory.
   * It only serializes and appends records to the WAL file.
   *
   * Rules:
   * - open() must be called before append().
   * - append() writes one complete encoded record.
   * - flush() should be called when durability matters.
   * - close() flushes and closes the file.
   * - the writer owns only the write side of the WAL.
   */
  class KvWalWriter
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a closed WAL writer.
     */
    KvWalWriter() = default;

    /**
     * @brief Creates a WAL writer from a config.
     *
     * @param config KV configuration.
     */
    explicit KvWalWriter(core::KvConfig config);

    /**
     * @brief Creates a WAL writer from a path.
     *
     * @param path WAL file path.
     */
    explicit KvWalWriter(std::filesystem::path path);

    /**
     * @brief Non-copyable.
     */
    KvWalWriter(const KvWalWriter &) = delete;

    /**
     * @brief Non-copyable.
     */
    KvWalWriter &operator=(const KvWalWriter &) = delete;

    /**
     * @brief Movable.
     */
    KvWalWriter(KvWalWriter &&other) noexcept;

    /**
     * @brief Movable.
     */
    KvWalWriter &operator=(KvWalWriter &&other) noexcept;

    /**
     * @brief Closes the writer.
     */
    ~KvWalWriter();

    /**
     * @brief Opens the WAL file for appending.
     *
     * Creates parent directories when create_directories is enabled.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open();

    /**
     * @brief Appends a record to the WAL.
     *
     * The record is encoded before being written.
     *
     * @param record Record to append.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> append(
        const records::KvRecord &record);

    /**
     * @brief Appends already encoded record bytes to the WAL.
     *
     * @param bytes Encoded record bytes.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> append_bytes(
        const Bytes &bytes);

    /**
     * @brief Flushes pending writes.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush();

    /**
     * @brief Closes the WAL writer.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Returns true if the writer is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns the WAL path.
     *
     * @return WAL file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns number of records written by this writer.
     *
     * @return Record count.
     */
    [[nodiscard]] std::uint64_t records_written() const noexcept;

    /**
     * @brief Returns number of bytes written by this writer.
     *
     * @return Byte count.
     */
    [[nodiscard]] std::uint64_t bytes_written() const noexcept;

  private:
    /**
     * @brief Ensures the WAL parent directory exists.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> ensure_parent_directory();

    /**
     * @brief Returns an error when the writer is not open.
     *
     * @return Success if open, error otherwise.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Moves from another writer.
     *
     * @param other Source writer.
     */
    void move_from(KvWalWriter &&other) noexcept;

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
    std::ofstream stream_{};
    bool open_{false};
    std::uint64_t records_written_{0};
    std::uint64_t bytes_written_{0};
  };

} // namespace vix::kv::wal

#endif // VIX_KV_WAL_KV_WAL_WRITER_HPP
