/**
 *
 *  @file KvWalReader.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL reader
 *
 */

#ifndef VIX_KV_WAL_KV_WAL_READER_HPP
#define VIX_KV_WAL_KV_WAL_READER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordDecoder.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>

namespace vix::kv::wal
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Sequential WAL reader for KV records.
   *
   * KvWalReader reads records written by KvWalWriter.
   *
   * It is used by:
   * - WAL recovery
   * - diagnostics
   * - tests
   * - future compaction tools
   *
   * Rules:
   * - open() must be called before read_next().
   * - records are read sequentially.
   * - partial records are reported as corruption.
   * - checksums are verified by KvRecordDecoder.
   */
  class KvWalReader
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Callback used by for_each().
     */
    using RecordCallback =
        std::function<core::KvResult<void>(const records::KvRecord &)>;

    /**
     * @brief Creates a closed WAL reader.
     */
    KvWalReader() = default;

    /**
     * @brief Creates a WAL reader from a config.
     *
     * @param config KV configuration.
     */
    explicit KvWalReader(core::KvConfig config);

    /**
     * @brief Creates a WAL reader from a path.
     *
     * @param path WAL file path.
     */
    explicit KvWalReader(std::filesystem::path path);

    /**
     * @brief Non-copyable.
     */
    KvWalReader(const KvWalReader &) = delete;

    /**
     * @brief Non-copyable.
     */
    KvWalReader &operator=(const KvWalReader &) = delete;

    /**
     * @brief Movable.
     */
    KvWalReader(KvWalReader &&other) noexcept;

    /**
     * @brief Movable.
     */
    KvWalReader &operator=(KvWalReader &&other) noexcept;

    /**
     * @brief Closes the reader.
     */
    ~KvWalReader();

    /**
     * @brief Opens the WAL file for reading.
     *
     * Missing WAL files are treated as an empty WAL.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open();

    /**
     * @brief Reads the next record.
     *
     * If the end of file is reached before a new header starts, this returns
     * KvResult<KvRecord> with NotFound.
     *
     * @return Next record or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecord> read_next();

    /**
     * @brief Reads all records and invokes a callback for each one.
     *
     * @param callback Callback called for every decoded record.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> for_each(
        const RecordCallback &callback);

    /**
     * @brief Reads all records into memory.
     *
     * @return Vector of records or KvError.
     */
    [[nodiscard]] core::KvResult<std::vector<records::KvRecord>>
    read_all();

    /**
     * @brief Closes the WAL reader.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Returns true if the reader is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns true when the reader reached EOF.
     *
     * @return true when EOF was reached.
     */
    [[nodiscard]] bool eof() const noexcept;

    /**
     * @brief Returns the WAL path.
     *
     * @return WAL file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns number of records read by this reader.
     *
     * @return Record count.
     */
    [[nodiscard]] std::uint64_t records_read() const noexcept;

    /**
     * @brief Returns number of bytes read by this reader.
     *
     * @return Byte count.
     */
    [[nodiscard]] std::uint64_t bytes_read() const noexcept;

  private:
    /**
     * @brief Reads exactly count bytes.
     *
     * EOF before any byte is read returns NotFound.
     * EOF after partial read returns Corruption.
     *
     * @param count Number of bytes to read.
     * @return Bytes or KvError.
     */
    [[nodiscard]] core::KvResult<Bytes> read_exact(std::size_t count);

    /**
     * @brief Reads and decodes one record header.
     *
     * @return Header or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecordHeader>
    read_header();

    /**
     * @brief Reads record payload using the decoded header.
     *
     * @param header Decoded header.
     * @return Payload bytes or KvError.
     */
    [[nodiscard]] core::KvResult<Bytes> read_payload(
        const records::KvRecordHeader &header);

    /**
     * @brief Returns an error when the reader is not open.
     *
     * @return Success if open, error otherwise.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Moves from another reader.
     *
     * @param other Source reader.
     */
    void move_from(KvWalReader &&other) noexcept;

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
    std::ifstream stream_{};
    bool open_{false};
    bool eof_{false};
    std::uint64_t records_read_{0};
    std::uint64_t bytes_read_{0};
  };

} // namespace vix::kv::wal

#endif // VIX_KV_WAL_KV_WAL_READER_HPP
