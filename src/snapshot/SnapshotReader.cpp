/**
 *
 *  @file SnapshotReader.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot reader implementation
 *
 */

#include <vix/kv/snapshot/SnapshotReader.hpp>

#include <span>
#include <system_error>

#include <vix/kv/checksum/Crc32.hpp>
#include <vix/kv/utils/Endian.hpp>

namespace vix::kv::snapshot
{
  namespace checksum = vix::kv::checksum;
  namespace utils = vix::kv::utils;

  SnapshotReader::SnapshotReader(core::KvConfig config)
      : config_(std::move(config))
  {
  }

  SnapshotReader::SnapshotReader(std::filesystem::path path)
      : path_(std::move(path))
  {
    config_ = core::KvConfig::durable(
        path_.has_parent_path()
            ? path_.parent_path().parent_path()
            : std::filesystem::path{"data/kv"});
  }

  core::KvResult<Snapshot> SnapshotReader::read() const
  {
    if (path_.empty())
    {
      return core::KvResult<Snapshot>::err(
          core::KvError::storage(
              "snapshot path must not be empty"));
    }

    return read_from(path_);
  }

  core::KvResult<Snapshot>
  SnapshotReader::read_snapshot(std::uint64_t snapshot_id) const
  {
    if (snapshot_id == 0)
    {
      return core::KvResult<Snapshot>::err(
          core::KvError::invalid_argument(
              "snapshot id must be greater than zero"));
    }

    return read_from(
        storage::FileLayout::snapshot_path(config_, snapshot_id));
  }

  core::KvResult<Snapshot>
  SnapshotReader::read_from(const std::filesystem::path &path) const
  {
    auto bytes = read_file(path);

    if (bytes.is_err())
    {
      return core::KvResult<Snapshot>::err(bytes.error());
    }

    return decode(bytes.value());
  }

  core::KvResult<Snapshot>
  SnapshotReader::decode(const Bytes &bytes)
  {
    return decode(
        std::span<const std::uint8_t>(bytes.data(), bytes.size()));
  }

  core::KvResult<Snapshot>
  SnapshotReader::decode(std::span<const std::uint8_t> bytes)
  {
    if (bytes.size() < 4)
    {
      return core::KvResult<Snapshot>::err(
          core::KvError::corruption(
              "snapshot file is too small"));
    }

    auto checksum_result = verify_checksum(bytes);

    if (checksum_result.is_err())
    {
      return core::KvResult<Snapshot>::err(
          checksum_result.error());
    }

    std::size_t offset = 0;
    std::uint64_t entry_count = 0;
    Snapshot snapshot;

    auto header = decode_header(
        bytes,
        offset,
        snapshot,
        entry_count);

    if (header.is_err())
    {
      return core::KvResult<Snapshot>::err(header.error());
    }

    auto entries = decode_entries(
        bytes,
        offset,
        entry_count,
        snapshot);

    if (entries.is_err())
    {
      return core::KvResult<Snapshot>::err(entries.error());
    }

    const std::size_t checksum_size = sizeof(std::uint32_t);

    if (offset + checksum_size != bytes.size())
    {
      return core::KvResult<Snapshot>::err(
          core::KvError::corruption(
              "snapshot contains trailing bytes"));
    }

    if (!snapshot.is_valid())
    {
      return core::KvResult<Snapshot>::err(
          core::KvError::corruption(
              "decoded snapshot is invalid"));
    }

    return core::KvResult<Snapshot>::ok(std::move(snapshot));
  }

  const core::KvConfig &SnapshotReader::config() const noexcept
  {
    return config_;
  }

  const std::filesystem::path &SnapshotReader::path() const noexcept
  {
    return path_;
  }

