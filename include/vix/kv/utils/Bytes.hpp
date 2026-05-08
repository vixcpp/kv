/**
 *
 *  @file Bytes.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Byte buffer helpers
 *
 */

#ifndef VIX_KV_UTILS_BYTES_HPP
#define VIX_KV_UTILS_BYTES_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/utils/Endian.hpp>

namespace vix::kv::utils
{
  namespace core = vix::kv::core;

  /**
   * @brief Byte buffer utility helpers.
   *
   * Bytes provides small reusable helpers for stable binary encoding and
   * decoding.
   *
   * It is used by:
   * - record encoders
   * - record decoders
   * - WAL readers
   * - WAL writers
   * - snapshot codecs
   */
  class Bytes
  {
  public:
    /**
     * @brief Byte vector type.
     */
    using Buffer = std::vector<std::uint8_t>;

    /**
     * @brief Returns true if count bytes can be read from offset.
     *
     * @param data Source bytes.
     * @param offset Current offset.
     * @param count Number of bytes to read.
     * @return true when the read is safe.
     */
    [[nodiscard]] static bool can_read(
        std::span<const std::uint8_t> data,
        std::size_t offset,
        std::size_t count) noexcept
    {
      return Endian::can_read(data, offset, count);
    }

    /**
     * @brief Appends raw bytes to a buffer.
     *
     * @param out Destination buffer.
     * @param data Bytes to append.
     */
    static void append(
        Buffer &out,
        std::span<const std::uint8_t> data)
    {
      out.insert(out.end(), data.begin(), data.end());
    }

    /**
     * @brief Appends raw bytes to a buffer.
     *
     * @param out Destination buffer.
     * @param data Bytes to append.
     */
    static void append(
        Buffer &out,
        const Buffer &data)
    {
      out.insert(out.end(), data.begin(), data.end());
    }

    /**
     * @brief Appends text bytes to a buffer.
     *
     * @param out Destination buffer.
     * @param text Text bytes to append.
     */
    static void append_string_bytes(
        Buffer &out,
        std::string_view text)
    {
      out.insert(out.end(), text.begin(), text.end());
    }

    /**
     * @brief Appends a length-prefixed byte span.
     *
     * Format:
     * @code
     * uint32 size
     * bytes payload
     * @endcode
     *
     * @param out Destination buffer.
     * @param data Bytes to append.
     */
    static void append_bytes_u32(
        Buffer &out,
        std::span<const std::uint8_t> data)
    {
      Endian::append_u32(
          out,
          static_cast<std::uint32_t>(data.size()));

      append(out, data);
    }

    /**
     * @brief Appends a length-prefixed byte vector.
     *
     * @param out Destination buffer.
     * @param data Bytes to append.
     */
    static void append_bytes_u32(
        Buffer &out,
        const Buffer &data)
    {
      Endian::append_u32(
          out,
          static_cast<std::uint32_t>(data.size()));

      append(out, data);
    }

    /**
     * @brief Appends a length-prefixed string.
     *
     * Format:
     * @code
     * uint32 size
     * bytes string
     * @endcode
     *
     * @param out Destination buffer.
     * @param text Text bytes to append.
     */
    static void append_string_u32(
        Buffer &out,
        std::string_view text)
    {
      Endian::append_u32(
          out,
          static_cast<std::uint32_t>(text.size()));

      append_string_bytes(out, text);
    }

    /**
     * @brief Reads a raw byte range.
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @param count Number of bytes to read.
     * @return Byte vector or KvError.
     */
    [[nodiscard]] static core::KvResult<Buffer> read_bytes(
        std::span<const std::uint8_t> data,
        std::size_t &offset,
        std::size_t count)
    {
      if (!can_read(data, offset, count))
      {
        return core::KvResult<Buffer>::err(
            core::KvError::corruption(
                "byte buffer is truncated"));
      }

      Buffer out(
          data.begin() + static_cast<std::ptrdiff_t>(offset),
          data.begin() + static_cast<std::ptrdiff_t>(offset + count));

      offset += count;

      return core::KvResult<Buffer>::ok(std::move(out));
    }

    /**
     * @brief Reads a length-prefixed byte vector.
     *
     * Format:
     * @code
     * uint32 size
     * bytes payload
     * @endcode
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @return Byte vector or KvError.
     */
    [[nodiscard]] static core::KvResult<Buffer> read_bytes_u32(
        std::span<const std::uint8_t> data,
        std::size_t &offset)
    {
      std::uint32_t size = 0;

      if (!Endian::read_u32(data, offset, size))
      {
        return core::KvResult<Buffer>::err(
            core::KvError::corruption(
                "failed to read byte buffer size"));
      }

      return read_bytes(data, offset, size);
    }

    /**
     * @brief Reads a length-prefixed string.
     *
     * Format:
     * @code
     * uint32 size
     * bytes string
     * @endcode
     *
     * @param data Source bytes.
     * @param offset Current offset, advanced on success.
     * @return String or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string> read_string_u32(
        std::span<const std::uint8_t> data,
        std::size_t &offset)
    {
      auto bytes = read_bytes_u32(data, offset);

      if (bytes.is_err())
      {
        return core::KvResult<std::string>::err(bytes.error());
      }

      const auto &buffer = bytes.value();

      std::string out(buffer.begin(), buffer.end());

      return core::KvResult<std::string>::ok(std::move(out));
    }

    /**
     * @brief Returns a subspan if the range is valid.
     *
     * @param data Source bytes.
     * @param offset Start offset.
     * @param count Number of bytes.
     * @return Subspan or KvError.
     */
    [[nodiscard]] static core::KvResult<std::span<const std::uint8_t>>
    view(
        std::span<const std::uint8_t> data,
        std::size_t offset,
        std::size_t count)
    {
      if (!can_read(data, offset, count))
      {
        return core::KvResult<std::span<const std::uint8_t>>::err(
            core::KvError::corruption(
                "byte view is out of range"));
      }

      return core::KvResult<std::span<const std::uint8_t>>::ok(
          data.subspan(offset, count));
    }

    /**
     * @brief Returns true when all bytes were consumed.
     *
     * @param data Source bytes.
     * @param offset Current offset.
     * @return true when offset equals data size.
     */
    [[nodiscard]] static bool fully_consumed(
        std::span<const std::uint8_t> data,
        std::size_t offset) noexcept
    {
      return offset == data.size();
    }

    /**
     * @brief Returns number of unread bytes.
     *
     * @param data Source bytes.
     * @param offset Current offset.
     * @return Remaining byte count.
     */
    [[nodiscard]] static std::size_t remaining(
        std::span<const std::uint8_t> data,
        std::size_t offset) noexcept
    {
      if (offset > data.size())
      {
        return 0;
      }

      return data.size() - offset;
    }

    /**
     * @brief Clears and releases a buffer.
     *
     * @param buffer Buffer to clear.
     */
    static void reset(Buffer &buffer)
    {
      Buffer empty;
      buffer.swap(empty);
    }
  };

} // namespace vix::kv::utils

#endif // VIX_KV_UTILS_BYTES_HPP
