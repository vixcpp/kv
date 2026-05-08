/**
 *
 *  @file Crc32.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  CRC32 checksum helper implementation
 *
 */

#include <vix/kv/checksum/Crc32.hpp>

namespace vix::kv::checksum
{
  std::uint32_t Crc32::compute(
      std::span<const std::uint8_t> data) noexcept
  {
    std::uint32_t crc = begin();
    crc = update(crc, data);
    return finish(crc);
  }

  std::uint32_t Crc32::compute(
      const std::vector<std::uint8_t> &data) noexcept
  {
    return compute(
        std::span<const std::uint8_t>(data.data(), data.size()));
  }

  std::uint32_t Crc32::compute(
      std::string_view data) noexcept
  {
    const auto *bytes =
        reinterpret_cast<const std::uint8_t *>(data.data());

    return compute(
        std::span<const std::uint8_t>(bytes, data.size()));
  }

  std::uint32_t Crc32::update(
      std::uint32_t current,
      std::span<const std::uint8_t> data) noexcept
  {
    std::uint32_t crc = current;

    for (const std::uint8_t byte : data)
    {
      crc = update_byte(crc, byte);
    }

    return crc;
  }

  bool Crc32::verify(
      std::span<const std::uint8_t> data,
      std::uint32_t expected) noexcept
  {
    return compute(data) == expected;
  }

  bool Crc32::verify(
      const std::vector<std::uint8_t> &data,
      std::uint32_t expected) noexcept
  {
    return compute(data) == expected;
  }

  bool Crc32::verify(
      std::string_view data,
      std::uint32_t expected) noexcept
  {
    return compute(data) == expected;
  }

  std::uint32_t Crc32::update_byte(
      std::uint32_t current,
      std::uint8_t byte) noexcept
  {
    std::uint32_t crc = current ^ byte;

    for (int i = 0; i < 8; ++i)
    {
      const bool bit_set = (crc & 1U) != 0U;

      crc >>= 1U;

      if (bit_set)
      {
        crc ^= polynomial;
      }
    }

    return crc;
  }

} // namespace vix::kv::checksum
