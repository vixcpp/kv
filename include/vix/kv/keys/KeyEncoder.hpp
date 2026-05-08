/**
 *
 *  @file KeyEncoder.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Stable KeyPath encoder
 *
 */

#ifndef VIX_KV_KEYS_KEY_ENCODER_HPP
#define VIX_KV_KEYS_KEY_ENCODER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/keys/KeyPath.hpp>

namespace vix::kv::keys
{
  namespace core = vix::kv::core;

  /**
   * @brief Encodes and decodes public KeyPath values.
   *
   * KeyEncoder converts a structured KeyPath into a stable internal string.
   *
   * The encoded format is length-prefixed and unambiguous.
   *
   * Format version 1:
   *
   * @code
   * v1|<segment_size>:<segment><segment_size>:<segment>...
   * @endcode
   *
   * Example:
   *
   * @code
   * {"users", "42", "profile"}
   * -> v1|5:users2:427:profile
   * @endcode
   *
   * This avoids the ambiguity of separator-based encodings.
   */
  class KeyEncoder
  {
  public:
    /**
     * @brief Encoded key format version.
     */
    static constexpr std::uint8_t format_version = 1;

    /**
     * @brief Prefix used by version 1 encoded keys.
     */
    static constexpr std::string_view version_prefix = "v1|";

    /**
     * @brief Character between segment size and segment bytes.
     */
    static constexpr char size_separator = ':';

    /**
     * @brief Encodes a KeyPath into an internal key string.
     *
     * @param path Public key path.
     * @return Encoded key string or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string>
    encode(const KeyPath &path);

    /**
     * @brief Encodes a KeyPath into an internal key string.
     *
     * Alias for encode().
     *
     * @param path Public key path.
     * @return Encoded key string or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string>
    encode_to_string(const KeyPath &path);

    /**
     * @brief Encodes a KeyPath and returns an empty string on failure.
     *
     * This helper is useful in tests and compatibility paths.
     * Production code should prefer encode().
     *
     * @param path Public key path.
     * @return Encoded key or empty string on failure.
     */
    [[nodiscard]] static std::string
    encode_or_empty(const KeyPath &path);

    /**
     * @brief Decodes an internal key string back to a KeyPath.
     *
     * @param encoded Encoded key string.
     * @return Decoded KeyPath or KvError.
     */
    [[nodiscard]] static core::KvResult<KeyPath>
    decode(std::string_view encoded);

    /**
     * @brief Returns true when an encoded key has the supported prefix.
     *
     * @param encoded Encoded key string.
     * @return true if the version prefix is supported.
     */
    [[nodiscard]] static bool
    has_supported_prefix(std::string_view encoded) noexcept;

    /**
     * @brief Returns true when encoded starts with encoded_prefix.
     *
     * This is used for prefix scans over encoded keys.
     *
     * @param encoded Full encoded key.
     * @param encoded_prefix Encoded prefix.
     * @return true if encoded belongs to the prefix range.
     */
    [[nodiscard]] static bool
    matches_prefix(
        std::string_view encoded,
        std::string_view encoded_prefix) noexcept;

    /**
     * @brief Returns the number of bytes required to encode a KeyPath.
     *
     * Returns 0 for invalid paths or if the encoded size would exceed limits.
     *
     * @param path Public key path.
     * @return Encoded size in bytes, or 0 on invalid input.
     */
    [[nodiscard]] static std::size_t
    encoded_size(const KeyPath &path);

  private:
    /**
     * @brief Appends one encoded segment.
     *
     * @param out Destination string.
     * @param segment Segment bytes.
     */
    static void append_segment(
        std::string &out,
        std::string_view segment);

    /**
     * @brief Reads a decimal size from encoded input.
     *
     * @param encoded Encoded input.
     * @param offset Current offset, updated on success.
     * @param size Output segment size.
     * @return true on success.
     */
    [[nodiscard]] static bool read_size(
        std::string_view encoded,
        std::size_t &offset,
        std::size_t &size) noexcept;

    /**
     * @brief Returns true if count bytes can be read safely.
     *
     * @param data Input string.
     * @param offset Current offset.
     * @param count Number of bytes to read.
     * @return true when the range is valid.
     */
    [[nodiscard]] static bool can_read(
        std::string_view data,
        std::size_t offset,
        std::size_t count) noexcept;

    /**
     * @brief Returns number of decimal digits needed for value.
     *
     * @param value Integer value.
     * @return Decimal digit count.
     */
    [[nodiscard]] static std::size_t decimal_digits(
        std::size_t value) noexcept;
  };

} // namespace vix::kv::keys

#endif // VIX_KV_KEYS_KEY_ENCODER_HPP
