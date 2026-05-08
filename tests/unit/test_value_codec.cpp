/**
 *
 *  @file test_value_codec.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  ValueCodec unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/values/KvValue.hpp>
#include <vix/kv/values/ValueCodec.hpp>

#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace values = vix::kv::values;

  bool expect_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      return false;
    }

    return true;
  }

  template <typename T>
  bool expect_eq(const T &actual, const T &expected, const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: " << expected << '\n';
      std::cerr << "  actual  : " << actual << '\n';
      return false;
    }

    return true;
  }

  bool test_validate_empty_value()
  {
    const auto value = values::KvValue::from_string("");

    const auto result = values::ValueCodec::validate(value);

    return expect_true(
        result.is_ok(),
        "empty values should be valid");
  }

  bool test_encode_string_value()
  {
    const auto value = values::KvValue::from_string("hello");

    auto encoded = values::ValueCodec::encode(value);

    if (!expect_true(encoded.is_ok(), "encoding string value should succeed"))
    {
      return false;
    }

    const std::vector<std::uint8_t> expected{
        static_cast<std::uint8_t>('h'),
        static_cast<std::uint8_t>('e'),
        static_cast<std::uint8_t>('l'),
        static_cast<std::uint8_t>('l'),
        static_cast<std::uint8_t>('o')};

    return expect_true(
        encoded.value() == expected,
        "encoded string should match raw bytes");
  }

  bool test_encode_string_view()
  {
    constexpr std::string_view text = "vix kv";

    auto encoded = values::ValueCodec::encode(text);

    if (!expect_true(encoded.is_ok(), "encoding string_view should succeed"))
    {
      return false;
    }

    const std::string decoded_text(
        encoded.value().begin(),
        encoded.value().end());

    return expect_eq<std::string>(
        decoded_text,
        "vix kv",
        "encoded string_view bytes should match input text");
  }

  bool test_encode_span()
  {
    const std::vector<std::uint8_t> input{
        0x00U,
        0x01U,
        0x02U,
        0xFFU};

    auto encoded = values::ValueCodec::encode(
        std::span<const std::uint8_t>(input.data(), input.size()));

    if (!expect_true(encoded.is_ok(), "encoding byte span should succeed"))
    {
      return false;
    }

    return expect_true(
        encoded.value() == input,
        "encoded byte span should preserve bytes");
  }

  bool test_decode_bytes()
  {
    const std::vector<std::uint8_t> input{
        static_cast<std::uint8_t>('A'),
        static_cast<std::uint8_t>('d'),
        static_cast<std::uint8_t>('a')};

    auto decoded = values::ValueCodec::decode(input);

    if (!expect_true(decoded.is_ok(), "decoding bytes should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        decoded.value().to_string(),
        "Ada",
        "decoded bytes should produce expected string");
  }

  bool test_decode_span()
  {
    const std::vector<std::uint8_t> input{
        static_cast<std::uint8_t>('G'),
        static_cast<std::uint8_t>('r'),
        static_cast<std::uint8_t>('a'),
        static_cast<std::uint8_t>('c'),
        static_cast<std::uint8_t>('e')};

    auto decoded = values::ValueCodec::decode(
        std::span<const std::uint8_t>(input.data(), input.size()));

    if (!expect_true(decoded.is_ok(), "decoding span should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        decoded.value().to_string(),
        "Grace",
        "decoded span should produce expected string");
  }

  bool test_from_string()
  {
    auto value = values::ValueCodec::from_string("hello world");

    if (!expect_true(value.is_ok(), "from_string should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        value.value().to_string(),
        "hello world",
        "from_string should preserve text");
  }

  bool test_from_bytes()
  {
    std::vector<std::uint8_t> input{
        0x10U,
        0x20U,
        0x30U};

    auto value = values::ValueCodec::from_bytes(input);

    if (!expect_true(value.is_ok(), "from_bytes should succeed"))
    {
      return false;
    }

    return expect_true(
        value.value().bytes() == input,
        "from_bytes should preserve raw bytes");
  }

  bool test_encode_or_empty_success()
  {
    const auto value = values::KvValue::from_string("ok");

    const auto encoded = values::ValueCodec::encode_or_empty(value);

    return expect_true(
        !encoded.empty(),
        "encode_or_empty should return bytes for valid value");
  }

  bool test_is_valid()
  {
    const auto value = values::KvValue::from_string("valid");

    return expect_true(
        values::ValueCodec::is_valid(value),
        "is_valid should return true for normal value");
  }

  bool test_is_valid_size()
  {
    return expect_true(
        values::ValueCodec::is_valid_size(0),
        "size 0 should be valid because empty values are allowed");
  }

  bool test_large_value_rejected()
  {
    const std::size_t invalid_size =
        core::KvLimits::max_value_size + 1U;

    auto result = values::ValueCodec::validate_size(invalid_size);

    if (!expect_true(
            result.is_err(),
            "value larger than max_value_size should be rejected"))
    {
      return false;
    }

    return expect_true(
        result.error().code() == core::KvErrorCode::InvalidArgument,
        "large value should return InvalidArgument");
  }
}

int main()
{
  if (!test_validate_empty_value())
  {
    return 1;
  }

  if (!test_encode_string_value())
  {
    return 1;
  }

  if (!test_encode_string_view())
  {
    return 1;
  }

  if (!test_encode_span())
  {
    return 1;
  }

  if (!test_decode_bytes())
  {
    return 1;
  }

  if (!test_decode_span())
  {
    return 1;
  }

  if (!test_from_string())
  {
    return 1;
  }

  if (!test_from_bytes())
  {
    return 1;
  }

  if (!test_encode_or_empty_success())
  {
    return 1;
  }

  if (!test_is_valid())
  {
    return 1;
  }

  if (!test_is_valid_size())
  {
    return 1;
  }

  if (!test_large_value_rejected())
  {
    return 1;
  }

  std::cout << "kv_test_value_codec passed\n";
  return 0;
}
