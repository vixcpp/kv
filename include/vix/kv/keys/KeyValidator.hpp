/**
 *
 *  @file KeyValidator.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KeyPath validation helpers
 *
 */

#ifndef VIX_KV_KEYS_KEY_VALIDATOR_HPP
#define VIX_KV_KEYS_KEY_VALIDATOR_HPP

#include <cstddef>
#include <string_view>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/keys/KeyPath.hpp>

namespace vix::kv::keys
{
  namespace core = vix::kv::core;

  /**
   * @brief Validates public KeyPath values before storage.
   *
   * KeyValidator is used by:
   * - KeyEncoder
   * - KvEngine
   * - public API checks
   * - tests
   *
   * Rules:
   * - key path must not be empty.
   * - segment count must be within limits.
   * - each segment must not be empty.
   * - each segment must fit max_key_segment_size.
   * - total segment bytes must fit max_key_size.
   *
   * The encoder may add extra bytes for length prefixes.
   * That encoded size is checked by KeyEncoder.
   */
  class KeyValidator
  {
  public:
    /**
     * @brief Validates a key path with default limits.
     *
     * @param path Key path.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate(const KeyPath &path)
    {
      return validate(
          path,
          core::KvLimits::max_key_segments,
          core::KvLimits::max_key_segment_size,
          core::KvLimits::max_key_size);
    }

    /**
     * @brief Validates a key path with custom limits.
     *
     * @param path Key path.
     * @param max_segments Maximum number of path segments.
     * @param max_segment_size Maximum size of one segment.
     * @param max_total_size Maximum total segment byte size.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate(
        const KeyPath &path,
        std::size_t max_segments,
        std::size_t max_segment_size,
        std::size_t max_total_size)
    {
      if (path.empty())
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_key(
                "key path must not be empty"));
      }

      if (max_segments == 0)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_argument(
                "max key segments must be greater than zero"));
      }

      if (max_segment_size == 0)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_argument(
                "max key segment size must be greater than zero"));
      }

      if (max_total_size == 0)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_argument(
                "max key size must be greater than zero"));
      }

      if (path.size() > max_segments)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_key(
                "key path has too many segments"));
      }

      std::size_t total_size = 0;

      for (const auto &segment : path.segments())
      {
        const auto segment_result =
            validate_segment(segment, max_segment_size);

        if (segment_result.is_err())
        {
          return segment_result;
        }

        total_size += segment.size();

        if (total_size > max_total_size)
        {
          return core::KvResult<void>::err(
              core::KvError::invalid_key(
                  "key path is too large"));
        }
      }

      return core::KvResult<void>::ok();
    }

    /**
     * @brief Validates one key segment with default limits.
     *
     * @param segment Key segment.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_segment(std::string_view segment)
    {
      return validate_segment(
          segment,
          core::KvLimits::max_key_segment_size);
    }

    /**
     * @brief Validates one key segment with a custom size limit.
     *
     * @param segment Key segment.
     * @param max_segment_size Maximum segment size.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate_segment(
        std::string_view segment,
        std::size_t max_segment_size)
    {
      if (segment.empty())
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_key(
                "key segment must not be empty"));
      }

      if (max_segment_size == 0)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_argument(
                "max key segment size must be greater than zero"));
      }

      if (segment.size() > max_segment_size)
      {
        return core::KvResult<void>::err(
            core::KvError::invalid_key(
                "key segment is too large"));
      }

      return core::KvResult<void>::ok();
    }

    /**
     * @brief Returns true when a key path is valid with default limits.
     *
     * @param path Key path.
     * @return true when validate(path) succeeds.
     */
    [[nodiscard]] static bool is_valid(const KeyPath &path)
    {
      return validate(path).is_ok();
    }

    /**
     * @brief Returns true when a segment is valid with default limits.
     *
     * @param segment Key segment.
     * @return true when validate_segment(segment) succeeds.
     */
    [[nodiscard]] static bool is_valid_segment(std::string_view segment)
    {
      return validate_segment(segment).is_ok();
    }

    /**
     * @brief Computes total segment bytes.
     *
     * This does not include encoded length prefixes.
     *
     * @param path Key path.
     * @return Total raw segment bytes.
     */
    [[nodiscard]] static std::size_t raw_size(
        const KeyPath &path) noexcept
    {
      return path.byte_size();
    }
  };

} // namespace vix::kv::keys

#endif // VIX_KV_KEYS_KEY_VALIDATOR_HPP
