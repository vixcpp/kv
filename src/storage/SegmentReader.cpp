/**
 *
 *  @file SegmentReader.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment reader implementation
 *
 */

#include <vix/kv/storage/SegmentReader.hpp>

#include <utility>

namespace vix::kv::storage
{
  SegmentReader::SegmentReader(Segment segment)
      : segment_(std::move(segment)),
        reader_(segment_.path)
  {
  }

  SegmentReader::SegmentReader(std::filesystem::path path)
      : segment_(0, std::move(path)),
        reader_(segment_.path)
  {
  }

  SegmentReader::SegmentReader(SegmentReader &&other) noexcept
  {
    move_from(std::move(other));
  }

  SegmentReader &SegmentReader::operator=(
      SegmentReader &&other) noexcept
  {
    if (this != &other)
    {
      if (reader_.is_open())
      {
        (void)reader_.close();
      }

      move_from(std::move(other));
    }

    return *this;
  }

  SegmentReader::~SegmentReader()
  {
    if (reader_.is_open())
    {
      (void)reader_.close();
    }
  }

  core::KvResult<void> SegmentReader::open()
  {
    auto validation = validate_segment();

    if (validation.is_err())
    {
      return validation;
    }

    return reader_.open();
  }

  core::KvResult<records::KvRecord> SegmentReader::read_next()
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          open_result.error());
    }

    auto record = reader_.read_next();

    if (record.is_err())
    {
      return record;
    }

    observe_record(record.value());

    return record;
  }

  core::KvResult<records::KvRecord>
  SegmentReader::read_at(std::uint64_t offset)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          open_result.error());
    }

    auto record = reader_.read_at(offset);

    if (record.is_err())
    {
      return record;
    }

    observe_record(record.value());

    return record;
  }

  core::KvResult<void> SegmentReader::for_each(
      const RecordCallback &callback)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return open_result;
    }

    while (true)
    {
      auto record = read_next();

      if (record.is_err())
      {
        if (record.error().code() == core::KvErrorCode::NotFound)
        {
          return core::KvResult<void>::ok();
        }

        return core::KvResult<void>::err(record.error());
      }

      auto callback_result = callback(record.value());

      if (callback_result.is_err())
      {
        return callback_result;
      }
    }
  }

  core::KvResult<std::vector<records::KvRecord>>
  SegmentReader::read_all()
  {
    std::vector<records::KvRecord> records;

    auto result = for_each(
        [&](const records::KvRecord &record)
        {
          records.push_back(record);
          return core::KvResult<void>::ok();
        });

    if (result.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          result.error());
    }

    return core::KvResult<std::vector<records::KvRecord>>::ok(
        std::move(records));
  }

  core::KvResult<void> SegmentReader::seek(std::uint64_t offset)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return open_result;
    }

    return reader_.seek(offset);
  }

  core::KvResult<void> SegmentReader::close()
  {
    return reader_.close();
  }

  bool SegmentReader::is_open() const noexcept
  {
    return reader_.is_open();
  }

  bool SegmentReader::eof() const noexcept
  {
    return reader_.eof();
  }

  const Segment &SegmentReader::segment() const noexcept
  {
    return segment_;
  }

  Segment &SegmentReader::segment() noexcept
  {
    return segment_;
  }

  const std::filesystem::path &SegmentReader::path() const noexcept
  {
    return segment_.path;
  }

  std::uint64_t SegmentReader::offset() const noexcept
  {
    return reader_.offset();
  }

  std::uint64_t SegmentReader::records_read() const noexcept
  {
    return reader_.records_read();
  }

  std::uint64_t SegmentReader::bytes_read() const noexcept
  {
    return reader_.bytes_read();
  }

  core::KvResult<void> SegmentReader::validate_segment() const
  {
    if (segment_.path.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::storage(
              "segment path must not be empty"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> SegmentReader::require_open() const
  {
    if (!is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "segment reader is not open",
              segment_.path));
    }

    return core::KvResult<void>::ok();
  }

  void SegmentReader::observe_record(
      const records::KvRecord &record) noexcept
  {
    if (segment_.id == 0)
    {
      return;
    }

    segment_.observe_sequence(record.header.sequence);
  }

  void SegmentReader::move_from(SegmentReader &&other) noexcept
  {
    segment_ = std::move(other.segment_);
    reader_ = std::move(other.reader_);
  }

} // namespace vix::kv::storage
