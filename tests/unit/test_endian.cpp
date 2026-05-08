/**
 *
 *  @file test_endian.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Endian unit tests
 *
 */

#include <vix/kv/utils/Endian.hpp>

#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace
{
  namespace utils = vix::kv::utils;

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
  bool expect_eq(T actual, T expected, const char *message)
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

  bool test_append_u8()
  {
    std::vector<std::uint8_t> out;

    utils::Endian::append_u8(out, 0xABU);

    return expect_true(
        out.size() == 1 &&
            out[0] == 0xABU,
        "append_u8 should append one byte");
  }

  bool test_append_u16_little_endian()
  {
    std::vector<std::uint8_t> out;

    utils::Endian::append_u16(out, 0x1234U);

    return expect_true(
        out.size() == 2 &&
            out[0] == 0x34U &&
            out[1] == 0x12U,
        "append_u16 should write little-endian bytes");
  }

  bool test_append_u32_little_endian()
  {
    std::vector<std::uint8_t> out;

    utils::Endian::append_u32(out, 0x12345678U);

    return expect_true(
        out.size() == 4 &&
            out[0] == 0x78U &&
            out[1] == 0x56U &&
            out[2] == 0x34U &&
            out[3] == 0x12U,
        "append_u32 should write little-endian bytes");
  }

  bool test_append_u64_little_endian()
  {
    std::vector<std::uint8_t> out;

    utils::Endian::append_u64(out, 0x0102030405060708ULL);

    return expect_true(
        out.size() == 8 &&
            out[0] == 0x08U &&
            out[1] == 0x07U &&
            out[2] == 0x06U &&
            out[3] == 0x05U &&
            out[4] == 0x04U &&
            out[5] == 0x03U &&
            out[6] == 0x02U &&
            out[7] == 0x01U,
        "append_u64 should write little-endian bytes");
  }

  bool test_read_u8()
  {
    const std::vector<std::uint8_t> data{0xABU};
    std::size_t offset = 0;
    std::uint8_t value = 0;

    const bool ok = utils::Endian::read_u8(
        std::span<const std::uint8_t>(data.data(), data.size()),
        offset,
        value);

    return expect_true(ok, "read_u8 should succeed") &&
           expect_eq<std::uint8_t>(
               value,
               0xABU,
               "read_u8 should read the expected value") &&
           expect_eq<std::size_t>(
               offset,
               1,
               "read_u8 should advance offset by 1");
  }

  bool test_read_u16()
  {
    const std::vector<std::uint8_t> data{0x34U, 0x12U};
    std::size_t offset = 0;
    std::uint16_t value = 0;

    const bool ok = utils::Endian::read_u16(
        std::span<const std::uint8_t>(data.data(), data.size()),
        offset,
        value);

    return expect_true(ok, "read_u16 should succeed") &&
           expect_eq<std::uint16_t>(
               value,
               0x1234U,
               "read_u16 should decode little-endian value") &&
           expect_eq<std::size_t>(
               offset,
               2,
               "read_u16 should advance offset by 2");
  }

  bool test_read_u32()
  {
    const std::vector<std::uint8_t> data{
        0x78U,
        0x56U,
        0x34U,
        0x12U};

    std::size_t offset = 0;
    std::uint32_t value = 0;

    const bool ok = utils::Endian::read_u32(
        std::span<const std::uint8_t>(data.data(), data.size()),
        offset,
        value);

    return expect_true(ok, "read_u32 should succeed") &&
           expect_eq<std::uint32_t>(
               value,
               0x12345678U,
               "read_u32 should decode little-endian value") &&
           expect_eq<std::size_t>(
               offset,
               4,
               "read_u32 should advance offset by 4");
  }

  bool test_read_u64()
  {
    const std::vector<std::uint8_t> data{
        0x08U,
        0x07U,
        0x06U,
        0x05U,
        0x04U,
        0x03U,
        0x02U,
        0x01U};

    std::size_t offset = 0;
    std::uint64_t value = 0;

    const bool ok = utils::Endian::read_u64(
        std::span<const std::uint8_t>(data.data(), data.size()),
        offset,
        value);

    return expect_true(ok, "read_u64 should succeed") &&
           expect_eq<std::uint64_t>(
               value,
               0x0102030405060708ULL,
               "read_u64 should decode little-endian value") &&
           expect_eq<std::size_t>(
               offset,
               8,
               "read_u64 should advance offset by 8");
  }

  bool test_roundtrip_values()
  {
    std::vector<std::uint8_t> data;

    utils::Endian::append_u8(data, 0x11U);
    utils::Endian::append_u16(data, 0x2233U);
    utils::Endian::append_u32(data, 0x44556677U);
    utils::Endian::append_u64(data, 0x8899AABBCCDDEEFFULL);

    std::size_t offset = 0;

    std::uint8_t value8 = 0;
    std::uint16_t value16 = 0;
    std::uint32_t value32 = 0;
    std::uint64_t value64 = 0;

    const auto span =
        std::span<const std::uint8_t>(data.data(), data.size());

    const bool ok =
        utils::Endian::read_u8(span, offset, value8) &&
        utils::Endian::read_u16(span, offset, value16) &&
        utils::Endian::read_u32(span, offset, value32) &&
        utils::Endian::read_u64(span, offset, value64);

    return expect_true(ok, "roundtrip reads should succeed") &&
           expect_eq<std::uint8_t>(
               value8,
               0x11U,
               "roundtrip u8 should match") &&
           expect_eq<std::uint16_t>(
               value16,
               0x2233U,
               "roundtrip u16 should match") &&
           expect_eq<std::uint32_t>(
               value32,
               0x44556677U,
               "roundtrip u32 should match") &&
           expect_eq<std::uint64_t>(
               value64,
               0x8899AABBCCDDEEFFULL,
               "roundtrip u64 should match") &&
           expect_eq<std::size_t>(
               offset,
               data.size(),
               "roundtrip should consume all bytes");
  }

  bool test_read_failure_does_not_advance_offset()
  {
    const std::vector<std::uint8_t> data{0x01U, 0x02U};
    std::size_t offset = 1;
    std::uint32_t value = 0;

    const bool ok = utils::Endian::read_u32(
        std::span<const std::uint8_t>(data.data(), data.size()),
        offset,
        value);

    return expect_true(!ok, "read_u32 should fail when truncated") &&
           expect_eq<std::size_t>(
               offset,
               1,
               "failed read should not advance offset");
  }
}

int main()
{
  if (!test_append_u8())
  {
    return 1;
  }

  if (!test_append_u16_little_endian())
  {
    return 1;
  }

  if (!test_append_u32_little_endian())
  {
    return 1;
  }

  if (!test_append_u64_little_endian())
  {
    return 1;
  }

  if (!test_read_u8())
  {
    return 1;
  }

  if (!test_read_u16())
  {
    return 1;
  }

  if (!test_read_u32())
  {
    return 1;
  }

  if (!test_read_u64())
  {
    return 1;
  }

  if (!test_roundtrip_values())
  {
    return 1;
  }

  if (!test_read_failure_does_not_advance_offset())
  {
    return 1;
  }

  std::cout << "kv_test_endian passed\n";
  return 0;
}
