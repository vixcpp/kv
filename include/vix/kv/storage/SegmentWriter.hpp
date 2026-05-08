/**
 *
 *  @file SegmentWriter.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment writer
 *
 */

#ifndef VIX_KV_STORAGE_SEGMENT_WRITER_HPP
#define VIX_KV_STORAGE_SEGMENT_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/DataFileWriter.hpp>
#include <vix/kv/storage/FileLayout.hpp>
#include <vix/kv/storage/Segment.hpp>

namespace vix::kv::storage
{
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;

  /**
   * @brief Result returned after writing one record to a segment.
   */
  struct SegmentWriteResult
  {
    /**
     * @brief Segment identifier.
     */
    std::uint64_t segment_id{0};

    /**
     * @brief Record offset inside the segment.
     */
    std::uint64_t offset{0};

    /**
     * @brief Encoded record size in bytes.
     */
    std::uint64_t size{0};

    /**
     * @brief Record sequence.
     */
    std::uint64_t sequence{0};

    /**
     * @brief Returns true if the result has a segment id.
     *
     * @return true when segment_id is greater than zero.
     */
    [[nodiscard]] bool has_segment_id() const noexcept
    {
      return segment_id > 0;
    }

    /**
     * @brief Returns true if the result has a size.
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
     * @return true when segment id, size, and sequence are valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return has_segment_id() &&
             has_size() &&
             has_sequence();
    }
  };

  /**
   * @brief Writer for immutable KV segment files.
   *
   * SegmentWriter writes records to one segment file and tracks segment
   * metadata while writing.
   *
   * It is used by:
   * - compaction
   * - snapshot export
   * - future flush from WAL/memtable to segment storage
   *
   * Rules:
   * - open() must be called before append().
   * - segment id must be greater than zero.
   * - close() seals the segment metadata.
   * - records are append-only.
   */
  class SegmentWriter
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a closed segment writer.
     */
    SegmentWriter() = default;

    /**
     * @brief Creates a segment writer from config and segment id.
     *
     * @param config KV configuration.
     * @param segment_id Segment identifier.
     */
    SegmentWriter(
        core::KvConfig config,
        std::uint64_t segment_id);

    /**
     * @brief Creates a segment writer from explicit metadata.
     *
     * @param segment Segment metadata.
     */
    explicit SegmentWriter(Segment segment);

    /**
     * @brief Non-copyable.
     */
    SegmentWriter(const SegmentWriter &) = delete;

    /**
     * @brief Non-copyable.
     */
    SegmentWriter &operator=(const SegmentWriter &) = delete;

    /**
     * @brief Movable.
     */
    SegmentWriter(SegmentWriter &&other) noexcept;

    /**
     * @brief Movable.
     */
    SegmentWriter &operator=(SegmentWriter &&other) noexcept;

    /**
     * @brief Closes the segment writer.
     */
    ~SegmentWriter();

    /**
     * @brief Opens the segment file.
     *
     * @param truncate_existing If true, existing file is truncated.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open(
        bool truncate_existing = true);

    /**
     * @brief Appends one record to the segment.
     *
     * @param record Record to write.
     * @return Segment write result or KvError.
     */
    [[nodiscard]] core::KvResult<SegmentWriteResult>
    append(const records::KvRecord &record);

    /**
     * @brief Appends already encoded bytes to the segment.
     *
     * @param bytes Encoded record bytes.
     * @param sequence Record sequence.
     * @return Segment write result or KvError.
     */
    [[nodiscard]] core::KvResult<SegmentWriteResult>
    append_bytes(
        const Bytes &bytes,
        std::uint64_t sequence);

    /**
     * @brief Flushes the segment file.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush();

    /**
     * @brief Closes and seals the segment.
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
     * @brief Returns the current segment metadata.
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
     * @brief Returns the segment id.
     *
     * @return Segment id.
     */
    [[nodiscard]] std::uint64_t segment_id() const noexcept;

    /**
     * @brief Returns the segment path.
     *
     * @return Segment path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    /**
     * @brief Returns current write offset.
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
     * @brief Validates segment metadata before opening.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> validate_segment() const;

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
    void move_from(SegmentWriter &&other) noexcept;

  private:
    core::KvConfig config_{};
    Segment segment_{};
    DataFileWriter writer_{};
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_SEGMENT_WRITER_HPP
