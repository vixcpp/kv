/**
 *
 *  @file DataFileWriter.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Data file writer
 *
 */

#ifndef VIX_KV_STORAGE_DATA_FILE_WRITER_HPP
#define VIX_KV_STORAGE_DATA_FILE_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>

namespace vix::kv::storage
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Result returned after writing one record to a data file.
   */
  struct DataFileWriteResult
  {
    /**
     * @brief Record offset in the file.
     */
    std::uint64_t offset{0};

    /**
     * @brief Encoded record size in bytes.
     */
    std::uint64_t size{0};

    /**
     * @brief Sequence number of the written record.
     */
    std::uint64_t sequence{0};

    /**
     * @brief Returns true if the result points to bytes.
     *
     * @return true when size is greater than zero.
     */
    [[nodiscard]] bool has_size() const noexcept
    {
      return size > 0;
    }

    /**
     * @brief Returns true if the result has a sequence.
     *
     * @return true when sequence is greater than zero.
     */
    [[nodiscard]] bool has_sequence() const noexcept
    {
      return sequence > 0;
    }

    /**
     * @brief Returns true if the result is structurally valid.
     *
     * @return true when size and sequence are valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return has_size() && has_sequence();
    }
  };

  /**
   * @brief Append-only writer for KV data files.
   *
   * DataFileWriter writes encoded KV records to a data file or segment file.
   *
   * It is used by:
   * - segment writer
   * - snapshot writer internals
   * - compaction output
   * - tests
   *
   * Rules:
   * - open() must be called before append().
   * - append() returns the physical offset and size of the record.
   * - flush() should be called before publishing a file.
   * - close() flushes and closes the file.
   */
  class DataFileWriter
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a closed writer.
     */
    DataFileWriter() = default;

    /**
     * @brief Creates a writer from a file path.
     *
     * @param path Data file path.
     */
    explicit DataFileWriter(std::filesystem::path path);

    /**
     * @brief Non-copyable.
     */
    DataFileWriter(const DataFileWriter &) = delete;

    /**
     * @brief Non-copyable.
     */
    DataFileWriter &operator=(const DataFileWriter &) = delete;

    /**
     * @brief Movable.
     */
    DataFileWriter(DataFileWriter &&other) noexcept;

    /**
     * @brief Movable.
     */
    DataFileWriter &operator=(DataFileWriter &&other) noexcept;

    /**
     * @brief Closes the writer.
     */
    ~DataFileWriter();

    /**
     * @brief Opens the data file for append or truncate.
     *
     * @param truncate_existing If true, file is truncated before writing.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open(
        bool truncate_existing = true);

    /**
     * @brief Appends a record.
     *
     * @param record Record to encode and write.
     * @return Write result or KvError.
     */
    [[nodiscard]] core::KvResult<DataFileWriteResult>
    append(const records::KvRecord &record);

    /**
     * @brief Appends already encoded record bytes.
     *
     * @param bytes Encoded record bytes.
     * @param sequence Record sequence.
     * @return Write result or KvError.
     */
    [[nodiscard]] core::KvResult<DataFileWriteResult>
    append_bytes(
        const Bytes &bytes,
        std::uint64_t sequence);

    /**
     * @brief Flushes the file.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush();

    /**
     * @brief Closes the file.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Returns true when the writer is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns the file path.
     *
     * @return Data file path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns the current write offset.
     *
     * @return Current offset.
     */
    [[nodiscard]] std::uint64_t offset() const noexcept;

    /**
     * @brief Returns number of records written.
     *
     * @return Record count.
     */
    [[nodiscard]] std::uint64_t records_written() const noexcept;

    /**
     * @brief Returns number of bytes written.
     *
     * @return Byte count.
     */
    [[nodiscard]] std::uint64_t bytes_written() const noexcept;

  private:
    /**
     * @brief Ensures parent directory exists.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> ensure_parent_directory();

    /**
     * @brief Returns an error if writer is not open.
     *
     * @return Success if open.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Moves from another writer.
     *
     * @param other Source writer.
     */
    void move_from(DataFileWriter &&other) noexcept;

  private:
    std::filesystem::path path_{};
    std::ofstream stream_{};
    bool open_{false};
    std::uint64_t offset_{0};
    std::uint64_t records_written_{0};
    std::uint64_t bytes_written_{0};
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_DATA_FILE_WRITER_HPP
