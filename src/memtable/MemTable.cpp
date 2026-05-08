/**
 *
 *  @file MemTable.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  In-memory materialized table implementation
 *
 */

#include <vix/kv/memtable/MemTable.hpp>

#include <algorithm>

namespace vix::kv::memtable
{
  MemTable::MemTable(std::size_t initial_capacity)
  {
    reserve(initial_capacity);
  }

  void MemTable::reserve(std::size_t capacity)
  {
    if (capacity > 0)
    {
      entries_.reserve(capacity);
    }
  }

  core::KvResult<void> MemTable::put(
      std::string key,
      std::vector<std::uint8_t> value,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    auto key_validation = validate_key(key);

    if (key_validation.is_err())
    {
      return key_validation;
    }

    auto sequence_validation = validate_sequence(sequence);

    if (sequence_validation.is_err())
    {
      return sequence_validation;
    }

    if (!core::KvLimits::valid_value_size(value.size()))
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "value is too large"));
    }

    return apply(
        MemTableEntry::live(
            std::move(key),
            std::move(value),
            sequence,
            timestamp_ms));
  }

  core::KvResult<void> MemTable::put(
      std::string_view key,
      const std::vector<std::uint8_t> &value,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return put(
        std::string(key),
        value,
        sequence,
        timestamp_ms);
  }

  core::KvResult<void> MemTable::erase(
      std::string key,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    auto key_validation = validate_key(key);

    if (key_validation.is_err())
    {
      return key_validation;
    }

    auto sequence_validation = validate_sequence(sequence);

    if (sequence_validation.is_err())
    {
      return sequence_validation;
    }

    return apply(
        MemTableEntry::tombstone(
            std::move(key),
            sequence,
            timestamp_ms));
  }

  core::KvResult<void> MemTable::erase(
      std::string_view key,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return erase(
        std::string(key),
        sequence,
        timestamp_ms);
  }

  core::KvResult<void> MemTable::apply(MemTableEntry entry)
  {
    auto key_validation = validate_key(entry.key);

    if (key_validation.is_err())
    {
      return key_validation;
    }

    auto sequence_validation = validate_sequence(entry.sequence);

    if (sequence_validation.is_err())
    {
      return sequence_validation;
    }

    if (!core::KvLimits::valid_value_size(entry.value.size()))
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "entry value is too large"));
    }

    if (has_newer_entry(entry.key, entry.sequence))
    {
      return core::KvResult<void>::ok();
    }

    const auto found = entries_.find(entry.key);
    const bool had_existing = found != entries_.end();
    const bool old_deleted =
        had_existing ? found->second.deleted : false;
    const bool new_deleted = entry.deleted;

    const std::string map_key = entry.key;

    entries_[map_key] = std::move(entry);

    update_counts_after_replace(
        had_existing,
        old_deleted,
        new_deleted);

    update_last_sequence(entries_[map_key].sequence);

    return core::KvResult<void>::ok();
  }

  std::optional<MemTableEntry>
  MemTable::get(std::string_view key) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return std::nullopt;
    }

    if (found->second.deleted)
    {
      return std::nullopt;
    }

    return found->second;
  }

  std::optional<MemTableEntry>
  MemTable::get_raw(std::string_view key) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return std::nullopt;
    }

    return found->second;
  }

  bool MemTable::contains(std::string_view key) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return false;
    }

    return !found->second.deleted;
  }

  bool MemTable::contains_raw(std::string_view key) const
  {
    return entries_.find(std::string(key)) != entries_.end();
  }

  MemTable::List MemTable::list(std::string_view prefix) const
  {
    List out;

    for (const auto &[key, entry] : entries_)
    {
      if (entry.deleted)
      {
        continue;
      }

      if (!matches_prefix(key, prefix))
      {
        continue;
      }

      out.push_back(entry);
    }

    std::sort(
        out.begin(),
        out.end(),
        [](const MemTableEntry &a, const MemTableEntry &b)
        {
          return a.key < b.key;
        });

    return out;
  }

  MemTable::List MemTable::list_raw(std::string_view prefix) const
  {
    List out;

    for (const auto &[key, entry] : entries_)
    {
      if (!matches_prefix(key, prefix))
      {
        continue;
      }

      out.push_back(entry);
    }

    std::sort(
        out.begin(),
        out.end(),
        [](const MemTableEntry &a, const MemTableEntry &b)
        {
          return a.key < b.key;
        });

    return out;
  }

  std::size_t MemTable::prune_tombstones()
  {
    std::size_t removed = 0;

    for (auto it = entries_.begin(); it != entries_.end();)
    {
      if (it->second.deleted)
      {
        it = entries_.erase(it);
        ++removed;
        continue;
      }

      ++it;
    }

    if (removed > tombstone_count_)
    {
      tombstone_count_ = 0;
    }
    else
    {
      tombstone_count_ -= removed;
    }

    return removed;
  }

  bool MemTable::remove_raw(std::string_view key)
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return false;
    }

    if (found->second.deleted)
    {
      if (tombstone_count_ > 0)
      {
        --tombstone_count_;
      }
    }
    else
    {
      if (live_count_ > 0)
      {
        --live_count_;
      }
    }

    entries_.erase(found);
    return true;
  }

  const MemTable::Map &MemTable::entries() const noexcept
  {
    return entries_;
  }

  std::size_t MemTable::size() const noexcept
  {
    return live_count_;
  }

  std::size_t MemTable::raw_size() const noexcept
  {
    return entries_.size();
  }

  std::size_t MemTable::tombstone_count() const noexcept
  {
    return tombstone_count_;
  }

  bool MemTable::empty() const noexcept
  {
    return live_count_ == 0;
  }

  bool MemTable::raw_empty() const noexcept
  {
    return entries_.empty();
  }

  std::uint64_t MemTable::byte_size() const noexcept
  {
    std::uint64_t total = 0;

    for (const auto &[_, entry] : entries_)
    {
      total += entry.byte_size();
    }

    return total;
  }

  std::uint64_t MemTable::last_sequence() const noexcept
  {
    return last_sequence_;
  }

  void MemTable::clear() noexcept
  {
    entries_.clear();
    live_count_ = 0;
    tombstone_count_ = 0;
    last_sequence_ = 0;
  }

  core::KvResult<void> MemTable::validate_key(
      std::string_view key)
  {
    if (key.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_key(
              "memtable key must not be empty"));
    }

    if (key.size() > core::KvLimits::max_key_size)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_key(
              "memtable key is too large"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> MemTable::validate_sequence(
      std::uint64_t sequence)
  {
    if (sequence == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "memtable sequence must be greater than zero"));
    }

    return core::KvResult<void>::ok();
  }

  bool MemTable::has_newer_entry(
      std::string_view key,
      std::uint64_t incoming_sequence) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return false;
    }

    return found->second.sequence > incoming_sequence;
  }

  bool MemTable::matches_prefix(
      std::string_view key,
      std::string_view prefix) noexcept
  {
    if (prefix.empty())
    {
      return true;
    }

    if (key.size() < prefix.size())
    {
      return false;
    }

    return key.substr(0, prefix.size()) == prefix;
  }

  void MemTable::update_counts_after_replace(
      bool had_existing,
      bool old_deleted,
      bool new_deleted) noexcept
  {
    if (!had_existing)
    {
      if (new_deleted)
      {
        ++tombstone_count_;
      }
      else
      {
        ++live_count_;
      }

      return;
    }

    if (old_deleted == new_deleted)
    {
      return;
    }

    if (old_deleted && !new_deleted)
    {
      if (tombstone_count_ > 0)
      {
        --tombstone_count_;
      }

      ++live_count_;
      return;
    }

    if (!old_deleted && new_deleted)
    {
      if (live_count_ > 0)
      {
        --live_count_;
      }

      ++tombstone_count_;
    }
  }

  void MemTable::update_last_sequence(
      std::uint64_t sequence) noexcept
  {
    if (sequence > last_sequence_)
    {
      last_sequence_ = sequence;
    }
  }

} // namespace vix::kv::memtable
