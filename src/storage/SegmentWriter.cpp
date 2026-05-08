/**
 *
 *  @file SegmentWriter.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Segment writer implementation
 *
 */

#include <vix/kv/storage/SegmentWriter.hpp>

#include <utility>

namespace vix::kv::storage
{
  SegmentWriter::SegmentWriter(
      core::KvConfig config,
      std::uint64_t segment_id)
      : config_(std::move(config)),
        segment_(
            segment_id,
            FileLayout::segment_path(config_, segment_id)),
        writer_(segment_.path)
  {
  }

  SegmentWriter::SegmentWriter(Segment segment)
      : segment_(std::move(segment)),
        writer_(segment_.path)
  {
  }

  SegmentWriter::SegmentWriter(SegmentWriter &&other) noexcept
  {
    move_from(std::move(other));
  }

  SegmentWriter &SegmentWriter::operator=(
      SegmentWriter &&other) noexcept
  {
    if (this != &other)
    {
      if (writer_.is_open())
      {
        (void)writer_.close();
      }

      move_from(std::move(other));
    }

    return *this;
  }

  SegmentWriter::~SegmentWriter()
  {
    if (writer_.is_open())
    {
      (void)writer_.close();
    }
  }

  core::KvResult<void> SegmentWriter::open(
      bool truncate_existing)
  {
    auto validation = validate_segment();

    if (validation.is_err())
    {
      return validation;
    }

    if (segment_.sealed)
    {
      return core::KvResult<void>::err(
          core::KvError::storage(
              "cannot open a sealed segment for writing"));
    }

    return writer_.open(truncate_existing);
  }

  core::KvResult<SegmentWriteResult>
  SegmentWriter::append(const records::KvRecord &record)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<SegmentWriteResult>::err(
          open_result.error());
    }

    auto written = writer_.append(record);

    if (written.is_err())
    {
      return core::KvResult<SegmentWriteResult>::err(
          written.error());
    }

    segment_.observe_record(
        written.value().size,
        written.value().sequence);

    SegmentWriteResult result;
    result.segment_id = segment_.id;
    result.offset = written.value().offset;
    result.size = written.value().size;
    result.sequence = written.value().sequence;

    return core::KvResult<SegmentWriteResult>::ok(result);
  }

  core::KvResult<SegmentWriteResult>
  SegmentWriter::append_bytes(
      const Bytes &bytes,
      std::uint64_t sequence)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<SegmentWriteResult>::err(
          open_result.error());
    }

    auto written = writer_.append_bytes(bytes, sequence);

    if (written.is_err())
    {
      return core::KvResult<SegmentWriteResult>::err(
          written.error());
    }

    segment_.observe_record(
        written.value().size,
        written.value().sequence);

    SegmentWriteResult result;
    result.segment_id = segment_.id;
    result.offset = written.value().offset;
    result.size = written.value().size;
    result.sequence = written.value().sequence;

    return core::KvResult<SegmentWriteResult>::ok(result);
  }

  core::KvResult<void> SegmentWriter::flush()
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return open_result;
    }

    return writer_.flush();
  }

  core::KvResult<void> SegmentWriter::close()
  {
    if (!writer_.is_open())
    {
      segment_.seal();
      return core::KvResult<void>::ok();
    }

    auto closed = writer_.close();

    if (closed.is_err())
    {
      return closed;
    }

    segment_.size_bytes = writer_.bytes_written();
    segment_.record_count = writer_.records_written();
    segment_.seal();

    return core::KvResult<void>::ok();
  }

  bool SegmentWriter::is_open() const noexcept
  {
    return writer_.is_open();
  }

  const Segment &SegmentWriter::segment() const noexcept
  {
    return segment_;
  }

  Segment &SegmentWriter::segment() noexcept
  {
    return segment_;
  }

  std::uint64_t SegmentWriter::segment_id() const noexcept
  {
    return segment_.id;
  }

  const std::filesystem::path &SegmentWriter::path() const noexcept
  {
    return segment_.path;
  }

  std::uint64_t SegmentWriter::offset() const noexcept
  {
    return writer_.offset();
  }

  std::uint64_t SegmentWriter::records_written() const noexcept
  {
    return writer_.records_written();
  }

  std::uint64_t SegmentWriter::bytes_written() const noexcept
  {
    return writer_.bytes_written();
  }

  core::KvResult<void> SegmentWriter::validate_segment() const
  {
    if (segment_.id == 0)
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "segment id must be greater than zero"));
    }

    if (segment_.path.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::storage(
              "segment path must not be empty"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> SegmentWriter::require_open() const
  {
    if (!is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "segment writer is not open",
              segment_.path));
    }

    return core::KvResult<void>::ok();
  }

  void SegmentWriter::move_from(SegmentWriter &&other) noexcept
  {
    config_ = std::move(other.config_);
    segment_ = std::move(other.segment_);
    writer_ = std::move(other.writer_);
  }

} // namespace vix::kv::storage
