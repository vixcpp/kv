/**
 *
 *  @file compact.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment compaction example
 *
 */

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <vix/kv/compaction/Compactor.hpp>
#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/keys/KeyEncoder.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/FileLayout.hpp>
#include <vix/kv/storage/SegmentWriter.hpp>
#include <vix/kv/values/KvValue.hpp>
#include <vix/kv/values/ValueCodec.hpp>

namespace
{
  namespace compaction = vix::kv::compaction;
  namespace core = vix::kv::core;
  namespace keys = vix::kv::keys;
  namespace records = vix::kv::records;
  namespace storage = vix::kv::storage;
  namespace values = vix::kv::values;

  int fail(const std::string &message)
  {
    std::cerr << message << '\n';
    return 1;
  }

  keys::KeyPath key_path(std::vector<std::string> parts)
  {
    return keys::KeyPath(std::move(parts));
  }

  core::KvResult<std::string> encode_key(
      const keys::KeyPath &key)
  {
    return keys::KeyEncoder::encode(key);
  }

  core::KvResult<std::vector<std::uint8_t>> encode_value(
      std::string_view value)
  {
    return values::ValueCodec::encode(
        values::KvValue::from_string(value));
  }

  core::KvResult<records::KvRecord> make_put(
      const keys::KeyPath &key,
      std::string_view value,
      std::uint64_t sequence)
  {
    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          encoded_key.error());
    }

    auto encoded_value = encode_value(value);

    if (encoded_value.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          encoded_value.error());
    }

    return core::KvResult<records::KvRecord>::ok(
        records::KvRecord::put(
            encoded_key.move_value(),
            encoded_value.move_value(),
            sequence,
            0));
  }

  core::KvResult<records::KvRecord> make_delete(
      const keys::KeyPath &key,
      std::uint64_t sequence)
  {
    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          encoded_key.error());
    }

    return core::KvResult<records::KvRecord>::ok(
        records::KvRecord::remove(
            encoded_key.move_value(),
            sequence,
            0));
  }

  core::KvResult<storage::Segment> write_segment(
      const core::KvConfig &config,
      std::uint64_t segment_id,
      const std::vector<records::KvRecord> &records_to_write)
  {
    storage::SegmentWriter writer{config, segment_id};

    auto opened = writer.open(true);

    if (opened.is_err())
    {
      return core::KvResult<storage::Segment>::err(
          opened.error());
    }

    for (const auto &record : records_to_write)
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
}

int main()
{
  const auto root = std::filesystem::path{"data/examples/compact.kv"};

  std::filesystem::remove_all(root);

  auto config = core::KvConfig::durable(root);

  auto user_v1 = make_put(
      key_path(std::vector<std::string>{"users", "1", "name"}),
      "Ada",
      1);

  if (user_v1.is_err())
  {
    return fail("failed to build record: " + user_v1.error().message());
  }

  auto user_v2 = make_put(
      key_path(std::vector<std::string>{"users", "1", "name"}),
      "Ada Lovelace",
      2);

  if (user_v2.is_err())
  {
    return fail("failed to build record: " + user_v2.error().message());
  }

  auto temp_value = make_put(
      key_path(std::vector<std::string>{"cache", "temp"}),
      "old",
      3);

  if (temp_value.is_err())
  {
    return fail("failed to build record: " + temp_value.error().message());
  }

  auto delete_temp = make_delete(
      key_path(std::vector<std::string>{"cache", "temp"}),
      4);

  if (delete_temp.is_err())
  {
    return fail(
        "failed to build delete record: " +
        delete_temp.error().message());
  }

  std::vector<records::KvRecord> segment_1_records;
  segment_1_records.push_back(user_v1.move_value());
  segment_1_records.push_back(temp_value.move_value());

  auto segment_1 = write_segment(
      config,
      1,
      segment_1_records);

  if (segment_1.is_err())
  {
    return fail(
        "failed to write input segment 1: " +
        segment_1.error().message());
  }

  std::vector<records::KvRecord> segment_2_records;
  segment_2_records.push_back(user_v2.move_value());
  segment_2_records.push_back(delete_temp.move_value());

  auto segment_2 = write_segment(
      config,
      2,
      segment_2_records);

  if (segment_2.is_err())
  {
    return fail(
        "failed to write input segment 2: " +
        segment_2.error().message());
  }

  const auto output_path =
      storage::FileLayout::segment_path(config, 3);

  std::vector<storage::Segment> input_segments;
  input_segments.push_back(segment_1.move_value());
  input_segments.push_back(segment_2.move_value());

  auto plan = compaction::Compactor::make_manual_plan(
      std::move(input_segments),
      3,
      output_path);

  compaction::Compactor compactor;

  auto result = compactor.compact(plan);

  if (result.is_err())
  {
    return fail(
        "compaction failed: " +
        result.error().message());
  }

  const auto &stats = result.value();

  std::cout << "compaction completed\n";
  std::cout << "input segments : "
            << stats.input_segment_count
            << '\n';

  std::cout << "input records  : "
            << stats.input_record_count
            << '\n';

  std::cout << "output records : "
            << stats.output_record_count
            << '\n';

  std::cout << "obsolete       : "
            << stats.skipped_obsolete_records
            << '\n';

  std::cout << "input bytes    : "
            << stats.input_bytes
            << '\n';

  std::cout << "output bytes   : "
            << stats.output_bytes
            << '\n';

  std::cout << "reclaimed      : "
            << stats.bytes_reclaimed()
            << '\n';

  std::cout << "output segment : "
            << stats.output_segment.path.string()
            << '\n';

  return 0;
}
