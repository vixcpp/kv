/**
 *
 *  @file KvWalRecovery.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL recovery implementation
 *
 */

#include <vix/kv/wal/KvWalRecovery.hpp>

namespace vix::kv::wal
{
  KvWalRecovery::KvWalRecovery(core::KvConfig config)
      : config_(std::move(config)),
        path_(config_.wal_path)
  {
  }

  KvWalRecovery::KvWalRecovery(std::filesystem::path wal_path)
      : path_(std::move(wal_path))
  {
    config_ = core::KvConfig::durable(
        path_.has_parent_path()
            ? path_.parent_path().parent_path()
            : std::filesystem::path{"data/kv"});

    config_.wal_path = path_;
  }

  core::KvResult<RecoveryResult>
  KvWalRecovery::recover(memtable::MemTable &table)
  {
    RecoveryResult result;

    if (path_.empty())
    {
      return core::KvResult<RecoveryResult>::err(
          core::KvError::wal(
              "WAL recovery path must not be empty"));
    }

    KvWalReader reader{config_};

    auto opened = reader.open();

    if (opened.is_err())
    {
      return core::KvResult<RecoveryResult>::err(opened.error());
    }

    auto replayed = reader.for_each(
        [&](const records::KvRecord &record)
        {
          return apply_record(record, table, result);
        });

    result.bytes_read = reader.bytes_read();
    result.live_keys = static_cast<std::uint64_t>(table.size());
    result.tombstones =
        static_cast<std::uint64_t>(table.tombstone_count());

    auto closed = reader.close();

    if (closed.is_err())
    {
      return core::KvResult<RecoveryResult>::err(closed.error());
    }

    if (replayed.is_err())
    {
      ++result.records_corrupted;

      return core::KvResult<RecoveryResult>::err(replayed.error());
    }

    return core::KvResult<RecoveryResult>::ok(result);
  }

  const std::filesystem::path &KvWalRecovery::path() const noexcept
  {
    return path_;
  }

  core::KvResult<void>
  KvWalRecovery::apply_record(
      const records::KvRecord &record,
      memtable::MemTable &table,
      RecoveryResult &result)
  {
    if (!record.is_valid())
    {
      ++result.records_corrupted;

      return core::KvResult<void>::err(
          core::KvError::corruption(
              "cannot recover invalid WAL record"));
    }

    if (record.is_put())
    {
      auto applied = apply_put(record, table);

      if (applied.is_err())
      {
        return applied;
      }

      update_result(record, result);
      return core::KvResult<void>::ok();
    }

    if (record.is_delete())
    {
      auto applied = apply_delete(record, table);

      if (applied.is_err())
      {
        return applied;
      }

      update_result(record, result);
      return core::KvResult<void>::ok();
    }

    if (record.is_snapshot() || record.is_compaction())
    {
      ++result.records_skipped;
      update_result(record, result);
      return core::KvResult<void>::ok();
    }

    ++result.records_corrupted;

    return core::KvResult<void>::err(
        core::KvError::corruption(
            "unsupported WAL record type during recovery"));
  }

  core::KvResult<void>
  KvWalRecovery::apply_put(
      const records::KvRecord &record,
      memtable::MemTable &table)
  {
    return table.put(
        record.key,
        record.value,
        record.header.sequence,
        record.header.timestamp_ms);
  }

  core::KvResult<void>
  KvWalRecovery::apply_delete(
      const records::KvRecord &record,
      memtable::MemTable &table)
  {
    return table.erase(
        record.key,
        record.header.sequence,
        record.header.timestamp_ms);
  }

  void KvWalRecovery::update_result(
      const records::KvRecord &record,
      RecoveryResult &result) noexcept
  {
    ++result.records_recovered;

    if (record.header.sequence > result.last_sequence)
    {
      result.last_sequence = record.header.sequence;
    }
  }

} // namespace vix::kv::wal
