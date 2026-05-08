/**
 *
 *  @file DataFileReader.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Data file reader
 *
 */

#ifndef VIX_KV_STORAGE_DATA_FILE_READER_HPP
#define VIX_KV_STORAGE_DATA_FILE_READER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordDecoder.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>

namespace vix::kv::storage
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Sequential reader for KV data files.
   *
   * DataFileReader reads records written by DataFileWriter.
   *
   * It is used by:
   * - segment reading
   * - compaction
   * - snapshot loading
   * - diagnostics
   * - tests
   *
   * Rules:
   * - open() must be called before read_next().
   * - records are read sequentially.
   * - partial records are reported as corruption.
   * - checksums are verified by KvRecordDecoder.
   */
  class DataFileReader
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
     * @brief Creates a closed reader.
     */
    DataFileReader() = default;

    /**
     * @brief Creates a reader from a file path.
     *
     * @param path Data file path.
     */
    explicit DataFileReader(std::filesystem::path path);

    /**
     * @brief Non-copyable.
     */
    DataFileReader(const DataFileReader &) = delete;

    /**
     * @brief Non-copyable.
     */
    DataFileReader &operator=(const DataFileReader &) = delete;

    /**
     * @brief Movable.
     */
    DataFileReader(DataFileReader &&other) noexcept;

    /**
     * @brief Movable.
     */
    DataFileReader &operator=(DataFileReader &&other) noexcept;

    /**
     * @brief Closes the reader.
     */
    ~DataFileReader();

    /**
     * @brief Opens the data file for reading.
     *
     * Missing files are treated as NotFound.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open();

    /**
     * @brief Reads the next record.
     *
     * If EOF is reached before a new header starts, this returns NotFound.
     *
     * @return Next record or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecord> read_next();

    /**
     * @brief Reads one record at a specific file offset.
     *
     * The reader must be open.
     *
     * @param offset File offset.
     * @return Record or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecord>
    read_at(std::uint64_t offset);

    /**
     * @brief Reads all records and invokes a callback.
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
     * @brief Seeks to a file offset.
     *
     * @param offset Target offset.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> seek(std::uint64_t offset);

    /**
     * @brief Closes the reader.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Returns true when the reader is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns true when EOF was reached.
     *
     * @return true when EOF reached.
     */
    [[nodiscard]] bool eof() const noexcept;

    /**
     * @brief Returns the file path.
     *
     * @return Data file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns current read offset.
     *
     * @return Current offset.
     */
    [[nodiscard]] std::uint64_t offset() const noexcept;

    /**
     * @brief Returns number of records read.
     *
     * @return Record count.
     */
    [[nodiscard]] std::uint64_t records_read() const noexcept;

    /**
     * @brief Returns number of bytes read.
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
     * @brief Returns an error if reader is not open.
     *
     * @return Success if open.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Moves from another reader.
     *
     * @param other Source reader.
     */
    void move_from(DataFileReader &&other) noexcept;

  private:
    std::filesystem::path path_{};
    std::ifstream stream_{};
    bool open_{false};
    bool eof_{false};
    std::uint64_t offset_{0};
    std::uint64_t records_read_{0};
    std::uint64_t bytes_read_{0};
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_DATA_FILE_READER_HPP
