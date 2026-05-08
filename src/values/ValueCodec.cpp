/**
 *
 *  @file ValueCodec.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Value encoding helpers implementation
 *
 */

#include <vix/kv/values/ValueCodec.hpp>

namespace vix::kv::values
{
  core::KvResult<void>
  ValueCodec::validate(const KvValue &value)
  {
    return validate_size(value.size());
  }

  core::KvResult<void>
  ValueCodec::validate_size(std::size_t size)
  {
    if (!core::KvLimits::valid_value_size(size))
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "value is too large"));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<std::vector<std::uint8_t>>
  ValueCodec::encode(const KvValue &value)
  {
    auto validation = validate(value);

    if (validation.is_err())
    {
      return core::KvResult<std::vector<std::uint8_t>>::err(
          validation.error());
    }

    return core::KvResult<std::vector<std::uint8_t>>::ok(
        value.bytes());
  }

  core::KvResult<std::vector<std::uint8_t>>
  ValueCodec::encode(std::span<const std::uint8_t> value)
  {
    auto validation = validate_size(value.size());

    if (validation.is_err())
    {
      return core::KvResult<std::vector<std::uint8_t>>::err(
          validation.error());
    }

    return core::KvResult<std::vector<std::uint8_t>>::ok(
        std::vector<std::uint8_t>(value.begin(), value.end()));
  }

  core::KvResult<std::vector<std::uint8_t>>
  ValueCodec::encode(std::string_view value)
  {
    auto validation = validate_size(value.size());

    if (validation.is_err())
    {
      return core::KvResult<std::vector<std::uint8_t>>::err(
          validation.error());
    }

    return core::KvResult<std::vector<std::uint8_t>>::ok(
        std::vector<std::uint8_t>(value.begin(), value.end()));
  }

  core::KvResult<KvValue>
  ValueCodec::decode(const std::vector<std::uint8_t> &bytes)
  {
    auto validation = validate_size(bytes.size());

    if (validation.is_err())
    {
      return core::KvResult<KvValue>::err(validation.error());
    }

    return core::KvResult<KvValue>::ok(
        KvValue::from_bytes(bytes));
  }

  core::KvResult<KvValue>
  ValueCodec::decode(std::span<const std::uint8_t> bytes)
  {
    auto validation = validate_size(bytes.size());

    if (validation.is_err())
    {
      return core::KvResult<KvValue>::err(validation.error());
    }

    return core::KvResult<KvValue>::ok(
        KvValue::from_bytes(bytes));
  }

  core::KvResult<KvValue>
  ValueCodec::from_string(std::string_view value)
  {
    auto encoded = encode(value);

    if (encoded.is_err())
    {
      return core::KvResult<KvValue>::err(encoded.error());
    }

    return core::KvResult<KvValue>::ok(
        KvValue::from_bytes(encoded.move_value()));
  }

  core::KvResult<KvValue>
  ValueCodec::from_bytes(std::vector<std::uint8_t> bytes)
  {
    auto validation = validate_size(bytes.size());

    if (validation.is_err())
    {
      return core::KvResult<KvValue>::err(validation.error());
    }

    return core::KvResult<KvValue>::ok(
        KvValue::from_bytes(std::move(bytes)));
  }

  std::vector<std::uint8_t>
  ValueCodec::encode_or_empty(const KvValue &value)
  {
    auto encoded = encode(value);

    if (encoded.is_err())
    {
      return {};
    }

    return encoded.move_value();
  }

  bool ValueCodec::is_valid(const KvValue &value)
  {
    return validate(value).is_ok();
  }

  bool ValueCodec::is_valid_size(std::size_t size)
  {
    return validate_size(size).is_ok();
  }

} // namespace vix::kv::values
