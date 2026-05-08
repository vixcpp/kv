/**
 *
 *  @file DataFileWriter.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Data file writer implementation
 *
 */

#include <vix/kv/storage/DataFileWriter.hpp>

#include <ios>
#include <system_error>

namespace vix::kv::storage
{
  DataFileWriter::DataFileWriter(std::filesystem::path path)
      : path_(std::move(path))
  {
  }

  DataFileWriter::DataFileWriter(DataFileWriter &&other) noexcept
  {
    move_from(std::move(other));
  }

  DataFileWriter &DataFileWriter::operator=(
      DataFileWriter &&other) noexcept
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

  DataFileWriter::~DataFileWriter()
  {
    if (stream_.is_open())
    {
      stream_.flush();
      stream_.close();
    }

    open_ = false;
  }

  core::KvResult<void> DataFileWriter::open(
      bool truncate_existing)
  {
    if (open_)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::AlreadyOpen,
              "data file writer is already open",
              path_));
    }

    if (path_.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::storage(
              "data file path must not be empty"));
    }

    auto directory = ensure_parent_directory();

    if (directory.is_err())
    {
      return directory;
    }

    std::ios::openmode mode =
        std::ios::binary |
        std::ios::out;

    if (truncate_existing)
    {
      mode |= std::ios::trunc;
      offset_ = 0;
    }
    else
    {
      mode |= std::ios::app;

      if (std::filesystem::exists(path_))
      {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path_, ec);

        if (ec)
        {
          return core::KvResult<void>::err(
              core::KvError::make(
                  core::KvErrorCode::IoError,
                  "failed to read data file size",
                  path_,
                  ec.message()));
        }

        offset_ = static_cast<std::uint64_t>(size);
      }
      else
      {
        offset_ = 0;
      }
    }

    stream_.open(path_, mode);

    if (!stream_.is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to open data file for writing",
              path_));
    }

    open_ = true;
    records_written_ = 0;
    bytes_written_ = 0;

    return core::KvResult<void>::ok();
  }

  core::KvResult<DataFileWriteResult>
  DataFileWriter::append(const records::KvRecord &record)
  {
    auto encoded = records::KvRecordEncoder::encode(record);

    if (encoded.is_err())
    {
      return core::KvResult<DataFileWriteResult>::err(
          encoded.error());
    }

    return append_bytes(
        encoded.value(),
        record.header.sequence);
  }

  core::KvResult<DataFileWriteResult>
  DataFileWriter::append_bytes(
      const Bytes &bytes,
      std::uint64_t sequence)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<DataFileWriteResult>::err(
          open_result.error());
    }

    if (bytes.empty())
    {
      return core::KvResult<DataFileWriteResult>::err(
          core::KvError::invalid_argument(
              "cannot append empty data file record"));
    }

    if (!vix::kv::core::KvLimits::valid_record_size(bytes.size()))
    {
      return core::KvResult<DataFileWriteResult>::err(
          core::KvError::storage(
              "encoded data file record is too large"));
    }

    if (sequence == 0)
    {
      return core::KvResult<DataFileWriteResult>::err(
          core::KvError::invalid_argument(
              "data file record sequence must be greater than zero"));
    }

    const std::uint64_t start_offset = offset_;

    stream_.write(
        reinterpret_cast<const char *>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));

    if (!stream_)
    {
      return core::KvResult<DataFileWriteResult>::err(
          core::KvError::io(
              "failed to write data file record",
              path_));
    }

    const std::uint64_t written =
        static_cast<std::uint64_t>(bytes.size());

    offset_ += written;
    ++records_written_;
    bytes_written_ += written;

    DataFileWriteResult result;
    result.offset = start_offset;
    result.size = written;
    result.sequence = sequence;

    return core::KvResult<DataFileWriteResult>::ok(result);
  }

  core::KvResult<void> DataFileWriter::flush()
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
              "failed to flush data file",
              path_));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> DataFileWriter::close()
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
              "failed to close data file",
              path_));
    }

    open_ = false;

    return core::KvResult<void>::ok();
  }

  bool DataFileWriter::is_open() const noexcept
  {
    return open_ && stream_.is_open();
  }

  const std::filesystem::path &DataFileWriter::path() const noexcept
  {
    return path_;
  }

  std::uint64_t DataFileWriter::offset() const noexcept
  {
    return offset_;
  }

  std::uint64_t DataFileWriter::records_written() const noexcept
  {
    return records_written_;
  }

  std::uint64_t DataFileWriter::bytes_written() const noexcept
  {
    return bytes_written_;
  }

  core::KvResult<void> DataFileWriter::ensure_parent_directory()
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
                "data file parent path exists but is not a directory",
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
              "failed to create data file parent directory",
              parent,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> DataFileWriter::require_open() const
  {
    if (!is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "data file writer is not open",
              path_));
    }

    return core::KvResult<void>::ok();
  }

  void DataFileWriter::move_from(DataFileWriter &&other) noexcept
  {
    path_ = std::move(other.path_);
    stream_ = std::move(other.stream_);
    open_ = other.open_;
    offset_ = other.offset_;
    records_written_ = other.records_written_;
    bytes_written_ = other.bytes_written_;

    other.open_ = false;
    other.offset_ = 0;
    other.records_written_ = 0;
    other.bytes_written_ = 0;
  }

} // namespace vix::kv::storage
