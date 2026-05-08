/**
 *
 *  @file KeyEncoder.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Stable KeyPath encoder implementation
 *
 */

#include <vix/kv/keys/KeyEncoder.hpp>

#include <cctype>
#include <limits>

#include <vix/kv/keys/KeyValidator.hpp>

namespace vix::kv::keys
{
  core::KvResult<std::string>
  KeyEncoder::encode(const KeyPath &path)
  {
    return encode_to_string(path);
  }

  core::KvResult<std::string>
  KeyEncoder::encode_to_string(const KeyPath &path)
  {
    auto validation = KeyValidator::validate(path);

    if (validation.is_err())
    {
      return core::KvResult<std::string>::err(validation.error());
    }

    const std::size_t size = encoded_size(path);

    if (size == 0)
    {
      return core::KvResult<std::string>::err(
          core::KvError::invalid_key(
              "encoded key size is invalid"));
    }

    if (size > core::KvLimits::max_key_size)
    {
      return core::KvResult<std::string>::err(
          core::KvError::invalid_key(
              "encoded key is too large"));
    }

    std::string encoded;
    encoded.reserve(size);
    encoded += version_prefix;

    for (const auto &segment : path.segments())
    {
      append_segment(encoded, segment);
    }

    return core::KvResult<std::string>::ok(std::move(encoded));
  }

  std::string KeyEncoder::encode_or_empty(const KeyPath &path)
  {
    auto encoded = encode(path);

    if (encoded.is_err())
    {
      return {};
    }

    return encoded.move_value();
  }

  core::KvResult<KeyPath>
  KeyEncoder::decode(std::string_view encoded)
  {
    if (encoded.empty())
    {
      return core::KvResult<KeyPath>::err(
          core::KvError::invalid_key(
              "encoded key must not be empty"));
    }

    if (!has_supported_prefix(encoded))
    {
      return core::KvResult<KeyPath>::err(
          core::KvError::invalid_key(
              "unsupported encoded key format"));
    }

    std::size_t offset = version_prefix.size();
    KeyPath path;

    while (offset < encoded.size())
    {
      std::size_t segment_size = 0;

      if (!read_size(encoded, offset, segment_size))
      {
        return core::KvResult<KeyPath>::err(
            core::KvError::invalid_key(
                "failed to read encoded key segment size"));
      }

      if (segment_size == 0)
      {
        return core::KvResult<KeyPath>::err(
            core::KvError::invalid_key(
                "encoded key segment must not be empty"));
      }

      if (segment_size > core::KvLimits::max_key_segment_size)
      {
        return core::KvResult<KeyPath>::err(
            core::KvError::invalid_key(
                "encoded key segment is too large"));
      }

      if (!can_read(encoded, offset, segment_size))
      {
        return core::KvResult<KeyPath>::err(
            core::KvError::invalid_key(
                "encoded key segment is truncated"));
      }

      path.push_back(
          std::string{
              encoded.substr(offset, segment_size)});

      offset += segment_size;

      if (path.size() > core::KvLimits::max_key_segments)
      {
        return core::KvResult<KeyPath>::err(
            core::KvError::invalid_key(
                "encoded key has too many segments"));
      }
    }

    auto validation = KeyValidator::validate(path);

    if (validation.is_err())
    {
      return core::KvResult<KeyPath>::err(validation.error());
    }

    return core::KvResult<KeyPath>::ok(std::move(path));
  }

  bool KeyEncoder::has_supported_prefix(
      std::string_view encoded) noexcept
  {
    if (encoded.size() < version_prefix.size())
    {
      return false;
    }

    return encoded.substr(0, version_prefix.size()) == version_prefix;
  }

  bool KeyEncoder::matches_prefix(
      std::string_view encoded,
      std::string_view encoded_prefix) noexcept
  {
    if (encoded_prefix.empty())
    {
      return true;
    }

    if (encoded.size() < encoded_prefix.size())
    {
      return false;
    }

    return encoded.substr(0, encoded_prefix.size()) == encoded_prefix;
  }

  std::size_t KeyEncoder::encoded_size(const KeyPath &path)
  {
    if (!KeyValidator::is_valid(path))
    {
      return 0;
    }

    std::size_t size = version_prefix.size();

    for (const auto &segment : path.segments())
    {
      size += decimal_digits(segment.size());
      size += 1;
      size += segment.size();

      if (size > core::KvLimits::max_key_size)
      {
        return 0;
      }
    }

    return size;
  }

  void KeyEncoder::append_segment(
      std::string &out,
      std::string_view segment)
  {
    out += std::to_string(segment.size());
    out.push_back(size_separator);
    out.append(segment.data(), segment.size());
  }

  bool KeyEncoder::read_size(
      std::string_view encoded,
      std::size_t &offset,
      std::size_t &size) noexcept
  {
    if (offset >= encoded.size())
    {
      return false;
    }

    size = 0;
    bool has_digit = false;

    while (offset < encoded.size() &&
           encoded[offset] != size_separator)
    {
      const unsigned char ch =
          static_cast<unsigned char>(encoded[offset]);

      if (!std::isdigit(ch))
      {
        return false;
      }

      has_digit = true;

      const std::size_t digit =
          static_cast<std::size_t>(encoded[offset] - '0');

      if (size >
          (std::numeric_limits<std::size_t>::max() - digit) / 10)
      {
        return false;
      }

      size = (size * 10) + digit;
      ++offset;
    }

    if (!has_digit)
    {
      return false;
    }

    if (offset >= encoded.size())
    {
      return false;
    }

    if (encoded[offset] != size_separator)
    {
      return false;
    }

    ++offset;
    return true;
  }

  bool KeyEncoder::can_read(
      std::string_view data,
      std::size_t offset,
      std::size_t count) noexcept
  {
    return offset <= data.size() && count <= data.size() - offset;
  }

  std::size_t KeyEncoder::decimal_digits(
      std::size_t value) noexcept
  {
    std::size_t digits = 1;

    while (value >= 10)
    {
      value /= 10;
      ++digits;
    }

    return digits;
  }

} // namespace vix::kv::keys
