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
 */
#ifndef VIX_KV_KEYS_KEY_ENCODER_HPP
#define VIX_KV_KEYS_KEY_ENCODER_HPP

#include <string>

#include <softadastra/store/types/Key.hpp>

#include <vix/kv/keys/KeyPath.hpp>

namespace vix::kv::keys
{
  /**
   * @brief Encodes a structured Vix KV key path into a flat Softadastra store key.
   *
   * The public API of Vix KV uses hierarchical key paths such as:
   *
   * {"users", "42", "profile"}
   *
   * Softadastra Store, however, expects a flat string key. This encoder is
   * responsible for converting between the two representations in a stable
   * and deterministic way.
   */
  class KeyEncoder
  {
  public:
    /**
     * @brief Encode a structured key path into a store key.
     *
     * @param path Structured key path.
     * @return Encoded flat store key.
     */
    [[nodiscard]] static softadastra::store::types::Key
    encode(const KeyPath &path);

    /**
     * @brief Encode a structured key path into a flat string.
     *
     * This is mainly useful internally for scans, debugging, and tests.
     *
     * @param path Structured key path.
     * @return Encoded flat string key.
     */
    [[nodiscard]] static std::string
    encode_to_string(const KeyPath &path);

    /**
     * @brief Return the separator used for flattened keys.
     *
     * @return Separator character.
     */
    [[nodiscard]] static constexpr char separator() noexcept
    {
      return '/';
    }
  };

} // namespace vix::kv::keys

#endif
