/**
 *
 *  @file SnapshotWriter.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot writer implementation
 *
 */

#include <vix/kv/snapshot/SnapshotWriter.hpp>

#include <system_error>

#include <vix/kv/checksum/Crc32.hpp>
#include <vix/kv/utils/Bytes.hpp>
#include <vix/kv/utils/Endian.hpp>

namespace vix::kv::snapshot
{
  namespace checksum = vix::kv::checksum;
  namespace utils = vix::kv::utils;

  SnapshotWriter::SnapshotWriter(core::KvConfig config)
      : config_(std::move(config))
  {
  }

  SnapshotWriter::SnapshotWriter(std::filesystem::path path)
      : path_(std::move(path))
  {
    config_ = core::KvConfig::durable(
        path_.has_parent_path()
            ? path_.parent_path().parent_path()
            : std::filesystem::path{"data/kv"});
  }

  core::KvResult<SnapshotWriteResult>
  SnapshotWriter::write(const Snapshot &snapshot)
  {
    std::filesystem::path final_path = path_;

    if (final_path.empty())
    {
      final_path = storage::FileLayout::snapshot_path(
          config_,
          snapshot.id);
    }

    return write_to(snapshot, final_path);
  }

  core::KvResult<SnapshotWriteResult>
  SnapshotWriter::write_to(
      const Snapshot &snapshot,
      const std::filesystem::path &final_path)
  {
    auto validation = validate(snapshot);

    if (validation.is_err())
    {
      return core::KvResult<SnapshotWriteResult>::err(
          validation.error());
    }

    if (final_path.empty())
    {
      return core::KvResult<SnapshotWriteResult>::err(
          core::KvError::storage(
              "snapshot path must not be empty"));
    }

    auto encoded = encode(snapshot);

    if (encoded.is_err())
    {
      return core::KvResult<SnapshotWriteResult>::err(
          encoded.error());
    }

    auto written = write_atomic(encoded.value(), final_path);

    if (written.is_err())
    {
      return core::KvResult<SnapshotWriteResult>::err(
          written.error());
    }

    SnapshotWriteResult result;
    result.snapshot_id = snapshot.id;
    result.path = final_path;
    result.size_bytes =
        static_cast<std::uint64_t>(encoded.value().size());
    result.entry_count =
        static_cast<std::uint64_t>(snapshot.entries.size());
    result.last_sequence = snapshot.last_sequence;

    return core::KvResult<SnapshotWriteResult>::ok(std::move(result));
  }

  core::KvResult<SnapshotWriter::Bytes>
  SnapshotWriter::encode(const Snapshot &snapshot)
  {
    auto validation = validate(snapshot);

    if (validation.is_err())
    {
      return core::KvResult<Bytes>::err(validation.error());
    }

    Bytes out;
    out.reserve(
        static_cast<std::size_t>(64) +
        static_cast<std::size_t>(snapshot.byte_size()));

    append_header(out, snapshot);
    append_entries(out, snapshot);
    append_checksum(out);

    return core::KvResult<Bytes>::ok(std::move(out));
  }

  core::KvResult<void>
  SnapshotWriter::validate(const Snapshot &snapshot)
  {
    if (!snapshot.has_id())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "snapshot id must be greater than zero"));
    }

    if (!snapshot.entries_are_valid())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "snapshot contains invalid entries"));
    }

    if (!snapshot.entries.empty() && snapshot.last_sequence == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "snapshot last sequence must be greater than zero when entries exist"));
    }

    if (snapshot.entries.size() >
        static_cast<std::size_t>(
            static_cast<std::uint64_t>(-1)))
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "snapshot entry count is too large"));
    }

    for (const auto &entry : snapshot.entries)
    {
      if (entry.key.size() > core::KvLimits::max_key_size)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_key(
                "snapshot entry key is too large"));
      }

      if (entry.value.size() > core::KvLimits::max_value_size)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_argument(
                "snapshot entry value is too large"));
      }
    }

    return core::KvResult<void>::ok();
  }

  const core::KvConfig &SnapshotWriter::config() const noexcept
  {
    return config_;
  }

  const std::filesystem::path &SnapshotWriter::path() const noexcept
  {
    return path_;
  }

  void SnapshotWriter::append_header(
      Bytes &out,
      const Snapshot &snapshot)
  {
    utils::Endian::append_u32(out, core::KvLimits::snapshot_magic);
    utils::Endian::append_u8(out, core::KvLimits::snapshot_format_version);
    utils::Endian::append_u64(out, snapshot.id);
    utils::Endian::append_u64(out, snapshot.last_sequence);
    utils::Endian::append_u64(out, snapshot.created_at_ms);
    utils::Endian::append_u64(
        out,
        static_cast<std::uint64_t>(snapshot.entries.size()));
  }

  void SnapshotWriter::append_entries(
      Bytes &out,
      const Snapshot &snapshot)
  {
    for (const auto &entry : snapshot.entries)
    {
      utils::Endian::append_u8(
          out,
          entry.deleted ? static_cast<std::uint8_t>(1)
                        : static_cast<std::uint8_t>(0));

      utils::Endian::append_u64(out, entry.sequence);
      utils::Endian::append_u64(out, entry.timestamp_ms);

      utils::Endian::append_u32(
          out,
          static_cast<std::uint32_t>(entry.key.size()));

      utils::Endian::append_u64(
          out,
          static_cast<std::uint64_t>(entry.value.size()));

      utils::Bytes::append_string_bytes(out, entry.key);
      utils::Bytes::append(out, entry.value);
    }
  }

  void SnapshotWriter::append_checksum(Bytes &out)
  {
    const std::uint32_t crc = checksum::Crc32::compute(out);
    utils::Endian::append_u32(out, crc);
  }

  core::KvResult<void>
  SnapshotWriter::ensure_parent_directory(
      const std::filesystem::path &final_path)
  {
    const auto parent = final_path.parent_path();

    if (parent.empty())
    {
      return core::KvResult<void>::ok();
    }

    if (std::filesystem::exists(parent))
    {
      if (!std::filesystem::is_directory(parent))
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "snapshot parent path exists but is not a directory",
                parent));
      }

      return core::KvResult<void>::ok();
    }

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);

    if (ec)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to create snapshot parent directory",
              parent,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void>
  SnapshotWriter::write_atomic(
      const Bytes &bytes,
      const std::filesystem::path &final_path)
  {
    auto directory = ensure_parent_directory(final_path);

    if (directory.is_err())
    {
      return directory;
    }

    const auto temp_path = storage::FileLayout::temporary_path(final_path);

    {
      std::ofstream stream(
          temp_path,
          std::ios::binary |
              std::ios::out |
              std::ios::trunc);

      if (!stream.is_open())
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to open temporary snapshot file",
                temp_path));
      }

      stream.write(
          reinterpret_cast<const char *>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));

      if (!stream)
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to write temporary snapshot file",
                temp_path));
      }

      stream.flush();

      if (!stream)
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to flush temporary snapshot file",
                temp_path));
      }

      stream.close();

      if (stream.fail())
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to close temporary snapshot file",
                temp_path));
      }
    }

    std::error_code ec;

    if (std::filesystem::exists(final_path, ec))
    {
      std::filesystem::remove(final_path, ec);

      if (ec)
      {
        return core::KvResult<void>::err(
            core::KvError::make(
                core::KvErrorCode::IoError,
                "failed to remove existing snapshot file",
                final_path,
                ec.message()));
      }
    }

    std::filesystem::rename(temp_path, final_path, ec);

    if (ec)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to publish snapshot file",
              final_path,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

} // namespace vix::kv::snapshot
