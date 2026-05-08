/**
 *
 *  @file ValueCodec.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Value encoding helpers
 *
 */

#ifndef VIX_KV_VALUES_VALUE_CODEC_HPP
#define VIX_KV_VALUES_VALUE_CODEC_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace vix::kv::values
{
  namespace core = vix::kv::core;

  /**
   * @brief Encodes and decodes KvValue payloads.
   *
   * ValueCodec is intentionally simple.
   *
   * In the KV engine, values are raw bytes. The codec exists to keep value
   * validation and byte conversions centralized.
   *
   * Rules:
   * - values are binary-safe.
   * - empty values are allowed.
   * - encoded bytes are exactly the user bytes.
   * - the codec does not add metadata.
   */
  class ValueCodec
  {
  public:
    /**
     * @brief Validates a KvValue using default limits.
     *
     * @param value KV value.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate(const KvValue &value);

    /**
     * @brief Validates a byte size using default limits.
     *
     * @param size Byte size.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_size(std::size_t size);

    /**
     * @brief Encodes KvValue into raw bytes.
     *
     * @param value KV value.
     * @return Encoded bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<std::vector<std::uint8_t>>
    encode(const KvValue &value);

    /**
     * @brief Encodes a byte span into owned raw bytes.
     *
     * @param value Value bytes.
     * @return Encoded bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<std::vector<std::uint8_t>>
    encode(std::span<const std::uint8_t> value);

    /**
     * @brief Encodes a string view into raw bytes.
     *
     * @param value Text bytes.
     * @return Encoded bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<std::vector<std::uint8_t>>
    encode(std::string_view value);

    /**
     * @brief Decodes raw bytes into KvValue.
     *
     * @param bytes Raw bytes.
     * @return KvValue or KvError.
     */
    [[nodiscard]] static core::KvResult<KvValue>
    decode(const std::vector<std::uint8_t> &bytes);

    /**
     * @brief Decodes raw bytes into KvValue.
     *
     * @param bytes Raw byte span.
     * @return KvValue or KvError.
     */
    [[nodiscard]] static core::KvResult<KvValue>
    decode(std::span<const std::uint8_t> bytes);

    /**
     * @brief Converts a string view directly to KvValue.
     *
     * @param value Text bytes.
     * @return KvValue or KvError.
     */
    [[nodiscard]] static core::KvResult<KvValue>
    from_string(std::string_view value);

    /**
     * @brief Converts raw bytes directly to KvValue.
     *
     * @param bytes Raw bytes.
     * @return KvValue or KvError.
     */
    [[nodiscard]] static core::KvResult<KvValue>
    from_bytes(std::vector<std::uint8_t> bytes);

    /**
     * @brief Returns an encoded byte vector without error reporting.
     *
     * This is useful for tests and compatibility code.
     * Production code should prefer encode().
     *
     * @param value KV value.
     * @return Encoded bytes or empty vector on invalid input.
     */
    [[nodiscard]] static std::vector<std::uint8_t>
    encode_or_empty(const KvValue &value);

    /**
     * @brief Returns true if the value is valid.
     *
     * @param value KV value.
     * @return true when validate() succeeds.
     */
    [[nodiscard]] static bool is_valid(const KvValue &value);

    /**
     * @brief Returns true if a byte size is valid.
     *
     * @param size Value size.
     * @return true when validate_size() succeeds.
     */
    [[nodiscard]] static bool is_valid_size(std::size_t size);
  };

} // namespace vix::kv::values

#endif // VIX_KV_VALUES_VALUE_CODEC_HPP
