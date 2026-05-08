/**
 *
 *  @file KvEngine.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Internal KV engine implementation
 *
 */

#include <vix/kv/internal/KvEngine.hpp>
#include <utility>
#include <vix/kv/values/ValueCodec.hpp>

namespace vix::kv::internal
{
  KvEngine::KvEngine()
      : KvEngine(core::KvConfig::durable("data/kv"))
  {
  }

  KvEngine::KvEngine(core::KvConfig config)
      : state_(std::move(config)),
        wal_(state_.config)
  {
  }

  KvEngine::~KvEngine()
  {
    if (state_.open)
    {
      (void)close();
    }
  }

  core::KvResult<void> KvEngine::open()
  {
    if (state_.open)
    {
      observe_error();

      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::AlreadyOpen,
              "KV engine is already open"));
    }

    auto config_validation = state_.config.validate();

    if (config_validation.is_err())
    {
      observe_error();
      return config_validation;
    }

    state_.refresh_static_stats();

    if (state_.config.recover_on_open)
    {
      auto recovered = recover();

      if (recovered.is_err())
      {
        observe_error();
        return recovered;
      }
    }

    auto opened_wal = open_wal();

    if (opened_wal.is_err())
    {
      observe_error();
      return opened_wal;
    }

    state_.open = true;
    state_.refresh_stats();

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::close()
  {
    if (!state_.open)
    {
      return core::KvResult<void>::ok();
    }

    auto closed = wal_.close();

    if (closed.is_err())
    {
      observe_error();
      return closed;
    }

    state_.open = false;
    ++state_.stats.close_count;
    state_.refresh_stats();

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::flush()
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      observe_error();
      return open_result;
    }

    auto flushed = wal_.flush();

    if (flushed.is_err())
    {
      observe_error();
      return flushed;
    }

    ++state_.stats.flush_count;

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::set(
      const keys::KeyPath &key,
      const values::KvValue &value)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      observe_error();
      return open_result;
    }

    auto writable = require_writable();

    if (writable.is_err())
    {
      observe_error();
      return writable;
    }

    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      observe_error();
      return core::KvResult<void>::err(encoded_key.error());
    }

    auto encoded_value = values::ValueCodec::encode(value);

    if (encoded_value.is_err())
    {
      observe_error();
      return core::KvResult<void>::err(encoded_value.error());
    }

    auto record = make_put_record(
        encoded_key.move_value(),
        encoded_value.move_value());

    auto written = append_to_wal(record);

    if (written.is_err())
    {
      observe_error();
      return written;
    }

    auto applied = apply_to_memtable(record);

    if (applied.is_err())
    {
      observe_error();
      return applied;
    }

    ++state_.stats.set_count;
    state_.refresh_memtable_stats();

    return core::KvResult<void>::ok();
  }

  core::KvResult<values::KvValue> KvEngine::get(
      const keys::KeyPath &key) const
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<values::KvValue>::err(open_result.error());
    }

    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      return core::KvResult<values::KvValue>::err(encoded_key.error());
    }

    auto entry = state_.memtable.get(encoded_key.value());

    if (!entry.has_value())
    {
      const_cast<KvEngine *>(this)->state_.stats.get_miss_count++;

      return core::KvResult<values::KvValue>::err(
          core::KvError::not_found(
              "key was not found"));
    }

    auto value = decode_value(entry.value());

    if (value.is_err())
    {
      const_cast<KvEngine *>(this)->observe_error();
      return value;
    }

    const_cast<KvEngine *>(this)->state_.stats.get_count++;

    return value;
  }

  core::KvResult<void> KvEngine::erase(const keys::KeyPath &key)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      observe_error();
      return open_result;
    }

    auto writable = require_writable();

    if (writable.is_err())
    {
      observe_error();
      return writable;
    }

    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      observe_error();
      return core::KvResult<void>::err(encoded_key.error());
    }

    if (!state_.memtable.contains(encoded_key.value()))
    {
      ++state_.stats.erase_miss_count;

      return core::KvResult<void>::err(
          core::KvError::not_found(
              "key was not found"));
    }

    auto record = make_delete_record(encoded_key.move_value());

    auto written = append_to_wal(record);

    if (written.is_err())
    {
      observe_error();
      return written;
    }

    auto applied = apply_to_memtable(record);

    if (applied.is_err())
    {
      observe_error();
      return applied;
    }

    ++state_.stats.erase_count;
    state_.refresh_memtable_stats();

    return core::KvResult<void>::ok();
  }

  bool KvEngine::contains(const keys::KeyPath &key) const
  {
    if (!state_.open)
    {
      return false;
    }

    auto encoded_key = encode_key(key);

    if (encoded_key.is_err())
    {
      return false;
    }

    return state_.memtable.contains(encoded_key.value());
  }

  core::KvResult<KvEngine::ListResult>
  KvEngine::list(const keys::KeyPath &prefix) const
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<ListResult>::err(open_result.error());
    }

    std::string encoded_prefix;

    if (!prefix.empty())
    {
      auto encoded = encode_key(prefix);

      if (encoded.is_err())
      {
        return core::KvResult<ListResult>::err(encoded.error());
      }

      encoded_prefix = encoded.move_value();
    }

    const auto entries = state_.memtable.list(encoded_prefix);

    ListResult out;
    out.reserve(entries.size());

    for (const auto &entry : entries)
    {
      auto decoded_key = decode_key(entry.key);

      if (decoded_key.is_err())
      {
        const_cast<KvEngine *>(this)->observe_error();

        return core::KvResult<ListResult>::err(
            decoded_key.error());
      }

      auto decoded_value = decode_value(entry);

      if (decoded_value.is_err())
      {
        const_cast<KvEngine *>(this)->observe_error();

        return core::KvResult<ListResult>::err(
            decoded_value.error());
      }

      out.emplace_back(
          decoded_key.move_value(),
          decoded_value.move_value());
    }

    const_cast<KvEngine *>(this)->state_.stats.list_count++;

    return core::KvResult<ListResult>::ok(std::move(out));
  }

  std::size_t KvEngine::size() const noexcept
  {
    return state_.memtable.size();
  }

  bool KvEngine::empty() const noexcept
  {
    return state_.memtable.empty();
  }

  bool KvEngine::is_open() const noexcept
  {
    return state_.open;
  }

  const core::KvConfig &KvEngine::config() const noexcept
  {
    return state_.config;
  }

  core::KvStats KvEngine::stats() const
  {
    auto copy = state_;
    copy.refresh_stats();
    return copy.stats;
  }

  const memtable::MemTable::Map &KvEngine::entries() const noexcept
  {
    return state_.memtable.entries();
  }

  core::KvResult<void> KvEngine::require_open() const
  {
    if (!state_.open)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "KV engine is not open"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::require_writable() const
  {
    if (state_.config.read_only)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::Unsupported,
              "KV engine is opened in read-only mode"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<std::string>
  KvEngine::encode_key(const keys::KeyPath &key)
  {
    return keys::KeyEncoder::encode(key);
  }

  core::KvResult<keys::KeyPath>
  KvEngine::decode_key(const std::string &encoded)
  {
    return keys::KeyEncoder::decode(encoded);
  }

  core::KvResult<values::KvValue>
  KvEngine::decode_value(const memtable::MemTableEntry &entry)
  {
    return values::ValueCodec::decode(entry.value);
  }

  records::KvRecord KvEngine::make_put_record(
      std::string encoded_key,
      std::vector<std::uint8_t> value)
  {
    return records::KvRecord::put(
        std::move(encoded_key),
        std::move(value),
        state_.next_sequence(),
        0);
  }

  records::KvRecord KvEngine::make_delete_record(
      std::string encoded_key)
  {
    return records::KvRecord::remove(
        std::move(encoded_key),
        state_.next_sequence(),
        0);
  }

  core::KvResult<void> KvEngine::append_to_wal(
      const records::KvRecord &record)
  {
    if (!state_.config.enable_wal)
    {
      return core::KvResult<void>::ok();
    }

    auto written = wal_.append(record);

    if (written.is_err())
    {
      return written;
    }

    ++state_.stats.wal_records_written;
    state_.stats.wal_bytes_written = wal_.bytes_written();

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::apply_to_memtable(
      const records::KvRecord &record)
  {
    state_.observe_sequence(record.header.sequence);

    if (record.is_put())
    {
      return state_.memtable.put(
          record.key,
          record.value,
          record.header.sequence,
          record.header.timestamp_ms);
    }

    if (record.is_delete())
    {
      return state_.memtable.erase(
          record.key,
          record.header.sequence,
          record.header.timestamp_ms);
    }

    return core::KvResult<void>::err(
        core::KvError::invalid_argument(
            "unsupported record type for memtable apply"));
  }

  core::KvResult<void> KvEngine::recover()
  {
    if (!state_.config.enable_wal ||
        !state_.config.recover_on_open)
    {
      return core::KvResult<void>::ok();
    }

    auto recovered = wal_.recover(state_.memtable);

    if (recovered.is_err())
    {
      return core::KvResult<void>::err(recovered.error());
    }

    observe_recovery(recovered.value());

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvEngine::open_wal()
  {
    if (!state_.config.enable_wal)
    {
      return core::KvResult<void>::ok();
    }

    return wal_.open();
  }

  void KvEngine::observe_recovery(
      const wal::RecoveryResult &result) noexcept
  {
    state_.observe_sequence(result.last_sequence);

    state_.stats.wal_records_recovered = result.records_recovered;
    state_.stats.wal_records_skipped = result.records_skipped;
    state_.stats.wal_records_corrupted = result.records_corrupted;
    state_.stats.wal_bytes_recovered = result.bytes_read;
    state_.stats.last_recovered_sequence = result.last_sequence;

    state_.refresh_memtable_stats();
  }

  void KvEngine::observe_error() noexcept
  {
    ++state_.stats.error_count;
  }

} // namespace vix::kv::internal
