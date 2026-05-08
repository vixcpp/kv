/**
 *
 *  @file KvWalWriter.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL writer implementation
 *
 */

#include <vix/kv/wal/KvWalWriter.hpp>

#include <ios>
#include <system_error>

namespace vix::kv::wal
{
  KvWalWriter::KvWalWriter(core::KvConfig config)
      : config_(std::move(config)),
        path_(config_.wal_path)
  {
  }

  KvWalWriter::KvWalWriter(std::filesystem::path path)
      : path_(std::move(path))
  {
    config_ = core::KvConfig::durable(
        path_.has_parent_path()
            ? path_.parent_path().parent_path()
            : std::filesystem::path{"data/kv"});

    config_.wal_path = path_;
  }

  KvWalWriter::KvWalWriter(KvWalWriter &&other) noexcept
  {
    move_from(std::move(other));
  }

  KvWalWriter &KvWalWriter::operator=(KvWalWriter &&other) noexcept
  {
    if (this != &other)
    {
      if (stream_.is_open())
      {
        stream_.flush();
        stream_.close();
      }

      move_from(std::move(other));
    }

    return *this;
  }

  KvWalWriter::~KvWalWriter()
  {
    if (stream_.is_open())
    {
      stream_.flush();
      stream_.close();
    }

    open_ = false;
  }

  core::KvResult<void> KvWalWriter::open()
  {
    if (open_)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::AlreadyOpen,
              "WAL writer is already open",
              path_));
    }

    if (path_.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::wal(
              "WAL path must not be empty"));
    }

    if (config_.read_only)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::Unsupported,
              "cannot open WAL writer in read-only mode",
              path_));
    }

    auto directory = ensure_parent_directory();

    if (directory.is_err())
    {
      return directory;
    }

    stream_.open(
        path_,
        std::ios::binary |
            std::ios::out |
            std::ios::app);

    if (!stream_.is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to open WAL file for append",
              path_));
    }

    open_ = true;

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvWalWriter::append(
      const records::KvRecord &record)
  {
    auto encoded = records::KvRecordEncoder::encode(record);

    if (encoded.is_err())
    {
      return core::KvResult<void>::err(encoded.error());
    }

    return append_bytes(encoded.value());
  }

  core::KvResult<void> KvWalWriter::append_bytes(
      const Bytes &bytes)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return open_result;
    }

    if (bytes.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "cannot append empty WAL record"));
    }

    if (!core::KvLimits::valid_record_size(bytes.size()))
    {
      return core::KvResult<void>::err(
          core::KvError::wal(
              "encoded WAL record is too large"));
    }

    stream_.write(
        reinterpret_cast<const char *>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));

    if (!stream_)
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to write WAL record",
              path_));
    }

    ++records_written_;
    bytes_written_ += static_cast<std::uint64_t>(bytes.size());

    if (config_.auto_flush)
    {
      auto flushed = flush();

      if (flushed.is_err())
      {
        return flushed;
      }
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvWalWriter::flush()
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return open_result;
    }

    stream_.flush();

    if (!stream_)
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to flush WAL file",
              path_));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvWalWriter::close()
  {
    if (!open_)
    {
      return core::KvResult<void>::ok();
    }

    auto flushed = flush();

    if (flushed.is_err())
    {
      return flushed;
    }

    stream_.close();

    if (stream_.fail())
    {
      open_ = false;

      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to close WAL file",
              path_));
    }

    open_ = false;

    return core::KvResult<void>::ok();
  }

  bool KvWalWriter::is_open() const noexcept
  {
    return open_ && stream_.is_open();
  }

  const std::filesystem::path &KvWalWriter::path() const noexcept
  {
    return path_;
  }

  std::uint64_t KvWalWriter::records_written() const noexcept
  {
    return records_written_;
  }

  std::uint64_t KvWalWriter::bytes_written() const noexcept
  {
    return bytes_written_;
  }

  core::KvResult<void> KvWalWriter::ensure_parent_directory()
  {
    const auto parent = path_.parent_path();

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
                "WAL parent path exists but is not a directory",
                parent));
      }

      return core::KvResult<void>::ok();
    }

    if (!config_.create_directories)
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "WAL parent directory does not exist",
              parent));
    }

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);

    if (ec)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to create WAL parent directory",
              parent,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> KvWalWriter::require_open() const
  {
    if (!is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "WAL writer is not open",
              path_));
    }

    return core::KvResult<void>::ok();
  }

  void KvWalWriter::move_from(KvWalWriter &&other) noexcept
  {
    config_ = std::move(other.config_);
    path_ = std::move(other.path_);
    stream_ = std::move(other.stream_);
    open_ = other.open_;
    records_written_ = other.records_written_;
    bytes_written_ = other.bytes_written_;

    other.open_ = false;
    other.records_written_ = 0;
    other.bytes_written_ = 0;
  }

} // namespace vix::kv::wal
