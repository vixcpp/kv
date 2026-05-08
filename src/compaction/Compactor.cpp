/**
 *
 *  @file Compactor.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment compactor implementation
 *
 */

#include <vix/kv/compaction/Compactor.hpp>

#include <algorithm>
#include <utility>

#include <vix/kv/storage/SegmentReader.hpp>
#include <vix/kv/storage/SegmentWriter.hpp>

namespace vix::kv::compaction
{
  core::KvResult<CompactionResult>
  Compactor::compact(const CompactionPlan &plan) const
  {
    auto validation = validate_plan(plan);

    if (validation.is_err())
    {
      return core::KvResult<CompactionResult>::err(
          validation.error());
    }

    CompactionResult result;
    result.input_segment_count =
        static_cast<std::uint64_t>(plan.input_segments.size());
    result.input_bytes = plan.input_bytes();

    SelectedMap selected;

    auto read_result = read_inputs(plan, selected, result);

    if (read_result.is_err())
    {
      return core::KvResult<CompactionResult>::err(
          read_result.error());
    }

    auto write_result = write_output(plan, selected, result);

    if (write_result.is_err())
    {
      return core::KvResult<CompactionResult>::err(
          write_result.error());
    }

    result.success = true;

    return core::KvResult<CompactionResult>::ok(std::move(result));
  }

  CompactionPlan Compactor::make_manual_plan(
      std::vector<storage::Segment> input_segments,
      std::uint64_t output_segment_id,
      std::filesystem::path output_path)
  {
    return CompactionPlan::manual(
        std::move(input_segments),
        output_segment_id,
        std::move(output_path));
  }

  core::KvResult<void>
  Compactor::validate_plan(const CompactionPlan &plan)
  {
    if (!plan.has_output_segment_id())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "compaction output segment id must be greater than zero"));
    }

    if (!plan.has_output_path())
    {
      return core::KvResult<void>::err(
          core::KvError::storage(
              "compaction output path must not be empty"));
    }

    if (!plan.has_reason())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "compaction reason is invalid"));
    }

    if (!plan.inputs_are_valid())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "compaction plan contains invalid input segments"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  Compactor::read_inputs(
      const CompactionPlan &plan,
      SelectedMap &selected,
      CompactionResult &result)
  {
    for (const auto &segment : plan.input_segments)
    {
      storage::SegmentReader reader{segment};

      auto opened = reader.open();

      if (opened.is_err())
      {
        return opened;
      }

      auto read_result = reader.for_each(
          [&](const records::KvRecord &record)
          {
            select_record(record, selected, result);
            return core::KvResult<void>::ok();
          });

      auto closed = reader.close();

      if (closed.is_err())
      {
        return closed;
      }

      if (read_result.is_err())
      {
        return read_result;
      }
    }

    return core::KvResult<void>::ok();
  }

  void Compactor::select_record(
      const records::KvRecord &record,
      SelectedMap &selected,
      CompactionResult &result)
  {
    ++result.input_record_count;

    const auto found = selected.find(record.key);

    if (found != selected.end() &&
        found->second.record.header.sequence >= record.header.sequence)
    {
      ++result.skipped_obsolete_records;
      return;
    }

    if (found != selected.end())
    {
      ++result.skipped_obsolete_records;
    }

    SelectedRecord selected_record;
    selected_record.record = record;
    selected_record.deleted = record.is_delete();

    selected[record.key] = std::move(selected_record);

    if (record.header.sequence > result.last_sequence)
    {
      result.last_sequence = record.header.sequence;
    }
  }

  core::KvResult<void>
  Compactor::write_output(
      const CompactionPlan &plan,
      const SelectedMap &selected,
      CompactionResult &result)
  {
    storage::Segment output_segment{
        plan.output_segment_id,
        plan.output_path};

    storage::SegmentWriter writer{std::move(output_segment)};

    auto opened = writer.open(true);

    if (opened.is_err())
    {
      return opened;
    }

    const auto live_records = sorted_live_records(selected);

    for (const auto &record : live_records)
    {
      auto written = writer.append(record);

      if (written.is_err())
      {
        (void)writer.close();
        return core::KvResult<void>::err(written.error());
      }

      ++result.output_record_count;
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      return closed;
    }

    result.output_segment = writer.segment();
    result.output_bytes = writer.bytes_written();

    return core::KvResult<void>::ok();
  }

  std::vector<records::KvRecord>
  Compactor::sorted_live_records(const SelectedMap &selected)
  {
    std::vector<records::KvRecord> records;
    records.reserve(selected.size());

    for (const auto &[_, selected_record] : selected)
    {
      if (selected_record.deleted)
      {
        continue;
      }

      records.push_back(selected_record.record);
    }

    std::sort(
        records.begin(),
        records.end(),
        [](const records::KvRecord &a, const records::KvRecord &b)
        {
          return a.key < b.key;
        });

    return records;
  }

} // namespace vix::kv::compaction
