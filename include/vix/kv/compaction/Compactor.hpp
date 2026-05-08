/**
 *
 *  @file Compactor.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment compactor
 *
 */

#ifndef VIX_KV_COMPACTION_COMPACTOR_HPP
#define VIX_KV_COMPACTION_COMPACTOR_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vix/kv/compaction/CompactionPlan.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/index/KvIndex.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/Segment.hpp>

namespace vix::kv::compaction
{
  namespace core = vix::kv::core;
  namespace index = vix::kv::index;
  namespace memtable = vix::kv::memtable;
  namespace records = vix::kv::records;
  namespace storage = vix::kv::storage;

  /**
   * @brief Result returned after compaction.
   */
  struct CompactionResult
  {
    /**
     * @brief True when compaction completed.
     */
    bool success{false};

    /**
     * @brief Output segment metadata.
     */
    storage::Segment output_segment{};

    /**
     * @brief Number of input segments read.
     */
    std::uint64_t input_segment_count{0};

    /**
     * @brief Number of input records read.
     */
    std::uint64_t input_record_count{0};

    /**
     * @brief Number of records written to the output segment.
     */
    std::uint64_t output_record_count{0};

    /**
     * @brief Number of records skipped because they were obsolete.
     */
    std::uint64_t skipped_obsolete_records{0};

    /**
     * @brief Number of tombstones skipped.
     */
    std::uint64_t skipped_tombstones{0};

    /**
     * @brief Input bytes before compaction.
     */
    std::uint64_t input_bytes{0};

    /**
     * @brief Output bytes after compaction.
     */
    std::uint64_t output_bytes{0};

    /**
     * @brief Highest sequence kept in output.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Returns true if compaction produced an output segment.
     *
     * @return true when output segment is valid.
     */
    [[nodiscard]] bool has_output() const noexcept
    {
      return output_segment.is_valid();
    }

    /**
     * @brief Returns true if compaction reclaimed bytes.
     *
     * @return true when output is smaller than input.
     */
    [[nodiscard]] bool reclaimed_bytes() const noexcept
    {
      return input_bytes > output_bytes;
    }

    /**
     * @brief Returns number of bytes reclaimed.
     *
     * @return Reclaimed bytes.
     */
    [[nodiscard]] std::uint64_t bytes_reclaimed() const noexcept
    {
      if (output_bytes >= input_bytes)
      {
        return 0;
      }

      return input_bytes - output_bytes;
    }

    /**
     * @brief Returns true if the result is structurally valid.
     *
     * @return true when result is valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      if (!success)
      {
        return !has_output();
      }

      return has_output();
    }
  };

  /**
   * @brief Compacts immutable segment files.
   *
   * Compactor reads input segments, keeps only the newest visible state per
   * key, and writes the compacted state to one output segment.
   *
   * Current behavior:
   * - reads records from input segments.
   * - keeps the newest sequence per key.
   * - drops obsolete older versions.
   * - drops tombstones from the output.
   * - writes live latest records to a new segment.
   *
   * Future behavior can keep tombstones based on retention policy.
   */
  class Compactor
  {
  public:
    /**
     * @brief Creates a compactor.
     */
    Compactor() = default;

    /**
     * @brief Runs compaction for a plan.
     *
     * @param plan Compaction plan.
     * @return Compaction result or KvError.
     */
    [[nodiscard]] core::KvResult<CompactionResult>
    compact(const CompactionPlan &plan) const;

    /**
     * @brief Builds a plan for manual compaction.
     *
     * @param input_segments Input segments.
     * @param output_segment_id Output segment id.
     * @param output_path Output segment path.
     * @return Compaction plan.
     */
    [[nodiscard]] static CompactionPlan make_manual_plan(
        std::vector<storage::Segment> input_segments,
        std::uint64_t output_segment_id,
        std::filesystem::path output_path);

    /**
     * @brief Validates a compaction plan.
     *
     * @param plan Plan to validate.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_plan(const CompactionPlan &plan);

  private:
    /**
     * @brief Record selected as latest state for a key.
     */
    struct SelectedRecord
    {
      records::KvRecord record{};
      bool deleted{false};
    };

    /**
     * @brief Map of encoded key to selected latest record.
     */
    using SelectedMap = std::unordered_map<std::string, SelectedRecord>;

    /**
     * @brief Reads all input segments and selects latest records.
     *
     * @param plan Compaction plan.
     * @param selected Output selected records.
     * @param result Result counters.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    read_inputs(
        const CompactionPlan &plan,
        SelectedMap &selected,
        CompactionResult &result);

    /**
     * @brief Considers one record for latest selection.
     *
     * @param record Record read from input.
     * @param selected Selected map.
     * @param result Result counters.
     */
    static void select_record(
        const records::KvRecord &record,
        SelectedMap &selected,
        CompactionResult &result);

    /**
     * @brief Writes selected live records to output segment.
     *
     * @param plan Compaction plan.
     * @param selected Selected records.
     * @param result Result counters.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    write_output(
        const CompactionPlan &plan,
        const SelectedMap &selected,
        CompactionResult &result);

    /**
     * @brief Returns selected records sorted by encoded key.
     *
     * @param selected Selected map.
     * @return Sorted records.
     */
    [[nodiscard]] static std::vector<records::KvRecord>
    sorted_live_records(const SelectedMap &selected);
  };

} // namespace vix::kv::compaction

#endif // VIX_KV_COMPACTION_COMPACTOR_HPP
