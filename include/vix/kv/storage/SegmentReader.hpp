/**
 *
 *  @file SegmentReader.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment reader
 *
 */

#ifndef VIX_KV_STORAGE_SEGMENT_READER_HPP
#define VIX_KV_STORAGE_SEGMENT_READER_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/DataFileReader.hpp>
#include <vix/kv/storage/Segment.hpp>

namespace vix::kv::storage
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Reader for immutable KV segment files.
   *
   * SegmentReader reads records from one segment file.
   *
   * It is used by:
   * - compaction
   * - index rebuild
   * - diagnostics
   * - tests
   *
   * Rules:
   * - open() must be called before read_next().
   * - records are read sequentially by default.
   * - read_at() can read one record at a specific offset.
   * - checksums are verified by the lower DataFileReader.
   */
  class SegmentReader
  {
  public:
    /**
     * @brief Callback used by for_each().
     */
    using RecordCallback =
        std::function<core::KvResult<void>(const records::KvRecord &)>;

    /**
     * @brief Creates a closed segment reader.
     */
    SegmentReader() = default;

    /**
     * @brief Creates a reader from segment metadata.
     *
     * @param segment Segment metadata.
     */
    explicit SegmentReader(Segment segment);

    /**
     * @brief Creates a reader from explicit path.
     *
     * @param path Segment file path.
     */
    explicit SegmentReader(std::filesystem::path path);

    /**
     * @brief Non-copyable.
     */
    SegmentReader(const SegmentReader &) = delete;

    /**
     * @brief Non-copyable.
     */
    SegmentReader &operator=(const SegmentReader &) = delete;

    /**
     * @brief Movable.
     */
    SegmentReader(SegmentReader &&other) noexcept;

    /**
     * @brief Movable.
     */
    SegmentReader &operator=(SegmentReader &&other) noexcept;

    /**
     * @brief Closes the reader.
     */
    ~SegmentReader();

    /**
     * @brief Opens the segment file.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open();

    /**
     * @brief Reads the next record.
     *
     * @return Record or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecord> read_next();

    /**
     * @brief Reads one record at a specific segment offset.
     *
     * @param offset Offset inside the segment file.
     * @return Record or KvError.
     */
    [[nodiscard]] core::KvResult<records::KvRecord>
    read_at(std::uint64_t offset);

    /**
     * @brief Reads every record and invokes a callback.
     *
     * @param callback Callback called for each decoded record.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> for_each(
        const RecordCallback &callback);

    /**
     * @brief Reads all records into memory.
     *
     * @return Records or KvError.
     */
    [[nodiscard]] core::KvResult<std::vector<records::KvRecord>>
    read_all();

    /**
     * @brief Seeks to a segment offset.
     *
     * @param offset Segment offset.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> seek(std::uint64_t offset);

    /**
     * @brief Closes the segment reader.
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
     * @brief Returns true if EOF was reached.
     *
     * @return true when EOF was reached.
     */
    [[nodiscard]] bool eof() const noexcept;

    /**
     * @brief Returns the segment metadata.
     *
     * @return Segment metadata.
     */
    [[nodiscard]] const Segment &segment() const noexcept;

    /**
     * @brief Returns mutable segment metadata.
     *
     * @return Mutable segment metadata.
     */
    [[nodiscard]] Segment &segment() noexcept;

    /**
     * @brief Returns the segment path.
     *
     * @return Segment file path.
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
     * @brief Validates segment metadata before opening.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> validate_segment() const;

    /**
     * @brief Returns an error if reader is not open.
     *
     * @return Success if open.
     */
    [[nodiscard]] core::KvResult<void> require_open() const;

    /**
     * @brief Updates segment metadata after reading a record.
     *
     * @param record Read record.
     */
    void observe_record(const records::KvRecord &record) noexcept;

    /**
     * @brief Moves from another reader.
     *
     * @param other Source reader.
     */
    void move_from(SegmentReader &&other) noexcept;

  private:
    Segment segment_{};
    DataFileReader reader_{};
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_SEGMENT_READER_HPP
