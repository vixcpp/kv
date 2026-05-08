/**
 *
 *  @file test_crc32.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  CRC32 unit tests
 *
 */

#include <vix/kv/checksum/Crc32.hpp>

#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace
{
  namespace checksum = vix::kv::checksum;

  bool expect_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      return false;
    }

    return true;
  }

  bool expect_eq(
      std::uint32_t actual,
      std::uint32_t expected,
      const char *message)
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

  bool test_empty_checksum()
  {
    const std::vector<std::uint8_t> data;

    const auto crc = checksum::Crc32::compute(data);

    return expect_eq(
        crc,
        0U,
        "CRC32 of empty data should be 0");
  }

  bool test_known_string_checksum()
  {
    constexpr std::string_view text = "123456789";

    const auto crc = checksum::Crc32::compute(text);

    return expect_eq(
        crc,
        0xCBF43926U,
        "CRC32 of 123456789 should match the standard check value");
  }

  bool test_vector_and_string_match()
  {
    constexpr std::string_view text = "hello world";

    const std::vector<std::uint8_t> bytes(
        text.begin(),
        text.end());

    const auto from_string = checksum::Crc32::compute(text);
    const auto from_vector = checksum::Crc32::compute(bytes);

    return expect_eq(
        from_vector,
        from_string,
        "CRC32 from vector and string_view should match");
  }

  bool test_span_and_vector_match()
  {
    const std::vector<std::uint8_t> bytes{
        0x00U,
        0x01U,
        0x02U,
        0x03U,
        0xFFU};

    const auto from_vector = checksum::Crc32::compute(bytes);
    const auto from_span = checksum::Crc32::compute(
        std::span<const std::uint8_t>(
            bytes.data(),
            bytes.size()));

    return expect_eq(
        from_span,
        from_vector,
        "CRC32 from span and vector should match");
  }

  bool test_incremental_checksum()
  {
    constexpr std::string_view first = "hello ";
    constexpr std::string_view second = "world";
    constexpr std::string_view full = "hello world";

    auto crc = checksum::Crc32::begin();

    crc = checksum::Crc32::update(
        crc,
        std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>(first.data()),
            first.size()));

    crc = checksum::Crc32::update(
        crc,
        std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>(second.data()),
            second.size()));

    const auto incremental = checksum::Crc32::finish(crc);
    const auto one_shot = checksum::Crc32::compute(full);

    return expect_eq(
        incremental,
        one_shot,
        "incremental CRC32 should match one-shot CRC32");
  }

  bool test_verify_success()
  {
    constexpr std::string_view text = "vix kv";

    const auto crc = checksum::Crc32::compute(text);

    return expect_true(
        checksum::Crc32::verify(text, crc),
        "verify should return true for matching checksum");
  }

  bool test_verify_failure()
  {
    constexpr std::string_view text = "vix kv";

    const auto crc = checksum::Crc32::compute(text);

    return expect_true(
        !checksum::Crc32::verify(text, crc + 1U),
        "verify should return false for wrong checksum");
  }
}

int main()
{
  if (!test_empty_checksum())
  {
    return 1;
  }

  if (!test_known_string_checksum())
  {
    return 1;
  }

  if (!test_vector_and_string_match())
  {
    return 1;
  }

  if (!test_span_and_vector_match())
  {
    return 1;
  }

  if (!test_incremental_checksum())
  {
    return 1;
  }

  if (!test_verify_success())
  {
    return 1;
  }

  if (!test_verify_failure())
  {
    return 1;
  }

  std::cout << "kv_test_crc32 passed\n";
  return 0;
}
