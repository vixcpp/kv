/**
 *
 *  @file KvIndex.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  In-memory index implementation
 *
 */

#include <vix/kv/index/KvIndex.hpp>

#include <algorithm>

namespace vix::kv::index
{
  KvIndex::KvIndex(std::size_t initial_capacity)
  {
    reserve(initial_capacity);
  }

  void KvIndex::reserve(std::size_t capacity)
  {
    if (capacity > 0)
    {
      entries_.reserve(capacity);
    }
  }

  core::KvResult<void> KvIndex::put(
      std::string key,
      std::uint64_t segment_id,
      std::uint64_t offset,
      std::uint64_t size,
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

    auto size_validation = validate_record_size(size);

    if (size_validation.is_err())
    {
      return size_validation;
    }

    return apply(
        KvIndexEntry::live(
            std::move(key),
            segment_id,
            offset,
            size,
            sequence,
            timestamp_ms));
  }

  core::KvResult<void> KvIndex::put(
      std::string_view key,
      std::uint64_t segment_id,
      std::uint64_t offset,
      std::uint64_t size,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return put(
        std::string(key),
        segment_id,
        offset,
        size,
        sequence,
        timestamp_ms);
  }

  core::KvResult<void> KvIndex::erase(
      std::string key,
      std::uint64_t segment_id,
      std::uint64_t offset,
      std::uint64_t size,
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

    auto size_validation = validate_record_size(size);

    if (size_validation.is_err())
    {
      return size_validation;
    }

    return apply(
        KvIndexEntry::tombstone(
            std::move(key),
            segment_id,
            offset,
            size,
            sequence,
            timestamp_ms));
  }

  core::KvResult<void> KvIndex::erase(
      std::string_view key,
      std::uint64_t segment_id,
      std::uint64_t offset,
      std::uint64_t size,
      std::uint64_t sequence,
      std::uint64_t timestamp_ms)
  {
    return erase(
        std::string(key),
        segment_id,
        offset,
        size,
        sequence,
        timestamp_ms);
  }

  core::KvResult<void> KvIndex::apply(KvIndexEntry entry)
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

    auto size_validation = validate_record_size(entry.size);

    if (size_validation.is_err())
    {
      return size_validation;
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

  std::optional<KvIndexEntry>
  KvIndex::get(std::string_view key) const
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

  std::optional<KvIndexEntry>
  KvIndex::get_raw(std::string_view key) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return std::nullopt;
    }

    return found->second;
  }

  bool KvIndex::contains(std::string_view key) const
  {
    const auto found = entries_.find(std::string(key));

    if (found == entries_.end())
    {
      return false;
    }

    return !found->second.deleted;
  }

  bool KvIndex::contains_raw(std::string_view key) const
  {
    return entries_.find(std::string(key)) != entries_.end();
  }

  KvIndex::List KvIndex::list(std::string_view prefix) const
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
        [](const KvIndexEntry &a, const KvIndexEntry &b)
        {
          return a.key < b.key;
        });

    return out;
  }

  KvIndex::List KvIndex::list_raw(std::string_view prefix) const
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
        [](const KvIndexEntry &a, const KvIndexEntry &b)
        {
          return a.key < b.key;
        });

    return out;
  }

  std::size_t KvIndex::prune_tombstones()
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

  bool KvIndex::remove_raw(std::string_view key)
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

  const KvIndex::Map &KvIndex::entries() const noexcept
  {
    return entries_;
  }

  std::size_t KvIndex::size() const noexcept
  {
    return live_count_;
  }

  std::size_t KvIndex::raw_size() const noexcept
  {
    return entries_.size();
  }

  std::size_t KvIndex::tombstone_count() const noexcept
  {
    return tombstone_count_;
  }

  bool KvIndex::empty() const noexcept
  {
    return live_count_ == 0;
  }

  bool KvIndex::raw_empty() const noexcept
  {
    return entries_.empty();
  }

  std::uint64_t KvIndex::last_sequence() const noexcept
  {
    return last_sequence_;
  }

  void KvIndex::clear() noexcept
  {
    entries_.clear();
    live_count_ = 0;
    tombstone_count_ = 0;
    last_sequence_ = 0;
  }

  core::KvResult<void> KvIndex::validate_key(std::string_view key)
  {
    if (key.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_key(
              "index key must not be empty"));
    }

    if (key.size() > core::KvLimits::max_key_size)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_key(
              "index key is too large"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvIndex::validate_sequence(
      std::uint64_t sequence)
  {
    if (sequence == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "index sequence must be greater than zero"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvIndex::validate_record_size(
      std::uint64_t size)
  {
    if (size == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "index record size must be greater than zero"));
    }

    if (size > core::KvLimits::max_record_size)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "index record size is too large"));
    }

    return core::KvResult<void>::ok();
  }

  bool KvIndex::has_newer_entry(
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

  bool KvIndex::matches_prefix(
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

  void KvIndex::update_counts_after_replace(
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

  void KvIndex::update_last_sequence(
      std::uint64_t sequence) noexcept
  {
    if (sequence > last_sequence_)
    {
      last_sequence_ = sequence;
    }
  }

} // namespace vix::kv::index
