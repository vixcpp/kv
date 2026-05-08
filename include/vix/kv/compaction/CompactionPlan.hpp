/**
 *
 *  @file CompactionPlan.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Compaction plan
 *
 */

#ifndef VIX_KV_COMPACTION_COMPACTION_PLAN_HPP
#define VIX_KV_COMPACTION_COMPACTION_PLAN_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <vix/kv/storage/Segment.hpp>

namespace vix::kv::compaction
{
  namespace storage = vix::kv::storage;

  /**
   * @brief Reason why a compaction is requested.
   */
  enum class CompactionReason : std::uint8_t
  {
    /**
     * @brief Unknown reason.
     */
    Unknown = 0,

    /**
     * @brief Manual compaction requested by the caller.
     */
    Manual,

    /**
     * @brief Too many segment files exist.
     */
    TooManySegments,

    /**
     * @brief Too many tombstones exist.
     */
    TooManyTombstones,

    /**
     * @brief Segment storage is too large.
     */
    StorageTooLarge,

    /**
     * @brief Snapshot or recovery optimization.
     */
    RecoveryOptimization
  };

  /**
   * @brief Converts a compaction reason to a stable string.
   *
   * @param reason Compaction reason.
   * @return Stable string representation.
   */
  [[nodiscard]] constexpr const char *
  to_string(CompactionReason reason) noexcept
  {
    switch (reason)
    {
    case CompactionReason::Unknown:
      return "unknown";

    case CompactionReason::Manual:
      return "manual";

    case CompactionReason::TooManySegments:
      return "too_many_segments";

    case CompactionReason::TooManyTombstones:
      return "too_many_tombstones";

    case CompactionReason::StorageTooLarge:
      return "storage_too_large";

    case CompactionReason::RecoveryOptimization:
      return "recovery_optimization";

    default:
      return "invalid";
    }
  }

  /**
   * @brief Returns true if the compaction reason is usable.
   *
   * @param reason Compaction reason.
   * @return true when reason is not Unknown.
   */
  [[nodiscard]] constexpr bool
  is_valid(CompactionReason reason) noexcept
  {
    return reason == CompactionReason::Manual ||
           reason == CompactionReason::TooManySegments ||
           reason == CompactionReason::TooManyTombstones ||
           reason == CompactionReason::StorageTooLarge ||
           reason == CompactionReason::RecoveryOptimization;
  }

  /**
   * @brief Plan describing one compaction operation.
   *
   * CompactionPlan tells the compactor:
   * - which segments should be read.
   * - which output segment should be produced.
   * - why the compaction is being performed.
   * - which sequence range is expected.
   *
   * Rules:
   * - output_segment_id should be greater than zero.
   * - output_path should not be empty.
   * - input segments may be empty for memtable-only compaction.
   * - reason should be valid for real compaction work.
   */
  struct CompactionPlan
  {
    /**
     * @brief Segments selected as input.
     */
    std::vector<storage::Segment> input_segments{};

    /**
     * @brief Output segment identifier.
     */
    std::uint64_t output_segment_id{0};

    /**
     * @brief Output segment path.
     */
    std::filesystem::path output_path{};

    /**
     * @brief Lowest sequence expected in the compacted output.
     */
    std::uint64_t min_sequence{0};

    /**
     * @brief Highest sequence expected in the compacted output.
     */
    std::uint64_t max_sequence{0};

    /**
     * @brief Compaction reason.
     */
    CompactionReason reason{CompactionReason::Unknown};

    /**
     * @brief Human-readable note for diagnostics.
     */
    std::string note{};

    /**
     * @brief Creates an empty compaction plan.
     */
    CompactionPlan() = default;

    /**
     * @brief Creates a compaction plan with explicit fields.
     *
     * @param selected_segments Input segments.
     * @param target_segment_id Output segment id.
     * @param target_path Output segment path.
     * @param compaction_reason Reason.
     */
    CompactionPlan(
        std::vector<storage::Segment> selected_segments,
        std::uint64_t target_segment_id,
        std::filesystem::path target_path,
        CompactionReason compaction_reason)
        : input_segments(std::move(selected_segments)),
          output_segment_id(target_segment_id),
          output_path(std::move(target_path)),
          reason(compaction_reason)
    {
      refresh_sequence_range();
    }

