/**
 *
 *  @file Endian.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Little-endian encoding helpers
 *
 */

#ifndef VIX_KV_UTILS_ENDIAN_HPP
#define VIX_KV_UTILS_ENDIAN_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace vix::kv::utils
{
  /**
   * @brief Little-endian integer encoding helpers.
   *
   * Vix KV uses explicit little-endian encoding for stable on-disk formats.
   *
   * These helpers are used by:
   * - record encoder
   * - record decoder
   * - WAL reader
   * - WAL writer
   * - snapshot reader
   * - snapshot writer
   */
  class Endian
  {
  public:
    /**
     * @brief Appends an unsigned 8-bit integer.
     *
     * @param out Destination buffer.
     * @param value Value to append.
     */
    static void append_u8(
        std::vector<std::uint8_t> &out,
        std::uint8_t value)
    {
      out.push_back(value);
    }

    /**
     * @brief Appends an unsigned 16-bit integer in little-endian order.
     *
     * @param out Destination buffer.
     * @param value Value to append.
     */
    static void append_u16(
        std::vector<std::uint8_t> &out,
        std::uint16_t value)
    {
      for (std::uint8_t i = 0; i < 2; ++i)
      {
        out.push_back(
            static_cast<std::uint8_t>(
                (value >> (i * 8U)) & 0xFFU));
      }
    }

    /**
     * @brief Appends an unsigned 32-bit integer in little-endian order.
     *
     * @param out Destination buffer.
     * @param value Value to append.
     */
    static void append_u32(
        std::vector<std::uint8_t> &out,
        std::uint32_t value)
    {
      for (std::uint8_t i = 0; i < 4; ++i)
      {
        out.push_back(
            static_cast<std::uint8_t>(
                (value >> (i * 8U)) & 0xFFU));
      }
    }

    /**
     * @brief Appends an unsigned 64-bit integer in little-endian order.
     *
     * @param out Destination buffer.
     * @param value Value to append.
     */
    static void append_u64(
        std::vector<std::uint8_t> &out,
        std::uint64_t value)
    {
      for (std::uint8_t i = 0; i < 8; ++i)
      {
        out.push_back(
            static_cast<std::uint8_t>(
                (value >> (i * 8U)) & 0xFFU));
      }
    }

    /**
     * @brief Appends a signed 64-bit integer in little-endian order.
     *
     * @param out Destination buffer.
     * @param value Value to append.
     */
    static void append_i64(
        std::vector<std::uint8_t> &out,
        std::int64_t value)
    {
      append_u64(out, static_cast<std::uint64_t>(value));
    }

    /**
     * @brief Reads an unsigned 8-bit integer.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param value Output value.
     * @return true on success.
     */
    [[nodiscard]] static bool read_u8(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::uint8_t &value) noexcept
    {
      if (!can_read(data, offset, 1))
      {
        return false;
      }

      value = data[offset];
      ++offset;
      return true;
    }

    /**
     * @brief Reads an unsigned 16-bit integer in little-endian order.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param value Output value.
     * @return true on success.
     */
    [[nodiscard]] static bool read_u16(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::uint16_t &value) noexcept
    {
      if (!can_read(data, offset, 2))
      {
        return false;
      }

      std::uint16_t out = 0;

      for (std::uint8_t i = 0; i < 2; ++i)
      {
        out |= static_cast<std::uint16_t>(
                   data[offset + i])
               << (i * 8U);
      }

      offset += 2;
      value = out;
      return true;
    }

    /**
     * @brief Reads an unsigned 32-bit integer in little-endian order.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param value Output value.
     * @return true on success.
     */
    [[nodiscard]] static bool read_u32(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::uint32_t &value) noexcept
    {
      if (!can_read(data, offset, 4))
      {
        return false;
      }

      std::uint32_t out = 0;

      for (std::uint8_t i = 0; i < 4; ++i)
      {
        out |= static_cast<std::uint32_t>(
                   data[offset + i])
               << (i * 8U);
      }

      offset += 4;
      value = out;
      return true;
    }

    /**
     * @brief Reads an unsigned 64-bit integer in little-endian order.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param value Output value.
     * @return true on success.
     */
    [[nodiscard]] static bool read_u64(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::uint64_t &value) noexcept
    {
      if (!can_read(data, offset, 8))
      {
        return false;
      }

      std::uint64_t out = 0;

      for (std::uint8_t i = 0; i < 8; ++i)
      {
        out |= static_cast<std::uint64_t>(
                   data[offset + i])
               << (i * 8U);
      }

      offset += 8;
      value = out;
      return true;
    }

    /**
     * @brief Reads a signed 64-bit integer in little-endian order.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param value Output value.
     * @return true on success.
     */
    [[nodiscard]] static bool read_i64(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::int64_t &value) noexcept
    {
      std::uint64_t raw = 0;

      if (!read_u64(data, offset, raw))
      {
        return false;
      }

      value = static_cast<std::int64_t>(raw);
      return true;
    }

    /**
     * @brief Returns true if count bytes can be read safely.
     *
     * @param data Source bytes.
     * @param offset Current offset.
     * @param count Number of bytes to read.
     * @return true when range is valid.
     */
    [[nodiscard]] static bool can_read(
        std::span<const std::uint8_t> data,
        std::size_t offset,
        std::size_t count) noexcept
    {
      return offset <= data.size() && count <= data.size() - offset;
    }
  };

} // namespace vix::kv::utils

#endif // VIX_KV_UTILS_ENDIAN_HPP
