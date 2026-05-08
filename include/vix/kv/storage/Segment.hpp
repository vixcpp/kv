/**
 *
 *  @file Segment.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment metadata
 *
 */

#ifndef VIX_KV_STORAGE_SEGMENT_HPP
#define VIX_KV_STORAGE_SEGMENT_HPP

#include <cstdint>
#include <filesystem>

namespace vix::kv::storage
{
  /**
   * @brief Metadata for one immutable KV segment file.
   *
   * A segment is a data file containing encoded KV records.
   *
   * It is used by:
   * - SegmentWriter
   * - SegmentReader
   * - KvIndex
   * - compaction
   * - diagnostics
   *
   * Rules:
   * - id should be greater than zero.
   * - path should not be empty.
   * - size_bytes tracks the current file size.
   * - min_sequence and max_sequence describe the record sequence range.
   */
  struct Segment
  {
    /**
     * @brief Segment identifier.
     */
    std::uint64_t id{0};

    /**
     * @brief Segment file path.
     */
    std::filesystem::path path{};

    /**
     * @brief Segment file size in bytes.
     */
    std::uint64_t size_bytes{0};

    /**
     * @brief Number of records stored in the segment.
     */
    std::uint64_t record_count{0};

    /**
     * @brief Lowest sequence number stored in the segment.
     */
    std::uint64_t min_sequence{0};

    /**
     * @brief Highest sequence number stored in the segment.
     */
    std::uint64_t max_sequence{0};

    /**
     * @brief True when the segment has been closed and should be immutable.
     */
    bool sealed{false};

    /**
     * @brief Creates an empty segment.
     */
    Segment() = default;

    /**
     * @brief Creates a segment with id and path.
     *
     * @param segment_id Segment identifier.
     * @param segment_path Segment file path.
     */
    Segment(
        std::uint64_t segment_id,
        std::filesystem::path segment_path)
        : id(segment_id),
          path(std::move(segment_path))
    {
    }

    /**
     * @brief Returns true if the segment has a valid id.
     *
     * @return true when id is greater than zero.
     */
    [[nodiscard]] bool has_id() const noexcept
    {
      return id > 0;
    }

    /**
     * @brief Returns true if the segment has a path.
     *
     * @return true when path is not empty.
     */
    [[nodiscard]] bool has_path() const noexcept
    {
      return !path.empty();
    }

    /**
     * @brief Returns true if the segment contains records.
     *
     * @return true when record_count is greater than zero.
     */
    [[nodiscard]] bool has_records() const noexcept
    {
      return record_count > 0;
    }

    /**
     * @brief Returns true if the segment has a sequence range.
     *
     * @return true when min and max sequence are valid.
     */
    [[nodiscard]] bool has_sequence_range() const noexcept
    {
      return min_sequence > 0 &&
             max_sequence > 0 &&
             min_sequence <= max_sequence;
    }

    /**
     * @brief Returns true if the segment has a file size.
     *
     * @return true when size_bytes is greater than zero.
     */
    [[nodiscard]] bool has_size() const noexcept
    {
      return size_bytes > 0;
    }

    /**
     * @brief Updates the sequence range with one sequence.
     *
     * @param sequence Record sequence.
     */
    void observe_sequence(std::uint64_t sequence) noexcept
    {
      if (sequence == 0)
      {
        return;
      }

      if (min_sequence == 0 || sequence < min_sequence)
      {
        min_sequence = sequence;
      }

      if (sequence > max_sequence)
      {
        max_sequence = sequence;
      }
    }

    /**
     * @brief Updates metadata after writing one record.
     *
     * @param record_size Encoded record size in bytes.
     * @param sequence Record sequence.
     */
    void observe_record(
        std::uint64_t record_size,
        std::uint64_t sequence) noexcept
    {
      size_bytes += record_size;
      ++record_count;
      observe_sequence(sequence);
    }

    /**
     * @brief Marks the segment as sealed.
     */
    void seal() noexcept
    {
      sealed = true;
    }

    /**
     * @brief Returns true if the segment is structurally valid.
     *
     * Empty newly-created segments may be valid before records are written.
     *
     * @return true when id and path are valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      if (!has_id() || !has_path())
      {
        return false;
      }

      if (has_records() && !has_sequence_range())
      {
        return false;
      }

      return true;
    }

    /**
     * @brief Clears the segment metadata.
     */
    void clear() noexcept
    {
      id = 0;
      path.clear();
      size_bytes = 0;
      record_count = 0;
      min_sequence = 0;
      max_sequence = 0;
      sealed = false;
    }
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_SEGMENT_HPP
