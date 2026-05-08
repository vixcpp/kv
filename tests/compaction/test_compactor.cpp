/**
 *
 *  @file test_compactor.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Compactor unit tests
 *
 */

#include <vix/kv/compaction/CompactionPlan.hpp>
#include <vix/kv/compaction/Compactor.hpp>
#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/FileLayout.hpp>
#include <vix/kv/storage/Segment.hpp>
#include <vix/kv/storage/SegmentReader.hpp>
#include <vix/kv/storage/SegmentWriter.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
  namespace compaction = vix::kv::compaction;
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;
  namespace storage = vix::kv::storage;

  bool expect_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      return false;
    }

    return true;
  }

  template <typename T>
  void print_value(const T &value)
  {
    if constexpr (std::is_enum_v<T>)
    {
      using Underlying = std::underlying_type_t<T>;

      std::cerr << static_cast<Underlying>(value);
    }
    else
    {
      std::cerr << value;
    }
  }

  template <typename T>
  bool expect_eq(
      const T &actual,
      const T &expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';

      std::cerr << "  expected: ";
      print_value(expected);
      std::cerr << '\n';

      std::cerr << "  actual  : ";
      print_value(actual);
      std::cerr << '\n';

      return false;
    }

    return true;
  }

  bool expect_error_code(
      core::KvErrorCode actual,
      core::KvErrorCode expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: "
                << core::to_string(expected)
                << '\n';
      std::cerr << "  actual  : "
                << core::to_string(actual)
                << '\n';
      return false;
    }

    return true;
  }

  std::filesystem::path make_test_root()
  {
    auto root =
        std::filesystem::temp_directory_path() /
        "vix_kv_test_compactor";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    return root;
  }

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  records::KvRecord make_put_record(
      std::uint64_t sequence,
      std::string key,
      std::string value)
  {
    return records::KvRecord::put(
        std::move(key),
        bytes(std::move(value)),
        sequence,
        sequence * 100);
  }

  records::KvRecord make_delete_record(
      std::uint64_t sequence,
      std::string key)
  {
    return records::KvRecord::remove(
        std::move(key),
        sequence,
        sequence * 100);
  }

  core::KvResult<storage::Segment> write_segment(
      const core::KvConfig &config,
      std::uint64_t segment_id,
      const std::vector<records::KvRecord> &items)
  {
    storage::SegmentWriter writer{config, segment_id};

    auto opened = writer.open(true);

    if (opened.is_err())
    {
      return core::KvResult<storage::Segment>::err(
          opened.error());
    }

    for (const auto &record : items)
    {
      auto written = writer.append(record);

      if (written.is_err())
      {
        (void)writer.close();

        return core::KvResult<storage::Segment>::err(
            written.error());
      }
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      return core::KvResult<storage::Segment>::err(
          closed.error());
    }

    return core::KvResult<storage::Segment>::ok(
        writer.segment());
  }

  core::KvResult<std::vector<records::KvRecord>>
  read_segment_records(const storage::Segment &segment)
  {
    storage::SegmentReader reader{segment};

    auto opened = reader.open();

    if (opened.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          opened.error());
    }

    auto records_read = reader.read_all();

    auto closed = reader.close();

    if (closed.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          closed.error());
    }

    if (records_read.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          records_read.error());
    }

    return records_read;
  }

  bool test_reason_helpers()
  {
    return expect_true(
               !compaction::is_valid(
                   compaction::CompactionReason::Unknown),
               "Unknown reason should be invalid") &&
           expect_true(
               compaction::is_valid(
                   compaction::CompactionReason::Manual),
               "Manual reason should be valid") &&
           expect_eq<std::string>(
               compaction::to_string(
                   compaction::CompactionReason::Manual),
               "manual",
               "Manual reason string should be stable") &&
           expect_eq<std::string>(
               compaction::to_string(
                   compaction::CompactionReason::TooManySegments),
               "too_many_segments",
               "TooManySegments reason string should be stable");
  }

  bool test_default_plan_is_invalid()
  {
    const compaction::CompactionPlan plan;

    return expect_true(
               !plan.has_inputs(),
               "default plan should not have inputs") &&
           expect_true(
               !plan.has_output_segment_id(),
               "default plan should not have output segment id") &&
           expect_true(
               !plan.has_output_path(),
               "default plan should not have output path") &&
           expect_true(
               !plan.has_reason(),
               "default plan should not have valid reason") &&
           expect_true(
               !plan.has_sequence_range(),
               "default plan should not have sequence range") &&
           expect_true(
               plan.inputs_are_valid(),
               "empty input list should be structurally valid") &&
           expect_true(
               !plan.is_valid(),
               "default plan should be invalid");
  }

  bool test_manual_plan_fields()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    storage::Segment segment{
        1,
        storage::FileLayout::segment_path(config, 1)};

    segment.observe_record(100, 2);
    segment.observe_record(120, 4);
    segment.seal();

    const auto output_path =
        storage::FileLayout::segment_path(config, 2);

    auto plan = compaction::CompactionPlan::manual(
        std::vector<storage::Segment>{segment},
        2,
        output_path);

    const bool ok =
        expect_true(
            plan.has_inputs(),
            "manual plan should have inputs") &&
        expect_true(
            plan.has_output_segment_id(),
            "manual plan should have output segment id") &&
        expect_true(
            plan.has_output_path(),
            "manual plan should have output path") &&
        expect_true(
            plan.has_reason(),
            "manual plan should have valid reason") &&
        expect_true(
            plan.has_sequence_range(),
            "manual plan should compute sequence range") &&
        expect_true(
            plan.inputs_are_valid(),
            "manual plan inputs should be valid") &&
        expect_true(
            plan.is_valid(),
            "manual plan should be valid") &&
        expect_eq<std::uint64_t>(
            plan.output_segment_id,
            2,
            "manual plan output segment id should match") &&
        expect_eq<std::filesystem::path>(
            plan.output_path,
            output_path,
            "manual plan output path should match") &&
        expect_eq<compaction::CompactionReason>(
            plan.reason,
            compaction::CompactionReason::Manual,
            "manual plan reason should be Manual") &&
        expect_eq<std::uint64_t>(
            plan.min_sequence,
            2,
            "manual plan min sequence should match input segment") &&
        expect_eq<std::uint64_t>(
            plan.max_sequence,
            4,
            "manual plan max sequence should match input segment") &&
        expect_eq<std::uint64_t>(
            plan.input_bytes(),
            segment.size_bytes,
            "manual plan input bytes should sum segments") &&
        expect_eq<std::uint64_t>(
            plan.input_records(),
            segment.record_count,
            "manual plan input records should sum segments");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_plan_add_input_and_clear()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    storage::Segment first{
        1,
        storage::FileLayout::segment_path(config, 1)};

    first.observe_record(100, 3);

    storage::Segment second{
        2,
        storage::FileLayout::segment_path(config, 2)};

    second.observe_record(120, 9);

    compaction::CompactionPlan plan;
    plan.output_segment_id = 3;
    plan.output_path = storage::FileLayout::segment_path(config, 3);
    plan.reason = compaction::CompactionReason::Manual;

    plan.add_input(first);
    plan.add_input(second);

    const bool before_clear =
        plan.input_segments.size() == 2 &&
        plan.min_sequence == 3 &&
        plan.max_sequence == 9 &&
        plan.is_valid();

    plan.clear();

    const bool ok =
        expect_true(
            before_clear,
            "add_input should add segments and refresh sequence range") &&
        expect_true(
            plan.input_segments.empty(),
            "clear should remove input segments") &&
        expect_eq<std::uint64_t>(
            plan.output_segment_id,
            0,
            "clear should reset output segment id") &&
        expect_true(
            plan.output_path.empty(),
            "clear should reset output path") &&
        expect_eq<std::uint64_t>(
            plan.min_sequence,
            0,
            "clear should reset min sequence") &&
        expect_eq<std::uint64_t>(
            plan.max_sequence,
            0,
            "clear should reset max sequence") &&
        expect_eq<compaction::CompactionReason>(
            plan.reason,
            compaction::CompactionReason::Unknown,
            "clear should reset reason") &&
        expect_true(
            plan.note.empty(),
            "clear should reset note");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_validate_plan_rejects_zero_output_segment_id()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto plan = compaction::CompactionPlan::manual(
        {},
        0,
        storage::FileLayout::segment_path(config, 1));

    auto result = compaction::Compactor::validate_plan(plan);

    const bool ok =
        expect_true(
            result.is_err(),
            "validate_plan should reject output segment id 0") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "output segment id 0 should return InvalidArgument");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_validate_plan_rejects_empty_output_path()
  {
    auto plan = compaction::CompactionPlan::manual(
        {},
        1,
        std::filesystem::path{});

    auto result = compaction::Compactor::validate_plan(plan);

    return expect_true(
               result.is_err(),
               "validate_plan should reject empty output path") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::StorageError,
               "empty output path should return StorageError");
  }

  bool test_validate_plan_rejects_unknown_reason()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto plan = compaction::CompactionPlan::manual(
        {},
        1,
        storage::FileLayout::segment_path(config, 1));

    plan.reason = compaction::CompactionReason::Unknown;

    auto result = compaction::Compactor::validate_plan(plan);

    const bool ok =
        expect_true(
            result.is_err(),
            "validate_plan should reject unknown reason") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "unknown reason should return InvalidArgument");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_validate_plan_rejects_invalid_input_segment()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto plan = compaction::CompactionPlan::manual(
        std::vector<storage::Segment>{storage::Segment{}},
        1,
        storage::FileLayout::segment_path(config, 1));

    auto result = compaction::Compactor::validate_plan(plan);

    const bool ok =
        expect_true(
            result.is_err(),
            "validate_plan should reject invalid input segment") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::InvalidArgument,
            "invalid input segment should return InvalidArgument");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_make_manual_plan()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    storage::Segment segment{
        1,
        storage::FileLayout::segment_path(config, 1)};

    segment.observe_record(100, 1);

    const auto output_path =
        storage::FileLayout::segment_path(config, 2);

    auto plan = compaction::Compactor::make_manual_plan(
        std::vector<storage::Segment>{segment},
        2,
        output_path);

    const bool ok =
        expect_true(
            plan.is_valid(),
            "make_manual_plan should create a valid plan") &&
        expect_eq<std::uint64_t>(
            plan.output_segment_id,
            2,
            "make_manual_plan output segment id should match") &&
        expect_eq<std::filesystem::path>(
            plan.output_path,
            output_path,
            "make_manual_plan output path should match") &&
        expect_eq<compaction::CompactionReason>(
            plan.reason,
            compaction::CompactionReason::Manual,
            "make_manual_plan reason should be Manual");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_result_helpers()
  {
    compaction::CompactionResult result;

    const bool initial =
        !result.success &&
        !result.has_output() &&
        !result.reclaimed_bytes() &&
        result.bytes_reclaimed() == 0 &&
        result.is_valid();

    result.success = true;
    result.input_bytes = 300;
    result.output_bytes = 100;
    result.output_segment = storage::Segment{1, "out.segment"};
    result.output_segment.observe_record(100, 5);
    result.output_segment.seal();

    return expect_true(
               initial,
               "default unsuccessful result without output should be valid") &&
           expect_true(
               result.has_output(),
               "successful result should have output") &&
           expect_true(
               result.reclaimed_bytes(),
               "result should report reclaimed bytes") &&
           expect_eq<std::uint64_t>(
               result.bytes_reclaimed(),
               200,
               "bytes_reclaimed should return input minus output") &&
           expect_true(
               result.is_valid(),
               "successful result with output should be valid");
  }

  bool test_compact_keeps_latest_live_record()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto segment_1 = write_segment(
        config,
        1,
        {
            make_put_record(1, "v1|5:hello", "old"),
        });

    auto segment_2 = write_segment(
        config,
        2,
        {
            make_put_record(2, "v1|5:hello", "new"),
        });

    if (!expect_true(
            segment_1.is_ok() && segment_2.is_ok(),
            "input segments should be written"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto output_path =
        storage::FileLayout::segment_path(config, 3);

    auto plan = compaction::Compactor::make_manual_plan(
        std::vector<storage::Segment>{
            segment_1.move_value(),
            segment_2.move_value()},
        3,
        output_path);

    compaction::Compactor compactor;

    auto result = compactor.compact(plan);

    if (!expect_true(result.is_ok(), "compaction should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto records_read = read_segment_records(result.value().output_segment);

    if (!expect_true(
            records_read.is_ok(),
            "output segment should be readable"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            result.value().success,
            "compaction result should be successful") &&
        expect_eq<std::uint64_t>(
            result.value().input_segment_count,
            2,
            "compaction should read two input segments") &&
        expect_eq<std::uint64_t>(
            result.value().input_record_count,
            2,
            "compaction should read two input records") &&
        expect_eq<std::uint64_t>(
            result.value().output_record_count,
            1,
            "compaction should write one latest record") &&
        expect_eq<std::uint64_t>(
            result.value().skipped_obsolete_records,
            1,
            "compaction should skip one obsolete record") &&
        expect_eq<std::uint64_t>(
            result.value().last_sequence,
            2,
            "compaction last sequence should be latest sequence") &&
        expect_true(
            result.value().has_output(),
            "compaction should produce output segment") &&
        expect_eq<std::size_t>(
            records_read.value().size(),
            1,
            "output segment should contain one record") &&
        expect_eq<std::string>(
            records_read.value()[0].key,
            "v1|5:hello",
            "output record key should match") &&
        expect_eq<std::uint64_t>(
            records_read.value()[0].header.sequence,
            2,
            "output record should keep newest sequence") &&
        expect_true(
            records_read.value()[0].value == bytes("new"),
            "output record should keep newest value");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_compact_drops_deleted_latest_record()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto segment_1 = write_segment(
        config,
        1,
        {
            make_put_record(1, "v1|5:hello", "world"),
        });

    auto segment_2 = write_segment(
        config,
        2,
        {
            make_delete_record(2, "v1|5:hello"),
        });

    if (!expect_true(
            segment_1.is_ok() && segment_2.is_ok(),
            "input segments should be written"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto output_path =
        storage::FileLayout::segment_path(config, 3);

    auto plan = compaction::Compactor::make_manual_plan(
        std::vector<storage::Segment>{
            segment_1.move_value(),
            segment_2.move_value()},
        3,
        output_path);

    compaction::Compactor compactor;

    auto result = compactor.compact(plan);

    if (!expect_true(result.is_ok(), "compaction should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto records_read = read_segment_records(result.value().output_segment);

    if (!expect_true(
            records_read.is_ok(),
            "output segment should be readable"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::uint64_t>(
            result.value().input_record_count,
            2,
            "compaction should read put and delete") &&
        expect_eq<std::uint64_t>(
            result.value().output_record_count,
            0,
            "compaction should write no record when latest state is delete") &&
        expect_eq<std::uint64_t>(
            result.value().skipped_obsolete_records,
            1,
            "older put should be obsolete") &&
        expect_eq<std::uint64_t>(
            records_read.value().size(),
            0,
            "output segment should contain no live records");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_compact_sorts_live_records_by_key()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    auto segment = write_segment(
        config,
        1,
        {
            make_put_record(2, "b", "two"),
            make_put_record(1, "a", "one"),
            make_put_record(3, "c", "three"),
        });

    if (!expect_true(segment.is_ok(), "input segment should be written"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const auto output_path =
        storage::FileLayout::segment_path(config, 2);

    auto plan = compaction::Compactor::make_manual_plan(
        std::vector<storage::Segment>{segment.move_value()},
        2,
        output_path);

    compaction::Compactor compactor;

    auto result = compactor.compact(plan);

    if (!expect_true(result.is_ok(), "compaction should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto records_read = read_segment_records(result.value().output_segment);

    if (!expect_true(
            records_read.is_ok(),
            "output segment should be readable"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_eq<std::size_t>(
            records_read.value().size(),
            3,
            "output segment should contain all live records") &&
        expect_eq<std::string>(
            records_read.value()[0].key,
            "a",
            "output records should be sorted by key") &&
        expect_eq<std::string>(
            records_read.value()[1].key,
            "b",
            "output records should be sorted by key") &&
        expect_eq<std::string>(
            records_read.value()[2].key,
            "c",
            "output records should be sorted by key");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_compact_empty_inputs_creates_empty_output_segment()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    const auto output_path =
        storage::FileLayout::segment_path(config, 1);

    auto plan = compaction::Compactor::make_manual_plan(
        {},
        1,
        output_path);

    compaction::Compactor compactor;

    auto result = compactor.compact(plan);

    if (!expect_true(result.is_ok(), "empty input compaction should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            result.value().success,
            "empty input compaction should be successful") &&
        expect_eq<std::uint64_t>(
            result.value().input_segment_count,
            0,
            "empty input compaction should read 0 segments") &&
        expect_eq<std::uint64_t>(
            result.value().input_record_count,
            0,
            "empty input compaction should read 0 records") &&
        expect_eq<std::uint64_t>(
            result.value().output_record_count,
            0,
            "empty input compaction should write 0 records") &&
        expect_true(
            result.value().has_output(),
            "empty input compaction should still produce output segment") &&
        expect_true(
            std::filesystem::exists(output_path),
            "empty output segment file should exist");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_compact_missing_input_segment_returns_error()
  {
    const auto root = make_test_root();
    auto config = core::KvConfig::durable(root);

    storage::Segment missing{
        1,
        storage::FileLayout::segment_path(config, 1)};

    missing.observe_record(100, 1);
    missing.seal();

    auto plan = compaction::Compactor::make_manual_plan(
        std::vector<storage::Segment>{missing},
        2,
        storage::FileLayout::segment_path(config, 2));

    compaction::Compactor compactor;

    auto result = compactor.compact(plan);

    const bool ok =
        expect_true(
            result.is_err(),
            "compaction with missing input segment should fail") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::NotFound,
            "missing input segment should return NotFound");

    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_reason_helpers())
  {
    return 1;
  }

  if (!test_default_plan_is_invalid())
  {
    return 1;
  }

  if (!test_manual_plan_fields())
  {
    return 1;
  }

  if (!test_plan_add_input_and_clear())
  {
    return 1;
  }

  if (!test_validate_plan_rejects_zero_output_segment_id())
  {
    return 1;
  }

  if (!test_validate_plan_rejects_empty_output_path())
  {
    return 1;
  }

  if (!test_validate_plan_rejects_unknown_reason())
  {
    return 1;
  }

  if (!test_validate_plan_rejects_invalid_input_segment())
  {
    return 1;
  }

  if (!test_make_manual_plan())
  {
    return 1;
  }

  if (!test_result_helpers())
  {
    return 1;
  }

  if (!test_compact_keeps_latest_live_record())
  {
    return 1;
  }

  if (!test_compact_drops_deleted_latest_record())
  {
    return 1;
  }

  if (!test_compact_sorts_live_records_by_key())
  {
    return 1;
  }

  if (!test_compact_empty_inputs_creates_empty_output_segment())
  {
    return 1;
  }

  if (!test_compact_missing_input_segment_returns_error())
  {
    return 1;
  }

  std::cout << "kv_test_compactor passed\n";
  return 0;
}