  core::KvResult<SnapshotReader::Bytes>
  SnapshotReader::read_file(const std::filesystem::path &path)
  {
    if (path.empty())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::storage(
              "snapshot path must not be empty"));
    }

    if (!std::filesystem::exists(path))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::NotFound,
              "snapshot file does not exist",
              path));
    }

    if (!std::filesystem::is_regular_file(path))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::io(
              "snapshot path exists but is not a regular file",
              path));
    }

    std::error_code ec;
    const auto file_size = std::filesystem::file_size(path, ec);

    if (ec)
    {
      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to read snapshot file size",
              path,
              ec.message()));
    }

    Bytes bytes(static_cast<std::size_t>(file_size));

    std::ifstream stream(path, std::ios::binary | std::ios::in);

    if (!stream.is_open())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::io(
              "failed to open snapshot file",
              path));
    }

    if (!bytes.empty())
    {
      stream.read(
          reinterpret_cast<char *>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));

      if (!stream)
      {
        return core::KvResult<Bytes>::err(
            core::KvError::io(
                "failed to read snapshot file",
                path));
      }
    }

    return core::KvResult<Bytes>::ok(std::move(bytes));
  }

  core::KvResult<void>
  SnapshotReader::verify_checksum(std::span<const std::uint8_t> bytes)
  {
    const std::size_t checksum_size = sizeof(std::uint32_t);

    if (bytes.size() < checksum_size)
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "snapshot checksum is missing"));
    }

    const std::size_t payload_size = bytes.size() - checksum_size;

    std::size_t checksum_offset = payload_size;
    std::uint32_t expected = 0;

    if (!utils::Endian::read_u32(bytes, checksum_offset, expected))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot checksum"));
    }

    const auto payload = bytes.subspan(0, payload_size);
    const std::uint32_t actual = checksum::Crc32::compute(payload);

    if (actual != expected)
    {
      return core::KvResult<void>::err(
          core::KvError::checksum_mismatch(
              "snapshot checksum mismatch"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  SnapshotReader::decode_header(
      std::span<const std::uint8_t> bytes,
      std::size_t &offset,
      Snapshot &snapshot,
      std::uint64_t &entry_count)
  {
    std::uint32_t magic = 0;
    std::uint8_t version = 0;

    if (!utils::Endian::read_u32(bytes, offset, magic))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot magic"));
    }

    if (magic != core::KvLimits::snapshot_magic)
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "snapshot magic is invalid"));
    }

    if (!utils::Endian::read_u8(bytes, offset, version))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot format version"));
    }

    if (version != core::KvLimits::snapshot_format_version)
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "snapshot format version is not supported"));
    }

    if (!utils::Endian::read_u64(bytes, offset, snapshot.id))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot id"));
    }

    if (!utils::Endian::read_u64(bytes, offset, snapshot.last_sequence))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot last sequence"));
    }

    if (!utils::Endian::read_u64(bytes, offset, snapshot.created_at_ms))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot timestamp"));
    }

    if (!utils::Endian::read_u64(bytes, offset, entry_count))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "failed to read snapshot entry count"));
    }

    if (snapshot.id == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "snapshot id must be greater than zero"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  SnapshotReader::decode_entries(
      std::span<const std::uint8_t> bytes,
      std::size_t &offset,
      std::uint64_t entry_count,
      Snapshot &snapshot)
  {
    snapshot.entries.clear();

    if (entry_count >
        static_cast<std::uint64_t>(
            static_cast<std::size_t>(-1)))
    {
      return core::KvResult<void>::err(
          core::KvError::corruption(
              "snapshot entry count cannot fit in memory size"));
    }

    snapshot.entries.reserve(static_cast<std::size_t>(entry_count));

    for (std::uint64_t index = 0; index < entry_count; ++index)
    {
      std::uint8_t deleted = 0;
      std::uint64_t sequence = 0;
      std::uint64_t timestamp_ms = 0;
      std::uint32_t key_size = 0;
      std::uint64_t value_size = 0;

      if (!utils::Endian::read_u8(bytes, offset, deleted))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "failed to read snapshot entry deleted flag"));
      }

      if (deleted > 1)
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry deleted flag is invalid"));
      }

      if (!utils::Endian::read_u64(bytes, offset, sequence))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "failed to read snapshot entry sequence"));
      }

      if (!utils::Endian::read_u64(bytes, offset, timestamp_ms))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "failed to read snapshot entry timestamp"));
      }

      if (!utils::Endian::read_u32(bytes, offset, key_size))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "failed to read snapshot entry key size"));
      }

      if (!utils::Endian::read_u64(bytes, offset, value_size))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "failed to read snapshot entry value size"));
      }

      if (key_size == 0 || key_size > core::KvLimits::max_key_size)
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry key size is invalid"));
      }

      if (value_size > core::KvLimits::max_value_size)
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry value size is too large"));
      }

      if (value_size >
          static_cast<std::uint64_t>(
              static_cast<std::size_t>(-1)))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry value size cannot fit in memory size"));
      }

      const std::size_t key_count = static_cast<std::size_t>(key_size);
      const std::size_t value_count = static_cast<std::size_t>(value_size);

      if (!utils::Endian::can_read(bytes, offset, key_count))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry key is truncated"));
      }

      std::string key(
          reinterpret_cast<const char *>(bytes.data() + offset),
          key_count);

      offset += key_count;

      if (!utils::Endian::can_read(bytes, offset, value_count))
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry value is truncated"));
      }

      std::vector<std::uint8_t> value(
          bytes.begin() + static_cast<std::ptrdiff_t>(offset),
          bytes.begin() + static_cast<std::ptrdiff_t>(offset + value_count));

      offset += value_count;

      memtable::MemTableEntry entry{
          std::move(key),
          std::move(value),
          sequence,
          timestamp_ms,
          deleted == 1};

      if (!entry.is_valid())
      {
        return core::KvResult<void>::err(
            core::KvError::corruption(
                "snapshot entry is invalid"));
      }

      snapshot.entries.push_back(std::move(entry));
    }

    return core::KvResult<void>::ok();
  }

} // namespace vix::kv::snapshot