    /**
     * @brief Creates a manual compaction plan.
     *
     * @param selected_segments Input segments.
     * @param target_segment_id Output segment id.
     * @param target_path Output segment path.
     * @return Compaction plan.
     */
    [[nodiscard]] static CompactionPlan manual(
        std::vector<storage::Segment> selected_segments,
        std::uint64_t target_segment_id,
        std::filesystem::path target_path)
    {
      return CompactionPlan(
          std::move(selected_segments),
          target_segment_id,
          std::move(target_path),
          CompactionReason::Manual);
    }

    /**
     * @brief Returns true if the plan has input segments.
     *
     * @return true when input_segments is not empty.
     */
    [[nodiscard]] bool has_inputs() const noexcept
    {
      return !input_segments.empty();
    }

    /**
     * @brief Returns true if output segment id is valid.
     *
     * @return true when output_segment_id is greater than zero.
     */
    [[nodiscard]] bool has_output_segment_id() const noexcept
    {
      return output_segment_id > 0;
    }

    /**
     * @brief Returns true if output path is set.
     *
     * @return true when output_path is not empty.
     */
    [[nodiscard]] bool has_output_path() const noexcept
    {
      return !output_path.empty();
    }

    /**
     * @brief Returns true if the plan has a valid reason.
     *
     * @return true when reason is valid.
     */
    [[nodiscard]] bool has_reason() const noexcept
    {
      return compaction::is_valid(reason);
    }

    /**
     * @brief Returns true if the plan has a sequence range.
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
     * @brief Returns total input segment bytes.
     *
     * @return Total bytes across input segments.
     */
    [[nodiscard]] std::uint64_t input_bytes() const noexcept
    {
      std::uint64_t total = 0;

      for (const auto &segment : input_segments)
      {
        total += segment.size_bytes;
      }

      return total;
    }

    /**
     * @brief Returns total input record count.
     *
     * @return Total records across input segments.
     */
    [[nodiscard]] std::uint64_t input_records() const noexcept
    {
      std::uint64_t total = 0;

      for (const auto &segment : input_segments)
      {
        total += segment.record_count;
      }

      return total;
    }

    /**
     * @brief Recomputes min and max sequence from input segments.
     */
    void refresh_sequence_range() noexcept
    {
      min_sequence = 0;
      max_sequence = 0;

      for (const auto &segment : input_segments)
      {
        if (!segment.has_sequence_range())
        {
          continue;
        }

        if (min_sequence == 0 || segment.min_sequence < min_sequence)
        {
          min_sequence = segment.min_sequence;
        }

        if (segment.max_sequence > max_sequence)
        {
          max_sequence = segment.max_sequence;
        }
      }
    }

    /**
     * @brief Adds one input segment and updates sequence range.
     *
     * @param segment Segment to add.
     */
    void add_input(storage::Segment segment)
    {
      if (segment.has_sequence_range())
      {
        if (min_sequence == 0 || segment.min_sequence < min_sequence)
        {
          min_sequence = segment.min_sequence;
        }

        if (segment.max_sequence > max_sequence)
        {
          max_sequence = segment.max_sequence;
        }
      }

      input_segments.push_back(std::move(segment));
    }

    /**
     * @brief Returns true if all input segments are structurally valid.
     *
     * @return true when all input segments are valid.
     */
    [[nodiscard]] bool inputs_are_valid() const noexcept
    {
      for (const auto &segment : input_segments)
      {
        if (!segment.is_valid())
        {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Returns true if the plan is structurally valid.
     *
     * @return true when the plan can be executed.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return has_output_segment_id() &&
             has_output_path() &&
             has_reason() &&
             inputs_are_valid();
    }

    /**
     * @brief Clears the plan.
     */
    void clear() noexcept
    {
      input_segments.clear();
      output_segment_id = 0;
      output_path.clear();
      min_sequence = 0;
      max_sequence = 0;
      reason = CompactionReason::Unknown;
      note.clear();
    }
  };

} // namespace vix::kv::compaction

#endif // VIX_KV_COMPACTION_COMPACTION_PLAN_HPP
