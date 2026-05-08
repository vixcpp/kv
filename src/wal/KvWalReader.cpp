/**
 *
 *  @file KvWalReader.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL reader implementation
 *
 */

#include <vix/kv/wal/KvWalReader.hpp>

#include <ios>
#include <system_error>
#include <span>

#include <vix/kv/records/KvRecordDecoder.hpp>
#include <vix/kv/records/KvRecordEncoder.hpp>

namespace vix::kv::wal
{
  KvWalReader::KvWalReader(core::KvConfig config)
      : config_(std::move(config)),
        path_(config_.wal_path)
  {
  }

  KvWalReader::KvWalReader(std::filesystem::path path)
      : path_(std::move(path))
  {
    config_ = core::KvConfig::durable(
        path_.has_parent_path()
            ? path_.parent_path().parent_path()
            : std::filesystem::path{"data/kv"});

    config_.wal_path = path_;
  }

  KvWalReader::KvWalReader(KvWalReader &&other) noexcept
  {
    move_from(std::move(other));
  }

  KvWalReader &KvWalReader::operator=(KvWalReader &&other) noexcept
  {
    if (this != &other)
    {
      if (stream_.is_open())
      {
        stream_.close();
      }

      move_from(std::move(other));
    }

    return *this;
  }

  KvWalReader::~KvWalReader()
  {
    if (stream_.is_open())
    {
      stream_.close();
    }

    open_ = false;
  }

  core::KvResult<void> KvWalReader::open()
  {
    if (open_)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::AlreadyOpen,
              "WAL reader is already open",
              path_));
    }

    if (path_.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::wal(
              "WAL path must not be empty"));
    }

    if (!std::filesystem::exists(path_))
    {
      open_ = true;
      eof_ = true;
      return core::KvResult<void>::ok();
    }

    if (!std::filesystem::is_regular_file(path_))
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "WAL path exists but is not a regular file",
              path_));
    }

    stream_.open(path_, std::ios::binary | std::ios::in);

    if (!stream_.is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to open WAL file for reading",
              path_));
    }

    open_ = true;
    eof_ = false;
    records_read_ = 0;
    bytes_read_ = 0;

    return core::KvResult<void>::ok();
  }

  core::KvResult<records::KvRecord> KvWalReader::read_next()
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          open_result.error());
    }

    if (eof_)
    {
      return core::KvResult<records::KvRecord>::err(
          core::KvError::make(
              core::KvErrorCode::NotFound,
              "end of WAL reached",
              path_));
    }

    auto header = read_header();

    if (header.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          header.error());
    }

    auto payload = read_payload(header.value());

    if (payload.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          payload.error());
    }

    Bytes encoded;
    encoded.reserve(
        static_cast<std::size_t>(header.value().total_size()));

    auto encoded_header = vix::kv::records::KvRecordEncoder::encode_header(header.value());

    if (encoded_header.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          encoded_header.error());
    }

    encoded.insert(
        encoded.end(),
        encoded_header.value().begin(),
        encoded_header.value().end());

    encoded.insert(
        encoded.end(),
        payload.value().begin(),
        payload.value().end());

    auto decoded = vix::kv::records::KvRecordDecoder::decode(encoded);

    if (decoded.is_err())
    {
      return core::KvResult<records::KvRecord>::err(
          decoded.error());
    }

    ++records_read_;

    return core::KvResult<records::KvRecord>::ok(
        decoded.move_value());
  }

  core::KvResult<void> KvWalReader::for_each(
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
  KvWalReader::read_all()
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

  core::KvResult<void> KvWalReader::close()
  {
    if (!open_)
    {
      return core::KvResult<void>::ok();
    }

    if (stream_.is_open())
    {
      stream_.close();

      if (stream_.fail())
      {
        open_ = false;

        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to close WAL reader",
                path_));
      }
    }

    open_ = false;

    return core::KvResult<void>::ok();
  }

  bool KvWalReader::is_open() const noexcept
  {
    if (eof_ && open_)
    {
      return true;
    }

    return open_ && stream_.is_open();
  }

  bool KvWalReader::eof() const noexcept
  {
    return eof_;
  }

  const std::filesystem::path &KvWalReader::path() const noexcept
  {
    return path_;
  }

  std::uint64_t KvWalReader::records_read() const noexcept
  {
    return records_read_;
  }

  std::uint64_t KvWalReader::bytes_read() const noexcept
  {
    return bytes_read_;
  }

  core::KvResult<KvWalReader::Bytes>
  KvWalReader::read_exact(std::size_t count)
  {
    auto open_result = require_open();

    if (open_result.is_err())
    {
      return core::KvResult<Bytes>::err(open_result.error());
    }

    if (count == 0)
    {
      return core::KvResult<Bytes>::ok(Bytes{});
    }

    Bytes buffer(count);

    stream_.read(
        reinterpret_cast<char *>(buffer.data()),
        static_cast<std::streamsize>(count));

    const auto read_count = stream_.gcount();

    if (read_count == 0 && stream_.eof())
    {
      eof_ = true;

      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::NotFound,
              "end of WAL reached",
              path_));
    }

    if (read_count != static_cast<std::streamsize>(count))
    {
      eof_ = true;

      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::Corruption,
              "partial WAL record detected",
              path_));
    }

    bytes_read_ += static_cast<std::uint64_t>(count);

    return core::KvResult<Bytes>::ok(std::move(buffer));
  }

  core::KvResult<records::KvRecordHeader>
  KvWalReader::read_header()
  {
    auto header_bytes =
        read_exact(records::KvRecordHeader::encoded_size);

    if (header_bytes.is_err())
    {
      return core::KvResult<records::KvRecordHeader>::err(
          header_bytes.error());
    }

    std::size_t offset = 0;

    auto header = records::KvRecordDecoder::decode_header(
        std::span<const std::uint8_t>(
            header_bytes.value().data(),
            header_bytes.value().size()),
        offset);

    if (header.is_err())
    {
      return header;
    }

    auto validation =
        records::KvRecordDecoder::validate_header(header.value());

    if (validation.is_err())
    {
      return core::KvResult<records::KvRecordHeader>::err(
          validation.error());
    }

    auto checksum =
        records::KvRecordDecoder::verify_header_checksum(header.value());

    if (checksum.is_err())
    {
      return core::KvResult<records::KvRecordHeader>::err(
          checksum.error());
    }

    return header;
  }

  core::KvResult<KvWalReader::Bytes>
  KvWalReader::read_payload(
      const records::KvRecordHeader &header)
  {
    const auto payload_size =
        static_cast<std::size_t>(header.payload_size());

    if (payload_size == 0)
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "WAL record payload must not be empty"));
    }

    if (!core::KvLimits::valid_record_size(payload_size))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::corruption(
              "WAL record payload is too large"));
    }

    return read_exact(payload_size);
  }

  core::KvResult<void> KvWalReader::require_open() const
  {
    if (!is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "WAL reader is not open",
              path_));
    }

    return core::KvResult<void>::ok();
  }

  void KvWalReader::move_from(KvWalReader &&other) noexcept
  {
    config_ = std::move(other.config_);
    path_ = std::move(other.path_);
    stream_ = std::move(other.stream_);
    open_ = other.open_;
    eof_ = other.eof_;
    records_read_ = other.records_read_;
    bytes_read_ = other.bytes_read_;

    other.open_ = false;
    other.eof_ = false;
    other.records_read_ = 0;
    other.bytes_read_ = 0;
  }

} // namespace vix::kv::wal
